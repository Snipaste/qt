/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "genshaders.h"

#include <QtCore/qdir.h>

#include <QtGui/private/qrhinull_p_p.h>

#include <QtQml/qqmllist.h>

#include <QtQuick3D/private/qquick3dsceneenvironment_p.h>
#include <QtQuick3D/private/qquick3dprincipledmaterial_p.h>
#include <QtQuick3D/private/qquick3dviewport_p.h>
#include <QtQuick3D/private/qquick3dscenerenderer_p.h>
#include <QtQuick3D/private/qquick3dscenemanager_p.h>
#include <QtQuick3D/private/qquick3dperspectivecamera_p.h>

// Lights
#include <QtQuick3D/private/qquick3dspotlight_p.h>
#include <QtQuick3D/private/qquick3ddirectionallight_p.h>
#include <QtQuick3D/private/qquick3dpointlight_p.h>

#include <QtQuick3DUtils/private/qqsbcollection_p.h>

#include <private/qssgrenderer_p.h>
#include <private/qssgrendererimpllayerrenderdata_p.h>

#include <QtQuick3DRuntimeRender/private/qssgrhieffectsystem_p.h>

#include <QtShaderTools/private/qshaderbaker_p.h>

static inline void qDryRunPrintQsbcAdd(const QByteArray &id)
{
    printf("Shader pipeline generated for (dry run):\n %s\n\n", qPrintable(id));
}

static void initBaker(QShaderBaker *baker, QRhi::Implementation target)
{
    Q_UNUSED(target);
    QVector<QShaderBaker::GeneratedShader> outputs;
    // TODO: For simplicity we're just going to add all off these for now.
    outputs.append({ QShader::SpirvShader, QShaderVersion(100) }); // Vulkan 1.0
    outputs.append({ QShader::HlslShader, QShaderVersion(50) }); // Shader Model 5.0
    outputs.append({ QShader::MslShader, QShaderVersion(12) }); // Metal 1.2
    outputs.append({ QShader::GlslShader, QShaderVersion(300, QShaderVersion::GlslEs) }); // GLES 3.0+
    outputs.append({ QShader::GlslShader, QShaderVersion(130) }); // OpenGL 3.0+

    baker->setGeneratedShaders(outputs);
    baker->setGeneratedShaderVariants({ QShader::StandardShader });
}

static QQsbShaderFeatureSet toQsbShaderFeatureSet(const ShaderFeatureSetList &featureSet)
{
    QQsbShaderFeatureSet ret;
    for (const auto &f : featureSet)
        ret.insert(f.name, f.enabled);
    return ret;
}

GenShaders::GenShaders()
{
    sceneManager = new QQuick3DSceneManager;

    rhi = QRhi::create(QRhi::Null, nullptr);
    QRhiCommandBuffer *cb;
    rhi->beginOffscreenFrame(&cb);

    const auto rhiContext = QSSGRef<QSSGRhiContext>(new QSSGRhiContext);
    rhiContext->initialize(rhi);
    rhiContext->setCommandBuffer(cb);

    auto shaderCache = new QSSGShaderCache(rhiContext, &initBaker);
    renderContext = QSSGRef<QSSGRenderContextInterface>(new QSSGRenderContextInterface(rhiContext,
                                                                                       new QSSGBufferManager(rhiContext, shaderCache),
                                                                                       new QSSGResourceManager(rhiContext),
                                                                                       new QSSGRenderer,
                                                                                       new QSSGShaderLibraryManager,
                                                                                       shaderCache,
                                                                                       new QSSGCustomMaterialSystem,
                                                                                       new QSSGProgramGenerator));
    sceneManager->rci = renderContext.data();
}

GenShaders::~GenShaders() = default;

bool GenShaders::process(const MaterialParser::SceneData &sceneData,
                         QVector<QString> &qsbcFiles,
                         const QDir &outDir,
                         bool generateMultipleLights,
                         bool dryRun)
{
    Q_UNUSED(generateMultipleLights);

    const QString resourceFolderRelative = QSSGShaderCache::resourceFolder().mid(2);
    if (!dryRun && !outDir.exists(resourceFolderRelative)) {
        if (!outDir.mkpath(resourceFolderRelative)) {
            qDebug("Unable to create folder: %s", qPrintable(outDir.path() + QDir::separator() + resourceFolderRelative));
            return false;
        }
    }

    const QString outputFolder = outDir.canonicalPath() + QDir::separator() + resourceFolderRelative;

    QSSGRenderLayer layer;
    renderContext->setViewport(QRect(QPoint(), QSize(888,666)));
    const auto &renderer = renderContext->renderer();
    QSSGLayerRenderData layerData(layer, renderer);

    const auto &shaderLibraryManager = renderContext->shaderLibraryManager();
    const auto &shaderCache = renderContext->shaderCache();
    const auto &shaderProgramGenerator = renderContext->shaderProgramGenerator();

    bool aaIsDirty = false;
    bool temporalIsDirty = false;
    float ssaaMultiplier = 1.5f;

    QQuick3DViewport *view3D = sceneData.viewport;
    Q_ASSERT(view3D);

    QVector<QSSGRenderGraphObject *> nodes;

    if (!view3D->camera()) {
        auto camera = new QQuick3DPerspectiveCamera();
        auto node = QQuick3DObjectPrivate::updateSpatialNode(camera, nullptr);
        QQuick3DObjectPrivate::get(camera)->spatialNode = node;
        nodes.append(node);
        view3D->setCamera(camera);
    }

    // Realize resources
    // Textures
    const auto &textures = sceneData.textures;
    for (const auto &tex : textures) {
        auto node = QQuick3DObjectPrivate::updateSpatialNode(tex, nullptr);
        auto obj = QQuick3DObjectPrivate::get(tex);
        obj->spatialNode = node;
        nodes.append(node);
    }

    // Free Materials (see also the model section)
    const auto &materials = sceneData.materials;
    for (const auto &mat : materials) {
        auto obj = QQuick3DObjectPrivate::get(mat);
        obj->sceneManager = sceneManager;
        auto node = QQuick3DObjectPrivate::updateSpatialNode(mat, nullptr);
        obj->spatialNode = node;
        nodes.append(node);
    }

    bool shadowCubePass = false;
    bool shadowMapPass = false;

    // Lights
    const auto &lights = sceneData.lights;
    for (const auto &light : lights) {
        if (auto node = QQuick3DObjectPrivate::updateSpatialNode(light, nullptr)) {
            nodes.append(node);
            layer.addChild(static_cast<QSSGRenderNode &>(*node));
            const auto &lightNode = static_cast<const QSSGRenderLight &>(*node);
            if (lightNode.type == QSSGRenderLight::Type::PointLight)
                shadowCubePass |= true;
            else
                shadowMapPass |= true;
        }
    }

    // NOTE: Model.castsShadows; Model.receivesShadows; variants needs to be added for runtime support
    const auto &models = sceneData.models;
    for (const auto &model : models) {
        auto materialList = model->materials();
        for (int i = 0, e = materialList.count(&materialList); i != e; ++i) {
            auto mat = materialList.at(&materialList, i);
            auto obj = QQuick3DObjectPrivate::get(mat);
            obj->sceneManager = sceneManager;
            QSSGRenderGraphObject *node = nullptr;
            if (obj->type == QQuick3DObjectPrivate::Type::CustomMaterial) {
                auto customMatNode = new QSSGRenderCustomMaterial;
                customMatNode->incompleteBuildTimeObject = true;
                node = QQuick3DObjectPrivate::updateSpatialNode(mat, customMatNode);
                customMatNode->incompleteBuildTimeObject = false;
            } else {
                node = QQuick3DObjectPrivate::updateSpatialNode(mat, nullptr);
            }
            QQuick3DObjectPrivate::get(mat)->spatialNode = node;
            nodes.append(node);
        }
        if (auto instanceList = qobject_cast<QQuick3DInstanceList *>(model->instancing())) {
            auto obj = QQuick3DObjectPrivate::get(instanceList);
            auto node = QQuick3DObjectPrivate::updateSpatialNode(instanceList, nullptr);
            obj->spatialNode = node;
            nodes.append(node);
        }

        auto node = QQuick3DObjectPrivate::updateSpatialNode(model, nullptr);
        QQuick3DObjectPrivate::get(model)->spatialNode = node;
        nodes.append(node);
    }

    QQuick3DRenderLayerHelpers::updateLayerNodeHelper(*view3D, layer, aaIsDirty, temporalIsDirty, ssaaMultiplier);

    const QString outCollectionFile = outputFolder + QString::fromLatin1(QSSGShaderCache::shaderCollectionFile());
    QQsbCollection qsbc(outCollectionFile);
    if (!dryRun && !qsbc.map(QQsbCollection::Write))
        return false;

    QByteArray shaderString;
    const auto generateShaderForModel = [&](QSSGRenderModel &model) {
        layerData.resetForFrame();
        layer.addChild(model);
        layerData.prepareForRender(QSize(888, 666));

        const auto &features = layerData.getShaderFeatureSet();

        auto &materialPropertis = layerData.renderer->defaultMaterialShaderKeyProperties();

        QSSGRenderableObject *renderable = nullptr;
        if (!layerData.opaqueObjects.isEmpty())
            renderable = layerData.opaqueObjects.at(0).obj;
        else if (!layerData.transparentObjects.isEmpty())
            renderable = layerData.transparentObjects.at(0).obj;

        auto generateShader = [&](const ShaderFeatureSetList &features) {
            if (renderable->renderableFlags.testFlag(QSSGRenderableObjectFlag::DefaultMaterialMeshSubset)) {
                auto shaderPipeline = QSSGRenderer::generateRhiShaderPipelineImpl(*static_cast<QSSGSubsetRenderable *>(renderable), shaderLibraryManager, shaderCache, shaderProgramGenerator, materialPropertis, features, shaderString);
                if (!shaderPipeline.isNull()) {
                    const size_t hkey = QSSGShaderCacheKey::generateHashCode(shaderString, features);
                    const auto vertexStage = shaderPipeline->vertexStage();
                    const auto fragmentStage = shaderPipeline->fragmentStage();
                    if (vertexStage && fragmentStage) {
                        if (dryRun)
                            qDryRunPrintQsbcAdd(shaderString);
                        else
                            qsbc.addQsbEntry(shaderString, toQsbShaderFeatureSet(features), vertexStage->shader(), fragmentStage->shader(), hkey);
                    }
                }
            } else if (renderable->renderableFlags.testFlag(QSSGRenderableObjectFlag::CustomMaterialMeshSubset)) {
                Q_ASSERT(layerData.camera);
                QSSGSubsetRenderable &cmr(static_cast<QSSGSubsetRenderable &>(*renderable));
                const auto &rhiContext = renderContext->rhiContext();
                const auto pipelineState = rhiContext->graphicsPipelineState(&layerData);
                const auto &cms = renderContext->customMaterialSystem();
                auto shaderPipeline = cms->shadersForCustomMaterial(pipelineState,
                                                                    cmr.customMaterial(),
                                                                    cmr,
                                                                    features);

                if (shaderPipeline) {
                    shaderString = cmr.customMaterial().m_shaderPathKey;
                    const size_t hkey = QSSGShaderCacheKey::generateHashCode(shaderString, features);
                    const auto vertexStage = shaderPipeline->vertexStage();
                    const auto fragmentStage = shaderPipeline->fragmentStage();
                    if (vertexStage && fragmentStage) {
                        if (dryRun)
                            qDryRunPrintQsbcAdd(shaderString);
                        else
                            qsbc.addQsbEntry(shaderString, toQsbShaderFeatureSet(features), vertexStage->shader(), fragmentStage->shader(), hkey);
                    }
                }
            }
        };

        if (renderable) {
            generateShader(features);

            ShaderFeatureSetList depthPassFeatures;
            depthPassFeatures.append({ QSSGShaderDefines::DepthPass, true });
            generateShader(depthPassFeatures);

            if (shadowCubePass) {
                ShaderFeatureSetList shadowPassFeatures;
                shadowPassFeatures.append({ QSSGShaderDefines::CubeShadowPass, true });
                generateShader(shadowPassFeatures);
            }

            if (shadowMapPass) {
                ShaderFeatureSetList shadowPassFeatures;
                shadowPassFeatures.append({ QSSGShaderDefines::OrthoShadowPass, true });
                generateShader(shadowPassFeatures);
            }
        }
        layer.removeChild(model);
    };

    for (const auto &model : models)
        generateShaderForModel(static_cast<QSSGRenderModel &>(*QQuick3DObjectPrivate::get(model)->spatialNode));

    // Let's generate some shaders for the "free" materials as well.
    QSSGRenderModel model; // dummy
    model.meshPath = QSSGRenderPath("#Cube");
    for (const auto &mat : materials) {
        model.materials = { QQuick3DObjectPrivate::get(mat)->spatialNode };
        generateShaderForModel(model);
    }

    // Now generate the shaders for the effects
    const auto generateEffectShader = [&](QQuick3DEffect &effect) {
        auto obj = QQuick3DObjectPrivate::get(&effect);
        obj->sceneManager = sceneManager;
        QSSGRenderEffect *renderEffect = new QSSGRenderEffect;
        renderEffect->incompleteBuildTimeObject = true;
        if (auto ret = QQuick3DObjectPrivate::updateSpatialNode(&effect, renderEffect))
            Q_ASSERT(ret == renderEffect);
        renderEffect->incompleteBuildTimeObject = false;
        obj->spatialNode = renderEffect;
        nodes.append(renderEffect);

        const auto &commands = renderEffect->commands;
        for (const auto &command : commands) {
            if (command->m_type == CommandType::BindShader) {
                auto bindShaderCommand = static_cast<const QSSGBindShader &>(*command);
                for (const auto isYUpInFramebuffer : { true, false }) { // Generate effects for both up-directions.
                    const auto shaderPipeline = QSSGRhiEffectSystem::buildShaderForEffect(bindShaderCommand,
                                                                                          shaderProgramGenerator,
                                                                                          shaderLibraryManager,
                                                                                          shaderCache,
                                                                                          isYUpInFramebuffer);
                    if (shaderPipeline) {
                        const auto &key = bindShaderCommand.m_shaderPathKey;
                        const size_t hkey = bindShaderCommand.m_hkey;
                        Q_ASSERT(hkey != 0);
                        const auto vertexStage = shaderPipeline->vertexStage();
                        const auto fragmentStage = shaderPipeline->fragmentStage();
                        if (vertexStage && fragmentStage) {
                            if (dryRun)
                                qDryRunPrintQsbcAdd(key);
                            else
                                qsbc.addQsbEntry(key, toQsbShaderFeatureSet(ShaderFeatureSetList()), vertexStage->shader(), fragmentStage->shader(), hkey);
                        }
                    }
                }
            }
        }
    };

    // Effects
    if (sceneData.viewport && sceneData.viewport->environment()) {
        auto &env = *sceneData.viewport->environment();
        auto effects = env.effects();
        const auto effectCount = effects.count(&effects);
        for (int i = 0; i < effectCount; ++i) {
            auto effect = effects.at(&effects, i);
            generateEffectShader(*effect);
        }
    }

    // Free Effects
    for (const auto &effect : qAsConst(sceneData.effects))
        generateEffectShader(*effect);

    if (!qsbc.getEntries().isEmpty())
        qsbcFiles.push_back(resourceFolderRelative + QDir::separator() + QString::fromLatin1(QSSGShaderCache::shaderCollectionFile()));
    qsbc.unmap();

    auto &children = layer.children;
    for (auto it = children.begin(), end = children.end(); it != end;)
        children.remove(*it++);

    qDeleteAll(nodes);

    return true;
}
