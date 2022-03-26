/****************************************************************************
**
** Copyright (C) 2008-2012 NVIDIA Corporation.
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
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

/* clang-format off */

#include <QtQuick3DRuntimeRender/private/qssgshadermaterialadapter_p.h>
#include <QtQuick3DRuntimeRender/private/qssgrendercontextcore_p.h>

QT_BEGIN_NAMESPACE

QSSGShaderMaterialAdapter::~QSSGShaderMaterialAdapter() = default;

QSSGShaderMaterialAdapter *QSSGShaderMaterialAdapter::create(const QSSGRenderGraphObject &materialNode)
{
    switch (materialNode.type) {
    case QSSGRenderGraphObject::Type::DefaultMaterial:
    case QSSGRenderGraphObject::Type::PrincipledMaterial:
        return new QSSGShaderDefaultMaterialAdapter(static_cast<const QSSGRenderDefaultMaterial &>(materialNode));

    case QSSGRenderGraphObject::Type::CustomMaterial:
        return new QSSGShaderCustomMaterialAdapter(static_cast<const QSSGRenderCustomMaterial &>(materialNode));

    default:
        break;
    }

    return nullptr;
}

bool QSSGShaderMaterialAdapter::isUnshaded()
{
    return false;
}

bool QSSGShaderMaterialAdapter::hasCustomShaderSnippet(QSSGShaderCache::ShaderType)
{
    return false;
}

QByteArray QSSGShaderMaterialAdapter::customShaderSnippet(QSSGShaderCache::ShaderType,
                                                          const QSSGRef<QSSGShaderLibraryManager> &)
{
    return QByteArray();
}

bool QSSGShaderMaterialAdapter::hasCustomShaderFunction(QSSGShaderCache::ShaderType,
                                                        const QByteArray &,
                                                        const QSSGRef<QSSGShaderLibraryManager> &)
{
    return false;
}

void QSSGShaderMaterialAdapter::setCustomPropertyUniforms(char *,
                                                          QSSGRef<QSSGRhiShaderPipeline> &,
                                                          const QSSGRenderContextInterface &)
{
}

bool QSSGShaderMaterialAdapter::usesSharedVariables()
{
    return false;
}



QSSGShaderDefaultMaterialAdapter::QSSGShaderDefaultMaterialAdapter(const QSSGRenderDefaultMaterial &material)
    : m_material(material)
{
}

bool QSSGShaderDefaultMaterialAdapter::isPrincipled()
{
    return m_material.type == QSSGRenderGraphObject::Type::PrincipledMaterial;
}

bool QSSGShaderDefaultMaterialAdapter::isMetalnessEnabled()
{
    return m_material.isMetalnessEnabled();
}

bool QSSGShaderDefaultMaterialAdapter::isSpecularEnabled()
{
    return m_material.isSpecularEnabled();
}

bool QSSGShaderDefaultMaterialAdapter::isVertexColorsEnabled()
{
    return m_material.isVertexColorsEnabled();
}

bool QSSGShaderDefaultMaterialAdapter::hasLighting()
{
    return m_material.hasLighting();
}

QSSGRenderDefaultMaterial::MaterialSpecularModel QSSGShaderDefaultMaterialAdapter::specularModel()
{
    return m_material.specularModel;
}

QSSGRenderDefaultMaterial::MaterialAlphaMode QSSGShaderDefaultMaterialAdapter::alphaMode()
{
    return m_material.alphaMode;
}

QSSGRenderImage *QSSGShaderDefaultMaterialAdapter::iblProbe()
{
    return m_material.iblProbe;
}

QVector3D QSSGShaderDefaultMaterialAdapter::emissiveColor()
{
    return m_material.emissiveColor;
}

QVector4D QSSGShaderDefaultMaterialAdapter::color()
{
    return m_material.color;
}

QVector3D QSSGShaderDefaultMaterialAdapter::specularTint()
{
    return m_material.specularTint;
}

float QSSGShaderDefaultMaterialAdapter::ior()
{
    return m_material.ior;
}

float QSSGShaderDefaultMaterialAdapter::fresnelPower()
{
    return m_material.fresnelPower;
}

float QSSGShaderDefaultMaterialAdapter::metalnessAmount()
{
    return m_material.metalnessAmount;
}

float QSSGShaderDefaultMaterialAdapter::specularAmount()
{
    return m_material.specularAmount;
}

float QSSGShaderDefaultMaterialAdapter::specularRoughness()
{
    return m_material.specularRoughness;
}

float QSSGShaderDefaultMaterialAdapter::bumpAmount()
{
    return m_material.bumpAmount;
}

float QSSGShaderDefaultMaterialAdapter::translucentFallOff()
{
    return m_material.translucentFalloff;
}

float QSSGShaderDefaultMaterialAdapter::diffuseLightWrap()
{
    return m_material.diffuseLightWrap;
}

float QSSGShaderDefaultMaterialAdapter::occlusionAmount()
{
    return m_material.occlusionAmount;
}

float QSSGShaderDefaultMaterialAdapter::alphaCutOff()
{
    return m_material.alphaCutoff;
}

float QSSGShaderDefaultMaterialAdapter::pointSize()
{
    return m_material.pointSize;
}

float QSSGShaderDefaultMaterialAdapter::lineWidth()
{
    return m_material.lineWidth;
}

float QSSGShaderDefaultMaterialAdapter::heightAmount()
{
    return m_material.heightAmount;
}

float QSSGShaderDefaultMaterialAdapter::minHeightSamples()
{
    return m_material.minHeightSamples;
}

float QSSGShaderDefaultMaterialAdapter::maxHeightSamples()
{
    return m_material.maxHeightSamples;
}



QSSGShaderCustomMaterialAdapter::QSSGShaderCustomMaterialAdapter(const QSSGRenderCustomMaterial &material)
    : m_material(material)
{
}

// Act like Principled. Lighting is always on, specular, metalness, etc. support should all be enabled.
// Unlike Principled, the *enabled values do not depend on the metalness or specularAmount values
// (we cannot tell what those are if they are written in the shader).

bool QSSGShaderCustomMaterialAdapter::isPrincipled()
{
    return true;
}

bool QSSGShaderCustomMaterialAdapter::isMetalnessEnabled()
{
    return true;
}

bool QSSGShaderCustomMaterialAdapter::isSpecularEnabled()
{
    return true;
}

bool QSSGShaderCustomMaterialAdapter::isVertexColorsEnabled()
{
    // qt_varColor must always be present. Works also if the mesh does not have
    // colors, it will assume vec4(1.0).
    return true;
}

bool QSSGShaderCustomMaterialAdapter::hasLighting()
{
    return true;
}

QSSGRenderDefaultMaterial::MaterialSpecularModel QSSGShaderCustomMaterialAdapter::specularModel()
{
    return QSSGRenderDefaultMaterial::MaterialSpecularModel::Default;
}

QSSGRenderDefaultMaterial::MaterialAlphaMode QSSGShaderCustomMaterialAdapter::alphaMode()
{
    return QSSGRenderDefaultMaterial::MaterialAlphaMode::Default;
}

QSSGRenderImage *QSSGShaderCustomMaterialAdapter::iblProbe()
{
    return m_material.m_iblProbe;
}

// The following are the values that get set into uniforms such as
// qt_material_properties etc. When a custom shader is present, these values
// are not used at all. However, a CustomMaterial is also valid without a
// vertex/fragment shader, or with no custom shaders at all. Therefore the
// values here must match the defaults of PrincipledMaterial, in order to make
// PrincipledMaterial { } and CustomMaterial { } identical.

QVector3D QSSGShaderCustomMaterialAdapter::emissiveColor()
{
    return QVector3D(0, 0, 0);
}

QVector4D QSSGShaderCustomMaterialAdapter::color()
{
    return QVector4D(1, 1, 1, 1);
}

QVector3D QSSGShaderCustomMaterialAdapter::specularTint()
{
    return QVector3D(1, 1, 1);
}

float QSSGShaderCustomMaterialAdapter::ior()
{
    return 1.45f;
}

float QSSGShaderCustomMaterialAdapter::fresnelPower()
{
    return 0.0f;
}

float QSSGShaderCustomMaterialAdapter::metalnessAmount()
{
    return 0.0f;
}

float QSSGShaderCustomMaterialAdapter::specularAmount()
{
    return 0.5f;
}

float QSSGShaderCustomMaterialAdapter::specularRoughness()
{
    return 0.0f;
}

float QSSGShaderCustomMaterialAdapter::bumpAmount()
{
    return 0.0f;
}

float QSSGShaderCustomMaterialAdapter::translucentFallOff()
{
    return 0.0f;
}

float QSSGShaderCustomMaterialAdapter::diffuseLightWrap()
{
    return 0.0f;
}

float QSSGShaderCustomMaterialAdapter::occlusionAmount()
{
    return 1.0f;
}

float QSSGShaderCustomMaterialAdapter::alphaCutOff()
{
    return 0.5f;
}

float QSSGShaderCustomMaterialAdapter::pointSize()
{
    return 1.0f;
}

float QSSGShaderCustomMaterialAdapter::lineWidth()
{
    return m_material.m_lineWidth;
}

float QSSGShaderCustomMaterialAdapter::heightAmount()
{
    return 0.0f;
}

float QSSGShaderCustomMaterialAdapter::minHeightSamples()
{
    return 0.0f;
}

float QSSGShaderCustomMaterialAdapter::maxHeightSamples()
{
    return 0.0f;
}

bool QSSGShaderCustomMaterialAdapter::isUnshaded()
{
    return m_material.m_shadingMode == QSSGRenderCustomMaterial::ShadingMode::Unshaded;
}

bool QSSGShaderCustomMaterialAdapter::hasCustomShaderSnippet(QSSGShaderCache::ShaderType type)
{
    if (type == QSSGShaderCache::ShaderType::Vertex)
        return m_material.m_customShaderPresence.testFlag(QSSGRenderCustomMaterial::CustomShaderPresenceFlag::Vertex);

    return m_material.m_customShaderPresence.testFlag(QSSGRenderCustomMaterial::CustomShaderPresenceFlag::Fragment);
}

QByteArray QSSGShaderCustomMaterialAdapter::customShaderSnippet(QSSGShaderCache::ShaderType type,
                                                                const QSSGRef<QSSGShaderLibraryManager> &shaderLibraryManager)
{
    if (hasCustomShaderSnippet(type))
        return shaderLibraryManager->getShaderSource(m_material.m_shaderPathKey, type);

    return QByteArray();
}

bool QSSGShaderCustomMaterialAdapter::hasCustomShaderFunction(QSSGShaderCache::ShaderType shaderType,
                                                              const QByteArray &funcName,
                                                              const QSSGRef<QSSGShaderLibraryManager> &shaderLibraryManager)
{
    if (hasCustomShaderSnippet(shaderType))
        return shaderLibraryManager->getShaderMetaData(m_material.m_shaderPathKey, shaderType).customFunctions.contains(funcName);

    return false;
}

void QSSGShaderCustomMaterialAdapter::setCustomPropertyUniforms(char *ubufData,
                                                                QSSGRef<QSSGRhiShaderPipeline> &shaderPipeline,
                                                                const QSSGRenderContextInterface &context)
{
    context.customMaterialSystem()->applyRhiShaderPropertyValues(ubufData, m_material, shaderPipeline);
}

bool QSSGShaderCustomMaterialAdapter::usesSharedVariables()
{
    return m_material.m_usesSharedVariables;
}

namespace {

// Custom material shader substitution table.
// Must be in sync with the shader generator.
static const QSSGCustomMaterialVariableSubstitution qssg_var_subst_tab[] = {
    // uniform (block members)
    { "MODELVIEWPROJECTION_MATRIX", "qt_modelViewProjection" },
    { "VIEWPROJECTION_MATRIX", "qt_viewProjectionMatrix" },
    { "MODEL_MATRIX", "qt_modelMatrix" },
    { "VIEW_MATRIX", "qt_viewMatrix" },
    { "NORMAL_MATRIX", "qt_normalMatrix"},
    { "BONE_TRANSFORMS", "qt_boneTransforms" },
    { "BONE_NORMAL_TRANSFORMS", "qt_boneNormalTransforms" },
    { "PROJECTION_MATRIX", "qt_projectionMatrix" },
    { "INVERSE_PROJECTION_MATRIX", "qt_inverseProjectionMatrix" },
    { "CAMERA_POSITION", "qt_cameraPosition" },
    { "CAMERA_DIRECTION", "qt_cameraDirection" },
    { "CAMERA_PROPERTIES", "qt_cameraProperties" },
    { "FRAMEBUFFER_Y_UP", "qt_rhi_properties.x" },
    { "NDC_Y_UP", "qt_rhi_properties.y" },
    { "NEAR_CLIP_VALUE", "qt_rhi_properties.z" },

    // outputs
    { "POSITION", "gl_Position" },
    { "FRAGCOLOR", "fragOutput" },
    { "POINT_SIZE", "gl_PointSize" },

    // fragment inputs
    { "FRAGCOORD", "gl_FragCoord"},

    // functions
    { "DIRECTIONAL_LIGHT", "qt_directionalLightProcessor" },
    { "POINT_LIGHT", "qt_pointLightProcessor" },
    { "SPOT_LIGHT", "qt_spotLightProcessor" },
    { "AMBIENT_LIGHT", "qt_ambientLightProcessor" },
    { "SPECULAR_LIGHT", "qt_specularLightProcessor" },
    { "MAIN", "qt_customMain" },
    { "POST_PROCESS", "qt_customPostProcessor" },

    // textures
    { "SCREEN_TEXTURE", "qt_screenTexture" },
    { "SCREEN_MIP_TEXTURE", "qt_screenTexture" }, // same resource as SCREEN_TEXTURE under the hood
    { "DEPTH_TEXTURE", "qt_depthTexture" },
    { "AO_TEXTURE", "qt_aoTexture" },

    // For shaded only: vertex outputs, for convenience and perf. (only those
    // that are always present when lighting is enabled) The custom vertex main
    // can also calculate on its own and pass them on with VARYING but that's a
    // bit wasteful since we calculate these anyways.
    { "VAR_WORLD_NORMAL", "qt_varNormal" },
    { "VAR_WORLD_TANGENT", "qt_varTangent" },
    { "VAR_WORLD_BINORMAL", "qt_varBinormal" },
    { "VAR_WORLD_POSITION", "qt_varWorldPos" },
    // vertex color is always enabled for custom materials (shaded)
    { "VAR_COLOR", "qt_varColor" },

    // effects
    { "INPUT", "qt_inputTexture" },
    { "INPUT_UV", "qt_inputUV" },
    { "TEXTURE_UV", "qt_textureUV" },
    { "INPUT_SIZE", "qt_inputSize" },
    { "OUTPUT_SIZE", "qt_outputSize" },
    { "FRAME", "qt_frame_num" },

    // instancing
    { "INSTANCE_COLOR", "qt_instanceColor"},
    { "INSTANCE_DATA", "qt_instanceData"},
    { "INSTANCE_INDEX", "gl_InstanceIndex"},

    // morphing
    { "MORPH_POSITION0", "attr_tpos0"},
    { "MORPH_POSITION1", "attr_tpos1"},
    { "MORPH_POSITION2", "attr_tpos2"},
    { "MORPH_POSITION3", "attr_tpos3"},
    { "MORPH_POSITION4", "attr_tpos4"},
    { "MORPH_POSITION5", "attr_tpos5"},
    { "MORPH_POSITION6", "attr_tpos6"},
    { "MORPH_POSITION7", "attr_tpos7"},
    { "MORPH_NORMAL0", "attr_tnorm0"},
    { "MORPH_NORMAL1", "attr_tnorm1"},
    { "MORPH_NORMAL2", "attr_tnorm2"},
    { "MORPH_NORMAL3", "attr_tnorm3"},
    { "MORPH_TANGENT0", "attr_ttan0"},
    { "MORPH_TANGENT1", "attr_ttan1"},
    { "MORPH_BINORMAL0", "attr_tbinorm0"},
    { "MORPH_BINORMAL1", "attr_tbinorm1"},
    { "MORPH_WEIGHTS", "qt_morphWeights"},

    // custom variables
    { "SHARED_VARS", "struct QT_SHARED_VARS"}
};

// Functions that, if present, get an argument list injected.
static const QByteArrayView qssg_func_injectarg_tab[] = {
    "DIRECTIONAL_LIGHT",
    "POINT_LIGHT",
    "SPOT_LIGHT",
    "AMBIENT_LIGHT",
    "SPECULAR_LIGHT",
    "MAIN",
    "POST_PROCESS"
};

// This is based on the Qt Quick shader rewriter (with fixes)
struct Tokenizer {
    enum Token {
        Token_Comment,
        Token_OpenBrace,
        Token_CloseBrace,
        Token_OpenParen,
        Token_CloseParen,
        Token_SemiColon,
        Token_Identifier,
        Token_Macro,
        Token_Unspecified,

        Token_EOF
    };

    void initialize(const QByteArray &input);
    Token next();

    const char *stream;
    const char *pos;
    const char *identifier;
};

void Tokenizer::initialize(const QByteArray &input)
{
    stream = input.constData();
    pos = input;
    identifier = input;
}

Tokenizer::Token Tokenizer::next()
{
    while (*pos) {
        char c = *pos++;
        switch (c) {
        case '/':
            if (*pos == '/') {
                // '//' comment
                ++pos;
                while (*pos && *pos != '\n') ++pos;
                return Token_Comment;
            } else if (*pos == '*') {
                // /* */ comment
                ++pos;
                while (*pos && (*pos != '*' || pos[1] != '/')) ++pos;
                if (*pos) pos += 2;
                return Token_Comment;
            }
            return Token_Unspecified;

        case '#': {
            while (*pos) {
                if (*pos == '\n') {
                    ++pos;
                    break;
                } else if (*pos == '\\') {
                    ++pos;
                    while (*pos && (*pos == ' ' || *pos == '\t'))
                        ++pos;
                    if (*pos && (*pos == '\n' || (*pos == '\r' && pos[1] == '\n')))
                        pos += 2;
                } else {
                    ++pos;
                }
            }
            return Token_Unspecified;
        }

        case ';': return Token_SemiColon;
        case '\0': return Token_EOF;
        case '{': return Token_OpenBrace;
        case '}': return Token_CloseBrace;
        case '(': return Token_OpenParen;
        case ')': return Token_CloseParen;

        case ' ':
        case '\n':
        case '\r': break;
        default:
            // Identifier...
            if ((c >= 'a' && c <= 'z' ) || (c >= 'A' && c <= 'Z' ) || c == '_') {
                identifier = pos - 1;
                while (*pos && ((*pos >= 'a' && *pos <= 'z')
                                     || (*pos >= 'A' && *pos <= 'Z')
                                     || *pos == '_'
                                     || (*pos >= '0' && *pos <= '9'))) {
                    ++pos;
                }
                return Token_Identifier;
            } else {
                return Token_Unspecified;
            }
        }
    }

    return Token_EOF;
}
} // namespace

QSSGShaderCustomMaterialAdapter::ShaderCodeAndMetaData
QSSGShaderCustomMaterialAdapter::prepareCustomShader(QByteArray &dst,
                                                     const QByteArray &shaderCode,
                                                     QSSGShaderCache::ShaderType type,
                                                     const StringPairList &baseUniforms,
                                                     const StringPairList &baseInputs,
                                                     const StringPairList &baseOutputs)
{
    QByteArrayList inputs;
    QByteArrayList outputs;

    Tokenizer tok;
    tok.initialize(shaderCode);

    QSSGCustomShaderMetaData md = {};
    QByteArray result;
    result.reserve(1024);
    // If shader debugging is not enabled we reset the line count to make error message
    // when a shader fails more useful. When shader debugging is enabled the whole shader
    // will be printed and not just the user written part, so in that case we do not want
    // to adjust the line numbers.
    //
    // NOTE: This is not perfect, we do expend the custom material and effect shaders, so
    // there cane still be cases where the reported line numbers are slightly off.
    if (!QSSGRhiContext::shaderDebuggingEnabled())
        result.prepend("#line 1\n");
    const char *lastPos = shaderCode.constData();

    int funcFinderState = 0;
    QByteArray currentShadedFunc;
    Tokenizer::Token t = tok.next();
    while (t != Tokenizer::Token_EOF) {
        switch (t) {
        case Tokenizer::Token_Comment:
            break;
        case Tokenizer::Token_Identifier:
        {
            QByteArray id = QByteArray::fromRawData(lastPos, tok.pos - lastPos);
            if (id.trimmed() == QByteArrayLiteral("VARYING")) {
                QByteArray vtype;
                QByteArray vname;
                lastPos = tok.pos;
                t = tok.next();
                while (t != Tokenizer::Token_EOF) {
                    QByteArray data = QByteArray::fromRawData(lastPos, tok.pos - lastPos);
                    if (t == Tokenizer::Token_Identifier) {
                        if (vtype.isEmpty())
                            vtype = data.trimmed();
                        else if (vname.isEmpty())
                            vname = data.trimmed();
                    }
                    if (t == Tokenizer::Token_SemiColon)
                        break;
                    lastPos = tok.pos;
                    t = tok.next();
                }
                if (type == QSSGShaderCache::ShaderType::Vertex)
                    outputs.append(vtype + " " + vname);
                else
                    inputs.append(vtype + " " + vname);
            } else {
                const QByteArray trimmedId = id.trimmed();
                if (funcFinderState == 0 && trimmedId == QByteArrayLiteral("void")) {
                    funcFinderState += 1;
                } else if (funcFinderState == 1) {
                    auto begin = qssg_func_injectarg_tab;
                    const auto end = qssg_func_injectarg_tab + (sizeof(qssg_func_injectarg_tab) / sizeof(qssg_func_injectarg_tab[0]));
                    const auto foundIt = std::find_if(begin, end, [trimmedId](const QByteArrayView &entry) { return entry == trimmedId; });
                    if (foundIt != end) {
                        currentShadedFunc = trimmedId;
                        funcFinderState += 1;
                    }
                } else {
                    funcFinderState = 0;
                }

                if (trimmedId == QByteArrayLiteral("SCREEN_TEXTURE"))
                    md.flags |= QSSGCustomShaderMetaData::UsesScreenTexture;
                else if (trimmedId == QByteArrayLiteral("SCREEN_MIP_TEXTURE"))
                    md.flags |= QSSGCustomShaderMetaData::UsesScreenMipTexture;
                else if (trimmedId == QByteArrayLiteral("DEPTH_TEXTURE"))
                    md.flags |= QSSGCustomShaderMetaData::UsesDepthTexture;
                else if (trimmedId == QByteArrayLiteral("AO_TEXTURE"))
                    md.flags |= QSSGCustomShaderMetaData::UsesAoTexture;
                else if (trimmedId == QByteArrayLiteral("POSITION"))
                    md.flags |= QSSGCustomShaderMetaData::OverridesPosition;
                else if (trimmedId == QByteArrayLiteral("PROJECTION_MATRIX"))
                    md.flags |= QSSGCustomShaderMetaData::UsesProjectionMatrix;
                else if (trimmedId == QByteArrayLiteral("INVERSE_PROJECTION_MATRIX"))
                    md.flags |= QSSGCustomShaderMetaData::UsesInverseProjectionMatrix;
                else if (trimmedId == QByteArrayLiteral("VAR_COLOR"))
                    md.flags |= QSSGCustomShaderMetaData::UsesVarColor;
                else if (trimmedId == QByteArrayLiteral("SHARED_VARS"))
                    md.flags |= QSSGCustomShaderMetaData::UsesSharedVars;

                for (const QSSGCustomMaterialVariableSubstitution &subst : qssg_var_subst_tab) {
                    if (trimmedId == subst.builtin) {
                        id.replace(subst.builtin, subst.actualName); // replace, not assignment, to keep whitespace etc.
                        break;
                    }
                }
                result += id;
            }
        }
            break;
        case Tokenizer::Token_OpenParen:
            result += QByteArray::fromRawData(lastPos, tok.pos - lastPos);
            if (funcFinderState == 2) {
                result += QByteArrayLiteral("/*%QT_ARGS_");
                result += currentShadedFunc;
                result += QByteArrayLiteral("%*/");
                for (const QSSGCustomMaterialVariableSubstitution &subst : qssg_var_subst_tab) {
                    if (currentShadedFunc == subst.builtin) {
                        currentShadedFunc = subst.actualName.toByteArray();
                        break;
                    }
                }
                md.customFunctions.insert(currentShadedFunc);
                currentShadedFunc.clear();
            }
            funcFinderState = 0;
            break;
        default:
            result += QByteArray::fromRawData(lastPos, tok.pos - lastPos);
            break;
        }
        lastPos = tok.pos;
        t = tok.next();
    }

    result += '\n';

    StringPairList allUniforms = baseUniforms;
    if (md.flags.testFlag(QSSGCustomShaderMetaData::UsesScreenTexture) || md.flags.testFlag(QSSGCustomShaderMetaData::UsesScreenMipTexture))
        allUniforms.append({ "sampler2D", "qt_screenTexture" });
    if (md.flags.testFlag(QSSGCustomShaderMetaData::UsesDepthTexture))
        allUniforms.append({ "sampler2D", "qt_depthTexture" });
    if (md.flags.testFlag(QSSGCustomShaderMetaData::UsesAoTexture))
        allUniforms.append({ "sampler2D", "qt_aoTexture" });

    static const char *metaStart = "#ifdef QQ3D_SHADER_META\n/*{\n  \"uniforms\": [\n";
    static const char *metaEnd = "  ]\n}*/\n#endif\n";
    dst.append(metaStart);
    for (int i = 0, count = allUniforms.count(); i < count; ++i) {
        const auto &typeAndName(allUniforms[i]);
        dst.append("    { \"type\": \"" + typeAndName.first + "\", \"name\": \"" + typeAndName.second + "\" }");
        if (i < count - 1)
            dst.append(",");
        dst.append("\n");
    }
    dst.append(metaEnd);

    const char *stageStr = type == QSSGShaderCache::ShaderType::Vertex ? "vertex" : "fragment";
    StringPairList allInputs = baseInputs;
    for (const QByteArray &inputTypeAndName : inputs) {
        const QByteArrayList typeAndName = inputTypeAndName.split(' ');
        if (typeAndName.count() == 2)
            allInputs.append({ typeAndName[0].trimmed(), typeAndName[1].trimmed() });
    }
    if (!allInputs.isEmpty()) {
        static const char *metaStart = "#ifdef QQ3D_SHADER_META\n/*{\n  \"inputs\": [\n";
        static const char *metaEnd = "  ]\n}*/\n#endif\n";
        dst.append(metaStart);
        for (int i = 0, count = allInputs.count(); i < count; ++i) {
            dst.append("    { \"type\": \"" + allInputs[i].first
                    + "\", \"name\": \"" + allInputs[i].second
                    + "\", \"stage\": \"" + stageStr + "\" }");
            if (i < count - 1)
                dst.append(",");
            dst.append("\n");
        }
        dst.append(metaEnd);
    }

    StringPairList allOutputs = baseOutputs;
    for (const QByteArray &outputTypeAndName : outputs) {
        const QByteArrayList typeAndName = outputTypeAndName.split(' ');
        if (typeAndName.count() == 2)
            allOutputs.append({ typeAndName[0].trimmed(), typeAndName[1].trimmed() });
    }
    if (!allOutputs.isEmpty()) {
        static const char *metaStart = "#ifdef QQ3D_SHADER_META\n/*{\n  \"outputs\": [\n";
        static const char *metaEnd = "  ]\n}*/\n#endif\n";
        dst.append(metaStart);
        for (int i = 0, count = allOutputs.count(); i < count; ++i) {
            dst.append("    { \"type\": \"" + allOutputs[i].first
                    + "\", \"name\": \"" + allOutputs[i].second
                    + "\", \"stage\": \"" + stageStr + "\" }");
            if (i < count - 1)
                dst.append(",");
            dst.append("\n");
        }
        dst.append(metaEnd);
    }

    return { result, md };
}

QT_END_NAMESPACE
