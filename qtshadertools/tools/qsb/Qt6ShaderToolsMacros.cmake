# Invokes qsb on each file in FILES. Extensions must be .vert, .frag, or .comp.
# The resulting .qsb files are added as resources under PREFIX.
# target and resourcename are like for qt6_add_resources.
#
# By default generates SPIR-V, GLSL (100 es, 120, 150), HLSL (shader model 5.0) and MSL (Metal 1.2).
# Configuring qsb:
#     Specify OUTPUTS as a list with a matching element for each entry in FILES to override the resulting .qsb name.
#         This makes sense when the desired name is different from the .vert/.frag source file name. (e.g. when DEFINES are involved)
#     Specify GLSL, HLSL, MSL to override the versions to generate.
#         Note: follows qsb and GLSL-style version syntax, e.g. "300 es,330".
#     Specify NOGLSL, NOHLSL, or NOMSL to skip generating a given language.
#         SPIR-V is always generated.
#     Specify PRECOMPILE to trigger invoking native tools where applicable.
#         F.ex. with HLSL enabled it passes -c to qsb which in turn runs fxc to store DXBC instead of HLSL.
#     Specify BATCHABLE to enable generating batchable vertex shader variants.
#         Mandatory for vertex shaders that are used with Qt Quick (2D) in materials or ShaderEffect.
#     Specify PERTARGETCOMPILE to compile to SPIR-V and translate separately per output language version.
#         Slow, but allows ifdefing based on QSHADER_<LANG>[_VERSION] macros.
#     Specify DEFINES with a "name1=value1;name2=value2" (or newline separated, like FILES) list to set custom macros for glslang.
#     Specify DEBUGINFO to enable generating full debug info where applicable (e.g. SPIR-V).
#     Specify OPTIMIZED to enable optimizing for performance where applicable.
#         For SPIR-V this involves invoking spirv-opt from SPIRV-Tools / the Vulkan SDK.
#     Specify QUIET to suppress all debug and warning prints from qsb.
#
# The actual file name in the resource system is either :/PREFIX/FILES[i]-BASE+".qsb" or :/PREFIX/OUTPUTS[i]
#
# The entries in FILES can contain @ separated replacement specifications after the filename.
# Example: FILES "wobble.frag@glsl,100es,my_custom_shader_for_gles.frag@spirv,100,my_custom_spirv_binary.spv"
# triggers an additional call to qsb in replace mode (only after the .qsb file is built by the initial run):
#   qsb -r glsl,100es,my_custom_shader_for_gles.frag -r spirv,100,my_custom_spirv_binary.spv wobble.frag.qsb
#
# NB! Most of this is documented in qtshadertools-build.qdoc. Changes without updating the documentation
# are not allowed.
#
# Example:
# qt6_add_shaders(testapp "testapp_shaders"
#    BATCHABLE
#    PRECOMPILE
#    PREFIX
#        "/shaders"
#    FILES
#        color.vert
#        color.frag
# )
# This leads to :/shaders/color.vert.qsb and :/shaders/color.frag.qsb being available in the application.
#
function(_qt_internal_add_shaders_impl target resourcename)
    cmake_parse_arguments(
        arg
        "BATCHABLE;PRECOMPILE;PERTARGETCOMPILE;NOGLSL;NOHLSL;NOMSL;DEBUGINFO;OPTIMIZED;SILENT;QUIET;_QT_INTERNAL"
        "PREFIX;BASE;GLSL;HLSL;MSL"
        "FILES;OUTPUTS;DEFINES"
        ${ARGN}
    )

    math(EXPR file_index "0")
    foreach(file_and_replacements IN LISTS arg_FILES)
        string(REPLACE "@" ";" file_and_replacement_list "${file_and_replacements}")
        list(GET file_and_replacement_list 0 file)
        list(LENGTH file_and_replacement_list replacement_count_plus_one)
        set(qsb_replace_args "")
        if(replacement_count_plus_one GREATER 1)
            math(EXPR replacement_count "${replacement_count_plus_one}-1")
            foreach(replacement_idx RANGE 1 ${replacement_count})
                list(GET file_and_replacement_list ${replacement_idx} replacement)
                # Get a list, f.ex. "glsl;100es;some/where/shader.frag" so that we can
                # adjust the filename (3rd component) to be absolute.
                string(REPLACE "," ";" replacement_list "${replacement}")
                list(GET replacement_list 2 replace_source_file)
                get_filename_component(absolute_replace_source_file ${replace_source_file} ABSOLUTE)
                list(REMOVE_AT replacement_list 2)
                list(APPEND replacement_list "${absolute_replace_source_file}")
                list(JOIN replacement_list "," qsb_replace_spec)
                list(APPEND qsb_replace_args "-r")
                list(APPEND qsb_replace_args "${qsb_replace_spec}")
            endforeach()
        endif()

        set(output_file "${file}.qsb")
        if(arg_OUTPUTS)
            list(GET arg_OUTPUTS ${file_index} output_file)
        elseif(arg_BASE)
            get_filename_component(abs_base "${arg_BASE}" ABSOLUTE)
            get_filename_component(abs_output_file "${output_file}" ABSOLUTE)
            file(RELATIVE_PATH output_file "${abs_base}" "${abs_output_file}")
        endif()
        set(qsb_result "${CMAKE_CURRENT_BINARY_DIR}/.qsb/${output_file}")
        get_filename_component(file_absolute ${file} ABSOLUTE)

        if (NOT arg_SILENT AND NOT arg_QUIET)
            message("${file} -> ${output_file} exposed as :${arg_PREFIX}/${output_file}")
        endif()

        set(qsb_args "")

        if (NOT arg_NOGLSL)
            if (arg_GLSL)
                set(glsl_versions "${arg_GLSL}")
            else()
                set(glsl_versions "100es,120,150") # both 'es' and ' es' are accepted by qsb
            endif()
            list(APPEND qsb_args "--glsl")
            list(APPEND qsb_args "${glsl_versions}")
        endif()

        if (NOT arg_NOHLSL)
            if (arg_HLSL)
                set(shader_model_versions "${arg_HLSL}")
            else()
                set(shader_model_versions 50)
            endif()
            list(APPEND qsb_args "--hlsl")
            list(APPEND qsb_args "${shader_model_versions}")
        endif()

        if (NOT arg_NOMSL)
            if (arg_MSL)
                set(metal_lang_versions "${arg_MSL}")
            else()
                set(metal_lang_versions 12)
            endif()
            list(APPEND qsb_args "--msl")
            list(APPEND qsb_args "${metal_lang_versions}")
        endif()

        if (arg_BATCHABLE)
            list(APPEND qsb_args "-b")
        endif()

        if (arg_PRECOMPILE)
            if (WIN32 AND NOT arg_NOHLSL)
                list(APPEND qsb_args "-c")
            endif()
        endif()

        if (arg_PERTARGETCOMPILE)
            list(APPEND qsb_args "-p")
        endif()

        if (arg_DEBUGINFO)
            list(APPEND qsb_args "-g")
        endif()

        if (arg_OPTIMIZED)
            list(APPEND qsb_args "-O")
        endif()

        if (arg_SILENT)
            list(APPEND qsb_args "-s")
        endif()

        foreach(qsb_def IN LISTS arg_DEFINES)
            list(APPEND qsb_args "-D")
            list(APPEND qsb_args "${qsb_def}")
        endforeach()

        list(APPEND qsb_args "-o")
        list(APPEND qsb_args "${qsb_result}")

        list(APPEND qsb_args "${file_absolute}")

        if (qsb_replace_args)
            list(APPEND qsb_replace_args "${qsb_result}")
            if (arg_SILENT)
                list(APPEND qsb_replace_args "-s")
            endif()
            add_custom_command(
                OUTPUT
                    ${qsb_result}
                COMMAND
                    ${QT_CMAKE_EXPORT_NAMESPACE}::qsb ${qsb_args}
                COMMAND
                    ${QT_CMAKE_EXPORT_NAMESPACE}::qsb ${qsb_replace_args}
                DEPENDS
                    "${file_absolute}"
                    ${QT_CMAKE_EXPORT_NAMESPACE}::qsb
                VERBATIM
            )
        else()
            add_custom_command(
                OUTPUT
                    ${qsb_result}
                COMMAND
                    ${QT_CMAKE_EXPORT_NAMESPACE}::qsb ${qsb_args}
                DEPENDS
                    "${file_absolute}"
                    ${QT_CMAKE_EXPORT_NAMESPACE}::qsb
                VERBATIM
            )
        endif()

        list(APPEND qsb_files "${qsb_result}")
        set_source_files_properties("${qsb_result}" PROPERTIES QT_RESOURCE_ALIAS "${output_file}")

        math(EXPR file_index "${file_index}+1")
    endforeach()

    if (arg__QT_INTERNAL)
        qt_internal_add_resource(${target} ${resourcename}
            PREFIX
                "${arg_PREFIX}"
            FILES
                "${qsb_files}"
        )
    else()
        qt6_add_resources(${target} ${resourcename}
            PREFIX
                "${arg_PREFIX}"
            FILES
                "${qsb_files}"
        )
    endif()
endfunction()

function(qt6_add_shaders)
    _qt_internal_add_shaders_impl(${ARGV})
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_add_shaders)
        qt6_add_shaders(${ARGV})
    endfunction()
endif()

# for use by Qt modules that need qt_internal_add_resource
function(qt_internal_add_shaders)
    _qt_internal_add_shaders_impl(${ARGV} _QT_INTERNAL)
endfunction()
