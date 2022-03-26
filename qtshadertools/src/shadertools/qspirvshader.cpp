/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Shader Tools module
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qspirvshader_p.h"
#include "qspirvshaderremap_p.h"
#include <QtGui/private/qshaderdescription_p_p.h>
#include <QFile>
#include <QDebug>

#include <spirv_cross_c.h>

QT_BEGIN_NAMESPACE

struct QSpirvShaderPrivate
{
    ~QSpirvShaderPrivate();

    void createCompiler(spvc_backend backend);
    void processResources();

    QShaderDescription::InOutVariable inOutVar(const spvc_reflected_resource &r);
    QShaderDescription::BlockVariable blockVar(spvc_type_id typeId, uint32_t memberIdx);

    QByteArray ir;
    QShaderDescription shaderDescription;

    spvc_context ctx = nullptr;
    spvc_compiler glslGen = nullptr;
    spvc_compiler hlslGen = nullptr;
    spvc_compiler mslGen = nullptr;

    QString spirvCrossErrorMsg;
};

QSpirvShaderPrivate::~QSpirvShaderPrivate()
{
    spvc_context_destroy(ctx);
}

void QSpirvShaderPrivate::createCompiler(spvc_backend backend)
{
    if (!ctx) {
        if (spvc_context_create(&ctx) != SPVC_SUCCESS) {
            qWarning("Failed to create SPIRV-Cross context");
            return;
        }
    }

    const SpvId *spirv = reinterpret_cast<const SpvId *>(ir.constData());
    size_t wordCount = ir.size() / sizeof(SpvId);
    spvc_parsed_ir parsedIr;
    if (spvc_context_parse_spirv(ctx, spirv, wordCount, &parsedIr) != SPVC_SUCCESS) {
        qWarning("Failed to parse SPIR-V: %s", spvc_context_get_last_error_string(ctx));
        return;
    }

    spvc_compiler *outCompiler = nullptr;
    switch (backend) {
    case SPVC_BACKEND_GLSL:
        outCompiler = &glslGen;
        break;
    case SPVC_BACKEND_HLSL:
        outCompiler = &hlslGen;
        break;
    case SPVC_BACKEND_MSL:
        outCompiler = &mslGen;
        break;
    default:
        return;
    }

    if (spvc_context_create_compiler(ctx, backend, parsedIr,
                                     SPVC_CAPTURE_MODE_TAKE_OWNERSHIP, outCompiler) != SPVC_SUCCESS)
    {
        qWarning("Failed to create SPIRV-Cross compiler: %s", spvc_context_get_last_error_string(ctx));
        return;
    }
}

static QShaderDescription::VariableType matVarType(const spvc_type &t, QShaderDescription::VariableType compType)
{
    const unsigned vecsize = spvc_type_get_vector_size(t);
    switch (spvc_type_get_columns(t)) {
    case 2:
        return QShaderDescription::VariableType(compType + 4 + (vecsize == 3 ? 1 : vecsize == 4 ? 2 : 0));
    case 3:
        return QShaderDescription::VariableType(compType + 7 + (vecsize == 2 ? 1 : vecsize == 4 ? 2 : 0));
    case 4:
        return QShaderDescription::VariableType(compType + 10 + (vecsize == 2 ? 1 : vecsize == 3 ? 2 : 0));
    default:
        return QShaderDescription::Unknown;
    }
}

static QShaderDescription::VariableType vecVarType(const spvc_type &t, QShaderDescription::VariableType compType)
{
    switch (spvc_type_get_vector_size(t)) {
    case 1:
        return compType;
    case 2:
        return QShaderDescription::VariableType(compType + 1);
    case 3:
        return QShaderDescription::VariableType(compType + 2);
    case 4:
        return QShaderDescription::VariableType(compType + 3);
    default:
        return QShaderDescription::Unknown;
    }
}

static QShaderDescription::VariableType sampledImageVarType(const spvc_type &t)
{
    switch (spvc_type_get_image_dimension(t)) {
    case SpvDim1D:
        return spvc_type_get_image_arrayed(t) ? QShaderDescription::Sampler1DArray
                                              : QShaderDescription::Sampler1D;
    case SpvDim2D:
        return spvc_type_get_image_arrayed(t)
                ? (spvc_type_get_image_multisampled(t) ? QShaderDescription::Sampler2DMSArray
                                                       : QShaderDescription::Sampler2DArray)
                : (spvc_type_get_image_multisampled(t) ? QShaderDescription::Sampler2DMS
                                                       : QShaderDescription::Sampler2D);
    case SpvDim3D:
        return spvc_type_get_image_arrayed(t) ? QShaderDescription::Sampler3DArray
                                              : QShaderDescription::Sampler3D;
    case SpvDimCube:
        return spvc_type_get_image_arrayed(t) ? QShaderDescription::SamplerCubeArray
                                              : QShaderDescription::SamplerCube;
    case SpvDimRect:
        return QShaderDescription::SamplerRect;
    case SpvDimBuffer:
        return QShaderDescription::SamplerBuffer;
    default:
        return QShaderDescription::Unknown;
    }
}

static QShaderDescription::VariableType imageVarType(const spvc_type &t)
{
    switch (spvc_type_get_image_dimension(t)) {
    case SpvDim1D:
        return spvc_type_get_image_arrayed(t) ? QShaderDescription::Image1DArray
                                              : QShaderDescription::Image1D;
    case SpvDim2D:
        return spvc_type_get_image_arrayed(t)
                ? (spvc_type_get_image_multisampled(t) ? QShaderDescription::Image2DMSArray
                                                       : QShaderDescription::Image2DArray)
                : (spvc_type_get_image_multisampled(t) ? QShaderDescription::Image2DMS
                                                       : QShaderDescription::Image2D);
    case SpvDim3D:
        return spvc_type_get_image_arrayed(t) ? QShaderDescription::Image3DArray
                                              : QShaderDescription::Image3D;
    case SpvDimCube:
        return spvc_type_get_image_arrayed(t) ? QShaderDescription::ImageCubeArray
                                              : QShaderDescription::ImageCube;
    case SpvDimRect:
        return QShaderDescription::ImageRect;
    case SpvDimBuffer:
        return QShaderDescription::ImageBuffer;
    default:
        return QShaderDescription::Unknown;
    }
}

static QShaderDescription::VariableType varType(const spvc_type &t)
{
    QShaderDescription::VariableType vt = QShaderDescription::Unknown;
    const spvc_basetype basetype = spvc_type_get_basetype(t);
    switch (basetype) {
    case SPVC_BASETYPE_FP32:
        vt = spvc_type_get_columns(t) > 1 ? matVarType(t, QShaderDescription::Float)
                                          : vecVarType(t, QShaderDescription::Float);
        break;
    case SPVC_BASETYPE_FP64:
        vt = spvc_type_get_columns(t) > 1 ? matVarType(t, QShaderDescription::Double)
                                          : vecVarType(t, QShaderDescription::Double);
        break;
    case SPVC_BASETYPE_UINT32:
        vt = vecVarType(t, QShaderDescription::Uint);
        break;
    case SPVC_BASETYPE_INT32:
        vt = vecVarType(t, QShaderDescription::Int);
        break;
    case SPVC_BASETYPE_BOOLEAN:
        vt = vecVarType(t, QShaderDescription::Uint);
        break;
    case SPVC_BASETYPE_SAMPLED_IMAGE:
        vt = sampledImageVarType(t);
        break;
    case SPVC_BASETYPE_IMAGE:
        vt = imageVarType(t);
        break;
    case SPVC_BASETYPE_STRUCT:
        vt = QShaderDescription::Struct;
        break;
    default:
        // can encounter types we do not (yet) handle, return Unknown for those
        qWarning("Unsupported base type %d", basetype);
        break;
    }
    return vt;
}

QShaderDescription::InOutVariable QSpirvShaderPrivate::inOutVar(const spvc_reflected_resource &r)
{
    QShaderDescription::InOutVariable v;
    v.name = r.name;

    spvc_type baseTypeHandle = spvc_compiler_get_type_handle(glslGen, r.base_type_id);
    v.type = varType(baseTypeHandle);

    spvc_type typeHandle = spvc_compiler_get_type_handle(glslGen, r.type_id);
    for (unsigned i = 0, dimCount = spvc_type_get_num_array_dimensions(typeHandle); i < dimCount; ++i)
        v.arrayDims.append(int(spvc_type_get_array_dimension(typeHandle, i)));

    if (spvc_compiler_has_decoration(glslGen, r.id, SpvDecorationLocation))
        v.location = spvc_compiler_get_decoration(glslGen, r.id, SpvDecorationLocation);

    if (spvc_compiler_has_decoration(glslGen, r.id, SpvDecorationBinding))
        v.binding = spvc_compiler_get_decoration(glslGen, r.id, SpvDecorationBinding);

    if (spvc_compiler_has_decoration(glslGen, r.id, SpvDecorationDescriptorSet))
        v.descriptorSet = spvc_compiler_get_decoration(glslGen, r.id, SpvDecorationDescriptorSet);

    if (spvc_type_get_basetype(baseTypeHandle) == SPVC_BASETYPE_IMAGE) {
        v.imageFormat = QShaderDescription::ImageFormat(spvc_type_get_image_storage_format(baseTypeHandle));

        // t.image.access is relevant for OpenCL kernels only so ignore.

        // No idea how to access the decorations like
        // DecorationNonReadable/Writable in a way that it returns the real
        // values (f.ex. has_decoration() on r.id or so is not functional). So
        // ignore these for now and pretend the image is read/write.
        v.imageFlags = {};
    }

    return v;
}

QShaderDescription::BlockVariable QSpirvShaderPrivate::blockVar(spvc_type_id typeId, uint32_t memberIdx)
{
    QShaderDescription::BlockVariable v;
    v.name = spvc_compiler_get_member_name(glslGen, typeId, memberIdx);

    spvc_type t = spvc_compiler_get_type_handle(glslGen, typeId);
    spvc_type_id memberTypeId = spvc_type_get_member_type(t, memberIdx);
    spvc_type memberType = spvc_compiler_get_type_handle(glslGen, memberTypeId);
    v.type = varType(memberType);

    unsigned offset = 0;
    if (spvc_compiler_type_struct_member_offset(glslGen, t, memberIdx, &offset) == SPVC_SUCCESS)
        v.offset = int(offset);

    size_t size = 0;
    if (spvc_compiler_get_declared_struct_member_size(glslGen, t, memberIdx, &size) == SPVC_SUCCESS)
        v.size = int(size);

    for (unsigned i = 0, dimCount = spvc_type_get_num_array_dimensions(memberType); i < dimCount; ++i)
        v.arrayDims.append(int(spvc_type_get_array_dimension(memberType, i)));

    if (spvc_compiler_has_member_decoration(glslGen, typeId, memberIdx, SpvDecorationArrayStride)) {
        unsigned stride = 0;
        if (spvc_compiler_type_struct_member_array_stride(glslGen, t, memberIdx, &stride) == SPVC_SUCCESS)
            v.arrayStride = int(stride);
    }

    if (spvc_compiler_has_member_decoration(glslGen, typeId, memberIdx, SpvDecorationMatrixStride)) {
        unsigned stride = 0;
        if (spvc_compiler_type_struct_member_matrix_stride(glslGen, t, memberIdx, &stride) == SPVC_SUCCESS)
            v.matrixStride = int(stride);
    }

    if (spvc_compiler_has_member_decoration(glslGen, typeId, memberIdx, SpvDecorationRowMajor))
        v.matrixIsRowMajor = true;

    if (v.type == QShaderDescription::Struct) {
        unsigned count = spvc_type_get_num_member_types(memberType);
        for (unsigned idx = 0; idx < count; ++idx)
            v.structMembers.append(blockVar(spvc_type_get_base_type_id(memberType), idx));
    }

    return v;
}

void QSpirvShaderPrivate::processResources()
{
    if (!glslGen)
        return;

    shaderDescription = QShaderDescription();
    QShaderDescriptionPrivate *dd = QShaderDescriptionPrivate::get(&shaderDescription);

    dd->localSize[0] = spvc_compiler_get_execution_mode_argument_by_index(glslGen, SpvExecutionModeLocalSize, 0);
    dd->localSize[1] = spvc_compiler_get_execution_mode_argument_by_index(glslGen, SpvExecutionModeLocalSize, 1);
    dd->localSize[2] = spvc_compiler_get_execution_mode_argument_by_index(glslGen, SpvExecutionModeLocalSize, 2);

    spvc_resources resources;
    if (spvc_compiler_create_shader_resources(glslGen, &resources) != SPVC_SUCCESS) {
        qWarning("Failed to get shader resources: %s", spvc_context_get_last_error_string(ctx));
        return;
    }

    const spvc_reflected_resource *resourceList = nullptr;
    size_t resourceListCount = 0;

    if (spvc_resources_get_resource_list_for_type(resources, SPVC_RESOURCE_TYPE_STAGE_INPUT,
                                                  &resourceList, &resourceListCount) == SPVC_SUCCESS)
    {
        for (size_t i = 0; i < resourceListCount; ++i) {
            const QShaderDescription::InOutVariable v = inOutVar(resourceList[i]);
            if (v.type != QShaderDescription::Unknown)
                dd->inVars.append(v);
        }
    }

    if (spvc_resources_get_resource_list_for_type(resources, SPVC_RESOURCE_TYPE_STAGE_OUTPUT,
                                                  &resourceList, &resourceListCount) == SPVC_SUCCESS)
    {
        for (size_t i = 0; i < resourceListCount; ++i) {
            const QShaderDescription::InOutVariable v = inOutVar(resourceList[i]);
            if (v.type != QShaderDescription::Unknown)
                dd->outVars.append(v);
        }
    }

    // uniform blocks map to either a uniform buffer or a plain struct
    if (spvc_resources_get_resource_list_for_type(resources, SPVC_RESOURCE_TYPE_UNIFORM_BUFFER,
                                                  &resourceList, &resourceListCount) == SPVC_SUCCESS)
    {
        for (size_t i = 0; i < resourceListCount; ++i) {
            const spvc_reflected_resource &r(resourceList[i]);
            spvc_type t = spvc_compiler_get_type_handle(glslGen, r.base_type_id);
            QShaderDescription::UniformBlock block;
            block.blockName = r.name;

            // the simple case:
            //     layout(...) uniform blk { T v; } inst;
            // gives us blockName "blk" and structName "inst" because
            // in GLSL without uniform blocks this ends up being
            //     struct blk { T v; }; uniform blk inst;
            // or, where real UBs are used, for instance Metal:
            //   struct blk { T v; }; constant blk& inst [[buffer(N)]]
            block.structName = spvc_compiler_get_name(glslGen, r.id);

            // the annoying case:
            //     layout(...) uniform blk { T v; };
            // here structName is empty and the generated code (when no uniform
            // blocks) uses _ID as a fallback name:
            //     struct blk { T v; }; uniform blk _ID;
            // or, with real uniform buffers, f.ex. Metal:
            //     struct blk { T v; }; constant blk& _ID [[buffer(N)]]
            // Let's make sure the fallback name is filled in correctly.
            if (block.structName.isEmpty()) {
                // The catch (matters only when uniform blocks are not used in
                // the output) is that ID is per-shader and so may differ
                // between shaders, meaning that having the same uniform block
                // in a vertex and fragment shader will lead to each stage
                // having its own set of uniforms corresponding to the ub
                // members (ofc, inactive uniforms may get optimized out by the
                // compiler, so the duplication then is only there for members
                // used in both stages). Not much we can do about that here,
                // though. The GL backend of QRhi can deal with this.
                block.structName = QByteArrayLiteral("_") + QByteArray::number(r.id);
            }

            size_t size = 0;
            spvc_compiler_get_declared_struct_size(glslGen, t, &size);
            block.size = int(size);
            if (spvc_compiler_has_decoration(glslGen, r.id, SpvDecorationBinding))
                block.binding = int(spvc_compiler_get_decoration(glslGen, r.id, SpvDecorationBinding));
            if (spvc_compiler_has_decoration(glslGen, r.id, SpvDecorationDescriptorSet))
                block.descriptorSet = int(spvc_compiler_get_decoration(glslGen, r.id, SpvDecorationDescriptorSet));

            unsigned count = spvc_type_get_num_member_types(t);
            for (unsigned idx = 0; idx < count; ++idx) {
                const QShaderDescription::BlockVariable v = blockVar(r.base_type_id, idx);
                if (v.type != QShaderDescription::Unknown)
                    block.members.append(v);
            }

            dd->uniformBlocks.append(block);
        }
    }

    // push constant blocks map to a plain GLSL struct regardless of version
    if (spvc_resources_get_resource_list_for_type(resources, SPVC_RESOURCE_TYPE_PUSH_CONSTANT,
                                                  &resourceList, &resourceListCount) == SPVC_SUCCESS)
    {
        for (size_t i = 0; i < resourceListCount; ++i) {
            const spvc_reflected_resource &r(resourceList[i]);
            spvc_type t = spvc_compiler_get_type_handle(glslGen, r.base_type_id);
            QShaderDescription::PushConstantBlock block;
            block.name = spvc_compiler_get_name(glslGen, r.id);
            size_t size = 0;
            spvc_compiler_get_declared_struct_size(glslGen, t, &size);
            block.size = int(size);
            unsigned count = spvc_type_get_num_member_types(t);
            for (unsigned idx = 0; idx < count; ++idx) {
                const QShaderDescription::BlockVariable v = blockVar(r.base_type_id, idx);
                if (v.type != QShaderDescription::Unknown)
                    block.members.append(v);
            }
            dd->pushConstantBlocks.append(block);
        }
    }

    if (spvc_resources_get_resource_list_for_type(resources, SPVC_RESOURCE_TYPE_STORAGE_BUFFER,
                                                  &resourceList, &resourceListCount) == SPVC_SUCCESS)
    {
        for (size_t i = 0; i < resourceListCount; ++i) {
            const spvc_reflected_resource &r(resourceList[i]);
            spvc_type t = spvc_compiler_get_type_handle(glslGen, r.base_type_id);
            QShaderDescription::StorageBlock block;
            block.blockName = r.name;
            block.instanceName = spvc_compiler_get_name(glslGen, r.id);
            size_t size = 0;
            spvc_compiler_get_declared_struct_size(glslGen, t, &size);
            block.knownSize = int(size);
            if (spvc_compiler_has_decoration(glslGen, r.id, SpvDecorationBinding))
                block.binding = int(spvc_compiler_get_decoration(glslGen, r.id, SpvDecorationBinding));
            if (spvc_compiler_has_decoration(glslGen, r.id, SpvDecorationDescriptorSet))
                block.descriptorSet = int(spvc_compiler_get_decoration(glslGen, r.id, SpvDecorationDescriptorSet));
            unsigned count = spvc_type_get_num_member_types(t);
            for (unsigned idx = 0; idx < count; ++idx) {
                const QShaderDescription::BlockVariable v = blockVar(r.base_type_id, idx);
                if (v.type != QShaderDescription::Unknown)
                    block.members.append(v);
            }
            dd->storageBlocks.append(block);
        }
    }

    if (spvc_resources_get_resource_list_for_type(resources, SPVC_RESOURCE_TYPE_SAMPLED_IMAGE,
                                                  &resourceList, &resourceListCount) == SPVC_SUCCESS)
    {
        for (size_t i = 0; i < resourceListCount; ++i) {
            const spvc_reflected_resource &r(resourceList[i]);
            const QShaderDescription::InOutVariable v = inOutVar(r);
            if (v.type != QShaderDescription::Unknown)
                dd->combinedImageSamplers.append(v);
        }
    }

    if (spvc_resources_get_resource_list_for_type(resources, SPVC_RESOURCE_TYPE_STORAGE_IMAGE,
                                                  &resourceList, &resourceListCount) == SPVC_SUCCESS)
    {
        for (size_t i = 0; i < resourceListCount; ++i) {
            const spvc_reflected_resource &r(resourceList[i]);
            const QShaderDescription::InOutVariable v = inOutVar(r);
            if (v.type != QShaderDescription::Unknown)
                dd->storageImages.append(v);
        }
    }
}

QSpirvShader::QSpirvShader()
    : d(new QSpirvShaderPrivate)
{
}

QSpirvShader::~QSpirvShader()
{
    delete d;
}

void QSpirvShader::setFileName(const QString &fileName)
{
    QFile f(fileName);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning("QSpirvShader: Failed to open %s", qPrintable(fileName));
        return;
    }
    setDevice(&f);
}

void QSpirvShader::setDevice(QIODevice *device)
{
    d->ir = device->readAll();
    d->createCompiler(SPVC_BACKEND_GLSL);
    d->processResources();
}

void QSpirvShader::setSpirvBinary(const QByteArray &spirv)
{
    d->ir = spirv;
    d->createCompiler(SPVC_BACKEND_GLSL);
    d->processResources();
}

QShaderDescription QSpirvShader::shaderDescription() const
{
    return d->shaderDescription;
}

QByteArray QSpirvShader::spirvBinary() const
{
    return d->ir;
}

QByteArray QSpirvShader::remappedSpirvBinary(RemapFlags flags, QString *errorMessage) const
{
    QSpirvShaderRemapper remapper;
    QByteArray result = remapper.remap(d->ir, flags);
    if (errorMessage)
        *errorMessage = remapper.errorMessage();
    return result;
}

QByteArray QSpirvShader::translateToGLSL(int version, GlslFlags flags) const
{
    d->spirvCrossErrorMsg.clear();

    d->createCompiler(SPVC_BACKEND_GLSL);
    if (!d->glslGen)
        return QByteArray();

    spvc_compiler_options options = nullptr;
    if (spvc_compiler_create_compiler_options(d->glslGen, &options) != SPVC_SUCCESS)
        return QByteArray();
    spvc_compiler_options_set_uint(options, SPVC_COMPILER_OPTION_GLSL_VERSION,
                                   version);
    spvc_compiler_options_set_bool(options, SPVC_COMPILER_OPTION_GLSL_ES,
                                   flags.testFlag(GlslEs));
    spvc_compiler_options_set_bool(options, SPVC_COMPILER_OPTION_FIXUP_DEPTH_CONVENTION,
                                   flags.testFlag(FixClipSpace));
    spvc_compiler_options_set_bool(options, SPVC_COMPILER_OPTION_GLSL_ES_DEFAULT_FLOAT_PRECISION_HIGHP,
                                   !flags.testFlag(FragDefaultMediump));
    // The gl backend of QRhi is not prepared for UBOs atm. Have a uniform (heh)
    // behavior regardless of the GLSL version.
    spvc_compiler_options_set_bool(options, SPVC_COMPILER_OPTION_GLSL_EMIT_UNIFORM_BUFFER_AS_PLAIN_UNIFORMS,
                                   true);
    // Do not emit binding qualifiers for samplers (and for uniform blocks, but
    // those we just disabled above).
    spvc_compiler_options_set_bool(options, SPVC_COMPILER_OPTION_GLSL_ENABLE_420PACK_EXTENSION,
                                   false);
    spvc_compiler_install_compiler_options(d->glslGen, options);

    const char *result = nullptr;
    if (spvc_compiler_compile(d->glslGen, &result) != SPVC_SUCCESS) {
        d->spirvCrossErrorMsg = QString::fromUtf8(spvc_context_get_last_error_string(d->ctx));
        return QByteArray();
    }

    // We used to fix up the result to complement GL_ARB_shading_language_420pack
    // with GL_ARB_separate_shader_objects to make Mesa happy, but the 420pack
    // is never relied on now so no need to do anything here.
    return result;
}

QByteArray QSpirvShader::translateToHLSL(int version, QShader::NativeResourceBindingMap *nativeBindings) const
{
    d->spirvCrossErrorMsg.clear();

    d->createCompiler(SPVC_BACKEND_HLSL);
    if (!d->hlslGen)
        return QByteArray();

    spvc_compiler_options options = nullptr;
    if (spvc_compiler_create_compiler_options(d->hlslGen, &options) != SPVC_SUCCESS)
        return QByteArray();
    spvc_compiler_options_set_uint(options, SPVC_COMPILER_OPTION_HLSL_SHADER_MODEL,
                                   version);
    spvc_compiler_options_set_bool(options, SPVC_COMPILER_OPTION_HLSL_POINT_SIZE_COMPAT,
                                   true);
    spvc_compiler_options_set_bool(options, SPVC_COMPILER_OPTION_HLSL_POINT_COORD_COMPAT,
                                   true);
    spvc_compiler_install_compiler_options(d->hlslGen, options);

    // D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT is 16, so we cannot have combined
    // image/sampler bindings above 15. Problem is, the (SPIR-V) binding
    // namespace is common with uniform buffers and other resources, and so
    // having many uniform buffers and samplers can push the sampler binding to
    // above 15 even when there are less than 16 samplers in total. So cannot
    // just rely on SPIRV-Cross setting registers t<binding> and s<binding> for
    // the SRV and sampler for each combined image/sampler. Instead, we want
    // t<slot> and s<slot> where slot is 0-based, with its own namespace, and
    // is then exposed in the native resource binding map so QRhi can map
    // between (SPIR-V) bindings and D3D sampler slots.
    const SpvExecutionModel stage = spvc_compiler_get_execution_model(d->hlslGen);
    int regBinding = 0; // SRVs and samplers
    for (const QShaderDescription::InOutVariable &var : d->shaderDescription.combinedImageSamplers()) {
        spvc_hlsl_resource_binding bindingMapping;
        bindingMapping.stage = stage; // will be per-stage but we have a per-shader NativeResourceBindingMap so it's ok
        bindingMapping.desc_set = 0;
        bindingMapping.binding = var.binding;
        bindingMapping.srv.register_space = 0;
        bindingMapping.srv.register_binding = regBinding; // t0, t1, ...
        bindingMapping.sampler.register_space = 0;
        bindingMapping.sampler.register_binding = regBinding; // s0, s1, ...
        spvc_compiler_hlsl_add_resource_binding(d->hlslGen, &bindingMapping);
        nativeBindings->insert(var.binding, { regBinding, regBinding });
        regBinding += var.arrayDims.isEmpty() ? 1 : var.arrayDims.first();
    }

    regBinding = 0; // CBVs
    for (const QShaderDescription::UniformBlock &blk : d->shaderDescription.uniformBlocks()) {
        spvc_hlsl_resource_binding bindingMapping;
        bindingMapping.stage = stage;
        bindingMapping.desc_set = 0;
        bindingMapping.binding = blk.binding;
        bindingMapping.cbv.register_space = 0;
        bindingMapping.cbv.register_binding = regBinding; // b0, b1, ...
        spvc_compiler_hlsl_add_resource_binding(d->hlslGen, &bindingMapping);
        nativeBindings->insert(blk.binding, { regBinding, -1 });
        regBinding += 1;
    }

    regBinding = 0; // UAVs
    for (const QShaderDescription::StorageBlock &blk : d->shaderDescription.storageBlocks()) {
        spvc_hlsl_resource_binding bindingMapping;
        bindingMapping.stage = stage;
        bindingMapping.desc_set = 0;
        bindingMapping.binding = blk.binding;
        bindingMapping.uav.register_space = 0;
        bindingMapping.uav.register_binding = regBinding; // u0, u1, ...
        spvc_compiler_hlsl_add_resource_binding(d->hlslGen, &bindingMapping);
        nativeBindings->insert(blk.binding, { regBinding, -1 });
        regBinding += 1;
    }
    for (const QShaderDescription::InOutVariable &var : d->shaderDescription.storageImages()) {
        spvc_hlsl_resource_binding bindingMapping;
        bindingMapping.stage = stage;
        bindingMapping.desc_set = 0;
        bindingMapping.binding = var.binding;
        bindingMapping.uav.register_space = 0;
        bindingMapping.uav.register_binding = regBinding; // u0, u1, ...
        spvc_compiler_hlsl_add_resource_binding(d->hlslGen, &bindingMapping);
        nativeBindings->insert(var.binding, { regBinding, -1 });
        regBinding += 1;
    }

    const char *result = nullptr;
    if (spvc_compiler_compile(d->hlslGen, &result) != SPVC_SUCCESS) {
        d->spirvCrossErrorMsg = QString::fromUtf8(spvc_context_get_last_error_string(d->ctx));
        return QByteArray();
    }

    return QByteArray(result);
}

QByteArray QSpirvShader::translateToMSL(int version, QShader::NativeResourceBindingMap *nativeBindings) const
{
    d->spirvCrossErrorMsg.clear();

    d->createCompiler(SPVC_BACKEND_MSL);
    if (!d->mslGen)
        return QByteArray();

    spvc_compiler_options options = nullptr;
    if (spvc_compiler_create_compiler_options(d->mslGen, &options) != SPVC_SUCCESS)
        return QByteArray();
    spvc_compiler_options_set_uint(options, SPVC_COMPILER_OPTION_MSL_VERSION,
                                   SPVC_MAKE_MSL_VERSION(version / 10, version % 10, 0));
    // leave platform set to macOS, it won't matter in practice (hopefully)
    spvc_compiler_install_compiler_options(d->mslGen, options);

    const char *result = nullptr;
    if (spvc_compiler_compile(d->mslGen, &result) != SPVC_SUCCESS) {
        d->spirvCrossErrorMsg = QString::fromUtf8(spvc_context_get_last_error_string(d->ctx));
        return QByteArray();
    }

    if (nativeBindings) {
        spvc_resources resources;
        if (spvc_compiler_create_shader_resources(d->mslGen, &resources) == SPVC_SUCCESS) {
            const spvc_reflected_resource *resourceList = nullptr;
            size_t resourceListCount = 0;
            // A nativeBinding of -1 means unused. This also fits
            // *_get_automatic_resource_binding() which returns uint32_t(-1)
            // when there is no binding, which can happen when the uniform
            // block, sampler, etc. is not actively used in the shader. The map
            // must always be complete, including a (binding -> -1) mapping for
            // inactive resources as well. The second value of the pair is only
            // relevant for combined image samplers, and is -1 otherwise.
            if (spvc_resources_get_resource_list_for_type(resources, SPVC_RESOURCE_TYPE_UNIFORM_BUFFER,
                                                          &resourceList, &resourceListCount) == SPVC_SUCCESS)
            {
                for (size_t i = 0; i < resourceListCount; ++i) {
                    unsigned binding = spvc_compiler_get_decoration(d->mslGen, resourceList[i].id, SpvDecorationBinding);
                    unsigned nativeBinding = spvc_compiler_msl_get_automatic_resource_binding(d->mslGen, resourceList[i].id);
                    nativeBindings->insert(int(binding), { int(nativeBinding), -1 });
                }
            }
            if (spvc_resources_get_resource_list_for_type(resources, SPVC_RESOURCE_TYPE_STORAGE_BUFFER,
                                                          &resourceList, &resourceListCount) == SPVC_SUCCESS)
            {
                for (size_t i = 0; i < resourceListCount; ++i) {
                    unsigned binding = spvc_compiler_get_decoration(d->mslGen, resourceList[i].id, SpvDecorationBinding);
                    unsigned nativeBinding = spvc_compiler_msl_get_automatic_resource_binding(d->mslGen, resourceList[i].id);
                    nativeBindings->insert(int(binding), { int(nativeBinding), -1 });
                }
            }
            if (spvc_resources_get_resource_list_for_type(resources, SPVC_RESOURCE_TYPE_SAMPLED_IMAGE,
                                                          &resourceList, &resourceListCount) == SPVC_SUCCESS)
            {
                for (size_t i = 0; i < resourceListCount; ++i) {
                    unsigned binding = spvc_compiler_get_decoration(d->mslGen, resourceList[i].id, SpvDecorationBinding);
                    unsigned nativeTextureBinding = spvc_compiler_msl_get_automatic_resource_binding(d->mslGen, resourceList[i].id);
                    unsigned nativeSamplerBinding = spvc_compiler_msl_get_automatic_resource_binding_secondary(d->mslGen, resourceList[i].id);
                    nativeBindings->insert(int(binding), { int(nativeTextureBinding), int(nativeSamplerBinding) });
                }
            }
            if (spvc_resources_get_resource_list_for_type(resources, SPVC_RESOURCE_TYPE_STORAGE_IMAGE,
                                                          &resourceList, &resourceListCount) == SPVC_SUCCESS)
            {
                for (size_t i = 0; i < resourceListCount; ++i) {
                    unsigned binding = spvc_compiler_get_decoration(d->mslGen, resourceList[i].id, SpvDecorationBinding);
                    unsigned nativeBinding = spvc_compiler_msl_get_automatic_resource_binding(d->mslGen, resourceList[i].id);
                    nativeBindings->insert(int(binding), { int(nativeBinding), -1 });
                }
            }
        }
    }

    return QByteArray(result);
}

QString QSpirvShader::translationErrorMessage() const
{
    return d->spirvCrossErrorMsg;
}

QT_END_NAMESPACE
