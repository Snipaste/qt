/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "assimputils.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/Logger.hpp>
#include <assimp/DefaultLogger.hpp>
#include <assimp/postprocess.h>
#include <assimp/pbrmaterial.h>
#include <assimp/importerdesc.h>

#include <QtCore/qstring.h>

#include <QtQuick3DAssetImport/private/qssglightmapuvgenerator_p.h>

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

QT_BEGIN_NAMESPACE

namespace
{

struct SubsetEntryData {
    QString name;
    int indexLength;
    int indexOffset;
    quint32 lightmapWidth;
    quint32 lightmapHeight;
};

}

QSSGMesh::Mesh AssimpUtils::generateMeshData(const aiScene &scene,
                                             const MeshList &meshes,
                                             const BoneIndexMap &boneIdxMap,
                                             bool generateLightmapUV,
                                             bool useFloatJointIndices,
                                             QString &errorString)
{
    // Check if we need placeholders in certain channels
    bool needsPositionData = false;
    bool needsNormalData = false;
    bool needsUV0Data = false;
    bool needsUV1Data = false;
    bool needsTangentData = false;
    bool needsVertexColorData = false;
    unsigned uv0Components = 0;
    unsigned uv1Components = 0;
    bool needsBones = false;

    // GLTF should support at least 8 attributes for morphing.
    // The supported combinations are the followings.
    // 1. 8 targets having only positions.
    // 2. 4 targets having both positions and normals.
    // 3. 2 targets having positions, normals, and tangents(with binormals)
    //
    // 4. 2 targets having only positions and 3 targets having both positions
    //   and normals,
    // 5. ....
    //
    // Handling the same types is simple but let's think about 4.
    // In this case, animMeshes should be sorted by descending order of the
    // number of input attributes. It means that we need to process 3 targets
    // having more attributes first and then 2 remaining targets.
    // However, we will assume the asset is made by this correct order.

    quint32 numMorphTargets = 0;
    QVector<bool> needsTargetPosition;
    QVector<bool> needsTargetNormal;
    QVector<bool> needsTargetTangent;
    QVector<float> targetWeight;

    for (const auto *mesh : meshes) {
        uv0Components = qMax(mesh->mNumUVComponents[0], uv0Components);
        uv1Components = qMax(mesh->mNumUVComponents[1], uv1Components);
        needsPositionData |= mesh->HasPositions();
        needsNormalData |= mesh->HasNormals();
        needsUV0Data |= mesh->HasTextureCoords(0);
        needsUV1Data |= mesh->HasTextureCoords(1);
        needsTangentData |= mesh->HasTangentsAndBitangents();
        needsVertexColorData |=mesh->HasVertexColors(0);
        needsBones |= mesh->HasBones();
        if (mesh->mNumAnimMeshes && mesh->mAnimMeshes) {
            if (mesh->mNumAnimMeshes > 8)
                qWarning() << "QtQuick3D supports maximum 8 morph targets, remains will be ignored\n";
            const quint32 numAnimMeshes = qMin(8U, mesh->mNumAnimMeshes);
            if (numMorphTargets < numAnimMeshes) {
                numMorphTargets = numAnimMeshes;
                needsTargetPosition.resize(numMorphTargets);
                needsTargetNormal.resize(numMorphTargets);
                needsTargetTangent.resize(numMorphTargets);
                targetWeight.resize(numMorphTargets);
            }
            for (uint i = 0; i < numAnimMeshes; ++i) {
                auto animMesh = mesh->mAnimMeshes[i];
                needsTargetPosition[i] |= animMesh->HasPositions();
                needsTargetNormal[i] |= animMesh->HasNormals();
                needsTargetTangent[i] |= animMesh->HasTangentsAndBitangents();
                targetWeight[i] = animMesh->mWeight;
            }
        }
    }

    QByteArray positionData;
    QByteArray normalData;
    QByteArray uv0Data;
    QByteArray uv1Data;
    QByteArray tangentData;
    QByteArray binormalData;
    QByteArray vertexColorData;
    QByteArray indexBufferData;
    QByteArray boneIndexData;
    QByteArray boneWeightData;
    QByteArray targetPositionData[8];
    QByteArray targetNormalData[8];
    QByteArray targetTangentData[8];
    QByteArray targetBinormalData[8];
    QVector<SubsetEntryData> subsetData;
    quint32 baseIndex = 0;

    // Always use 32-bit indices. Metal has a requirement of 4 byte alignment
    // for index buffer offsets, and we cannot risk hitting that.
    QSSGMesh::Mesh::ComponentType indexType = QSSGMesh::Mesh::ComponentType::UnsignedInt32;

    const quint32 float32ByteSize = QSSGMesh::MeshInternal::byteSizeForComponentType(QSSGMesh::Mesh::ComponentType::Float32);
    for (const auto *mesh : meshes) {
        // Index Buffer
        QVector<quint32> indexes;
        indexes.reserve(mesh->mNumFaces * 3);

        for (unsigned int faceIndex = 0;faceIndex < mesh->mNumFaces; ++faceIndex) {
            const auto face = mesh->mFaces[faceIndex];
            // Faces should always have 3 indicides
            Q_ASSERT(face.mNumIndices == 3);
            indexes.append(quint32(face.mIndices[0]) + baseIndex);
            indexes.append(quint32(face.mIndices[1]) + baseIndex);
            indexes.append(quint32(face.mIndices[2]) + baseIndex);
        }
        // Since we might be combining multiple meshes together, we also need to change the index offset
        baseIndex = *std::max_element(indexes.constBegin(), indexes.constEnd()) + 1;

        QByteArray positions;
        if (mesh->HasPositions()) {
            positions = QByteArray::fromRawData(reinterpret_cast<const char *>(mesh->mVertices),
                                                mesh->mNumVertices * 3 * float32ByteSize);
        }

        QByteArray normals;
        if (mesh->HasNormals()) {
            normals = QByteArray::fromRawData(reinterpret_cast<const char *>(mesh->mNormals),
                                              mesh->mNumVertices * 3 * float32ByteSize);
        }

        QByteArray uv0;
        if (mesh->HasTextureCoords(0)) {
            QVector<float> uvCoords;
            uvCoords.resize(uv0Components * mesh->mNumVertices);
            for (uint i = 0; i < mesh->mNumVertices; ++i) {
                int offset = i * uv0Components;
                aiVector3D *textureCoords = mesh->mTextureCoords[0];
                uvCoords[offset] = textureCoords[i].x;
                uvCoords[offset + 1] = textureCoords[i].y;
                if (uv0Components == 3)
                    uvCoords[offset + 2] = textureCoords[i].z;
            }
            uv0 = QByteArray(reinterpret_cast<const char*>(uvCoords.constData()), uvCoords.size() * sizeof(float));
        }

        // UV unwrapping for lightmap
        int outputVertexCount = mesh->mNumVertices;
        QVector<quint32> vertexMap;
        QByteArray lightmapUVChannel;
        quint32 lightmapWidth = 0;
        quint32 lightmapHeight = 0;
        if (generateLightmapUV && mesh->HasPositions()) {
            QSSGLightmapUVGenerator uvGen;
            const QByteArray indices = QByteArray::fromRawData(reinterpret_cast<const char *>(indexes.constData()),
                                                               indexes.count() * sizeof(quint32));
            QSSGLightmapUVGeneratorResult lightmapResult = uvGen.run(positions,
                                                                     normals,
                                                                     uv0,
                                                                     indices,
                                                                     QSSGMesh::Mesh::ComponentType::UnsignedInt32);
            if (lightmapResult.isValid()) {
                qDebug("Lightmap UV unwrap, original vertex count = %u, new vertex count = %d, "
                       "texture size hint = %ux%u",
                       mesh->mNumVertices,
                       int(lightmapResult.vertexMap.count()),
                       lightmapResult.lightmapWidth,
                       lightmapResult.lightmapHeight);

                indexType = QSSGMesh::Mesh::ComponentType::UnsignedInt32;
                const quint32 *indexPtr = reinterpret_cast<const quint32 *>(lightmapResult.indexData.constData());
                for (int i = 0, count = indexes.count(); i != count; ++i)
                    indexes[i] = *indexPtr++;

                outputVertexCount = lightmapResult.vertexMap.count();
                vertexMap = lightmapResult.vertexMap;
                lightmapUVChannel = lightmapResult.lightmapUVChannel;
                lightmapWidth = lightmapResult.lightmapWidth;
                lightmapHeight = lightmapResult.lightmapHeight;
            } else {
                errorString = QStringLiteral("Lightmap UV generation failed");
                return QSSGMesh::Mesh();
            }
        }

        // From this point on, if vertexMap is non-empty, then remapping is
        // needed for all attribute data. Also note the potential difference
        // between outputVertexCount and mesh->mNumVertices from this point on.

        if (mesh->HasPositions()) {
            if (vertexMap.isEmpty())
                positionData += positions;
            else
                positionData += QSSGLightmapUVGenerator::remap<float>(positions, vertexMap, 3);
        } else if (needsPositionData) {
            positionData += QByteArray(outputVertexCount * 3 * float32ByteSize, '\0');
        }

        if (mesh->HasNormals()) {
            if (vertexMap.isEmpty())
                normalData += normals;
            else
                normalData += QSSGLightmapUVGenerator::remap<float>(normals, vertexMap, 3);
        } else if (needsNormalData) {
            normalData += QByteArray(outputVertexCount * 3 * float32ByteSize, '\0');
        }

        if (mesh->HasTextureCoords(0)) {
            if (vertexMap.isEmpty())
                uv0Data += uv0;
            else
                uv0Data += QSSGLightmapUVGenerator::remap<float>(uv0, vertexMap, uv0Components);
        } else {
            uv0Data += QByteArray(outputVertexCount * uv0Components * float32ByteSize, '\0');
        }

        if (!lightmapUVChannel.isEmpty()) {
            if (mesh->HasTextureCoords(1))
                qWarning("Mesh has UV1 but lightmap UV generation was enabled, overwriting UV1");
            uv1Data += lightmapUVChannel;
            uv1Components = 2;
        } else if (mesh->HasTextureCoords(1)) {
            QVector<float> uvCoords;
            uvCoords.resize(uv1Components * mesh->mNumVertices);
            for (uint i = 0; i < mesh->mNumVertices; ++i) {
                int offset = i * uv1Components;
                aiVector3D *textureCoords = mesh->mTextureCoords[1];
                uvCoords[offset] = textureCoords[i].x;
                uvCoords[offset + 1] = textureCoords[i].y;
                if (uv1Components == 3)
                    uvCoords[offset + 2] = textureCoords[i].z;
            }
            uv1Data += QByteArray(reinterpret_cast<const char*>(uvCoords.constData()), uvCoords.size() * sizeof(float));
        } else {
            Q_ASSERT(outputVertexCount == int(mesh->mNumVertices)); // because we handled the lightmapUVChannel case above
            uv1Data += QByteArray(mesh->mNumVertices * uv1Components * float32ByteSize, '\0');
        }

        if (mesh->HasTangentsAndBitangents()) {
            const QByteArray tangents = QByteArray::fromRawData(reinterpret_cast<const char *>(mesh->mTangents),
                                                                mesh->mNumVertices * 3 * float32ByteSize);
            if (vertexMap.isEmpty())
                tangentData += tangents;
            else
                tangentData += QSSGLightmapUVGenerator::remap<float>(tangents, vertexMap, 3);

            // Binormals (They are actually supposed to be Bitangents despite what they are called)
            const QByteArray binormals = QByteArray::fromRawData(reinterpret_cast<const char*>(mesh->mBitangents),
                                                                 mesh->mNumVertices * 3 * float32ByteSize);
            if (vertexMap.isEmpty())
                binormalData += binormals;
            else
                binormalData += QSSGLightmapUVGenerator::remap<float>(binormals, vertexMap, 3);
        } else if (needsTangentData) {
            tangentData += QByteArray(outputVertexCount * 3 * float32ByteSize, '\0');
            binormalData += QByteArray(outputVertexCount * 3 * float32ByteSize, '\0');
        }

        if (mesh->HasVertexColors(0)) {
            const QByteArray colors = QByteArray::fromRawData(reinterpret_cast<const char *>(mesh->mColors[0]),
                                                              mesh->mNumVertices * 4 * float32ByteSize);
            if (vertexMap.isEmpty())
                vertexColorData += colors;
            else
                vertexColorData += QSSGLightmapUVGenerator::remap<float>(colors, vertexMap, 4);
        } else if (needsVertexColorData) {
            vertexColorData += QByteArray(outputVertexCount * 4 * float32ByteSize, '\0');
        }

        // Bones + Weights
        QVector<qint32> boneIndexes;
        QVector<float> fBoneIndexes;
        QVector<float> weights;
        if (mesh->HasBones()) {
            weights.resize(mesh->mNumVertices * 4, 0.0f);
            if (useFloatJointIndices)
                fBoneIndexes.resize(mesh->mNumVertices * 4, 0);
            else
                boneIndexes.resize(mesh->mNumVertices * 4, 0);

            for (uint i = 0; i < mesh->mNumBones; ++i) {
                QString boneName = QString::fromUtf8(mesh->mBones[i]->mName.C_Str());

                const auto boneIdxItr = boneIdxMap.constFind(boneName);
                if (boneIdxItr == boneIdxMap.cend()) {
                    qWarning() << "Joint " << boneName << " is not included in pre-defined skeleton.";
                    continue;
                }

                for (uint j = 0; j < mesh->mBones[i]->mNumWeights; ++j) {
                    quint32 vertexId = mesh->mBones[i]->mWeights[j].mVertexId;
                    float weight = mesh->mBones[i]->mWeights[j].mWeight;

                    // skip a bone transform having small weight
                    if (weight <= 0.01f)
                        continue;

                    //  if any vertex has more weights than 4, it will be ignored
                    for (uint ii = 0; ii < 4; ++ii) {
                        if (weights[vertexId * 4 + ii] == 0.0f) {
                            if (useFloatJointIndices)
                                fBoneIndexes[vertexId * 4 + ii] = (float)*boneIdxItr;
                            else
                                boneIndexes[vertexId * 4 + ii] = *boneIdxItr;
                            weights[vertexId * 4 + ii] = weight;
                            break;
                        } else if (ii == 3) {
                            qWarning("vertexId %d has already 4 weights and index %d's weight %f will be ignored.", vertexId, *boneIdxItr, weight);
                        }
                    }
                }
            }

            if (useFloatJointIndices) {
                const QByteArray boneIndices = QByteArray::fromRawData(reinterpret_cast<const char *>(fBoneIndexes.constData()),
                                                                       fBoneIndexes.size() * sizeof(float));
                if (vertexMap.isEmpty())
                    boneIndexData += boneIndices;
                else
                    boneIndexData += QSSGLightmapUVGenerator::remap<float>(boneIndices, vertexMap, 4);
            } else {
                const QByteArray boneIndices = QByteArray::fromRawData(reinterpret_cast<const char *>(boneIndexes.constData()),
                                                                       boneIndexes.size() * sizeof(qint32));
                if (vertexMap.isEmpty())
                    boneIndexData += boneIndices;
                else
                    boneIndexData += QSSGLightmapUVGenerator::remap<qint32>(boneIndices, vertexMap, 4);
            }

            const QByteArray boneWeights = QByteArray::fromRawData(reinterpret_cast<const char *>(weights.constData()),
                                                                   weights.size() * sizeof(float));
            if (vertexMap.isEmpty())
                boneWeightData += boneWeights;
            else
                boneWeightData += QSSGLightmapUVGenerator::remap<float>(boneWeights, vertexMap, 4);
        } else if (needsBones) {
            boneIndexData += QByteArray(outputVertexCount * 4 * QSSGMesh::MeshInternal::byteSizeForComponentType(QSSGMesh::Mesh::ComponentType::Int32), '\0');
            boneWeightData += QByteArray(outputVertexCount * 4 * float32ByteSize, '\0');
        }

        for (uint i = 0; i < numMorphTargets; ++i) {
            aiAnimMesh *animMesh = nullptr;
            if (i < mesh->mNumAnimMeshes) {
                animMesh = mesh->mAnimMeshes[i];
                Q_ASSERT(animMesh->mNumVertices == mesh->mNumVertices);
            }
            if (needsTargetPosition[i]) {
                if (animMesh && animMesh->HasPositions()) {
                    const QByteArray targetPositions = QByteArray::fromRawData(reinterpret_cast<const char *>(animMesh->mVertices),
                                                                              mesh->mNumVertices * 3 * float32ByteSize);
                    if (vertexMap.isEmpty())
                        targetPositionData[i] += targetPositions;
                    else
                        targetPositionData[i] += QSSGLightmapUVGenerator::remap<float>(targetPositions, vertexMap, 3);
                 } else {
                    targetPositionData[i] += QByteArray(outputVertexCount * 3 * float32ByteSize, '\0');
                }
            }
            if (needsTargetNormal[i]) {
                if (animMesh && animMesh->HasNormals()) {
                    const QByteArray targetNormals = QByteArray::fromRawData(reinterpret_cast<const char *>(animMesh->mNormals),
                                                                             mesh->mNumVertices * 3 * float32ByteSize);
                    if (vertexMap.isEmpty())
                        targetNormalData[i] += targetNormals;
                    else
                        targetNormalData[i] += QSSGLightmapUVGenerator::remap<float>(targetNormals, vertexMap, 3);
                } else {
                    targetNormalData[i] += QByteArray(outputVertexCount * 3 * float32ByteSize, '\0');
                }
            }
            if (needsTargetTangent[i]) {
                if (animMesh && animMesh->HasTangentsAndBitangents()) {
                    const QByteArray targetTangents = QByteArray::fromRawData(reinterpret_cast<const char *>(animMesh->mTangents),
                                                                              mesh->mNumVertices * 3 * float32ByteSize);
                    const QByteArray targetBinormals = QByteArray::fromRawData(reinterpret_cast<const char *>(animMesh->mBitangents),
                                                                               mesh->mNumVertices * 3 * float32ByteSize);
                    if (vertexMap.isEmpty()) {
                        targetTangentData[i] += targetTangents;
                        targetBinormalData[i] += targetBinormals;
                    } else {
                        targetTangentData[i] += QSSGLightmapUVGenerator::remap<float>(targetTangents, vertexMap, 3);
                        targetBinormalData[i] += QSSGLightmapUVGenerator::remap<float>(targetBinormals, vertexMap, 3);
                    }
                } else {
                    targetTangentData[i] += QByteArray(outputVertexCount * 3 * float32ByteSize, '\0');
                    targetBinormalData[i] += QByteArray(outputVertexCount * 3 * float32ByteSize, '\0');
                }
            }
        }

        SubsetEntryData subsetEntry;
        subsetEntry.indexOffset = indexBufferData.length() / QSSGMesh::MeshInternal::byteSizeForComponentType(indexType);
        subsetEntry.indexLength = indexes.length();
        if (indexType == QSSGMesh::Mesh::ComponentType::UnsignedInt32) {
            indexBufferData += QByteArray(reinterpret_cast<const char *>(indexes.constData()),
                                          indexes.length() * QSSGMesh::MeshInternal::byteSizeForComponentType(indexType));
        } else {
            // convert data to quint16
            QVector<quint16> shortIndexes;
            shortIndexes.resize(indexes.length());
            for (int i = 0; i < shortIndexes.length(); ++i)
                shortIndexes[i] = quint16(indexes[i]);
            indexBufferData += QByteArray(reinterpret_cast<const char *>(shortIndexes.constData()),
                                          shortIndexes.length() * QSSGMesh::MeshInternal::byteSizeForComponentType(indexType));
        }

        subsetEntry.name = QString::fromUtf8(scene.mMaterials[mesh->mMaterialIndex]->GetName().C_Str());
        subsetEntry.lightmapWidth = lightmapWidth;
        subsetEntry.lightmapHeight = lightmapHeight;
        subsetData.append(subsetEntry);
    }

    // Vertex Buffer Entries
    QVector<QSSGMesh::AssetVertexEntry> entries;
    if (positionData.length() > 0) {
        entries.append({
                           QSSGMesh::MeshInternal::getPositionAttrName(),
                           positionData,
                           QSSGMesh::Mesh::ComponentType::Float32,
                           3
                       });
    }
    if (normalData.length() > 0) {
        entries.append({
                           QSSGMesh::MeshInternal::getNormalAttrName(),
                           normalData,
                           QSSGMesh::Mesh::ComponentType::Float32,
                           3
                       });
    }
    if (uv0Data.length() > 0) {
        entries.append({
                           QSSGMesh::MeshInternal::getUV0AttrName(),
                           uv0Data,
                           QSSGMesh::Mesh::ComponentType::Float32,
                           uv0Components
                       });
    }
    if (uv1Data.length() > 0) {
        entries.append({
                           QSSGMesh::MeshInternal::getUV1AttrName(),
                           uv1Data,
                           QSSGMesh::Mesh::ComponentType::Float32,
                           uv1Components
                       });
    }

    if (tangentData.length() > 0) {
        entries.append({
                           QSSGMesh::MeshInternal::getTexTanAttrName(),
                           tangentData,
                           QSSGMesh::Mesh::ComponentType::Float32,
                           3
                       });
    }

    if (binormalData.length() > 0) {
        entries.append({
                           QSSGMesh::MeshInternal::getTexBinormalAttrName(),
                           binormalData,
                           QSSGMesh::Mesh::ComponentType::Float32,
                           3
                       });
    }

    if (vertexColorData.length() > 0) {
        entries.append({
                           QSSGMesh::MeshInternal::getColorAttrName(),
                           vertexColorData,
                           QSSGMesh::Mesh::ComponentType::Float32,
                           4
                       });
    }

    if (boneIndexData.length() > 0) {
        entries.append({
                           QSSGMesh::MeshInternal::getJointAttrName(),
                           boneIndexData,
                           QSSGMesh::Mesh::ComponentType::Int32,
                           4
                       });
        entries.append({
                           QSSGMesh::MeshInternal::getWeightAttrName(),
                           boneWeightData,
                           QSSGMesh::Mesh::ComponentType::Float32,
                           4
                       });
    }
    for (uint i = 0; i < numMorphTargets; ++i) {
        if (targetPositionData[i].length() > 0) {
            entries.append({
                               QSSGMesh::MeshInternal::getTargetPositionAttrName(i),
                               targetPositionData[i],
                               QSSGMesh::Mesh::ComponentType::Float32,
                               3
                           });
        }
        if (targetNormalData[i].length() > 0) {
            entries.append({
                               QSSGMesh::MeshInternal::getTargetNormalAttrName(i),
                               targetNormalData[i],
                               QSSGMesh::Mesh::ComponentType::Float32,
                               3
                           });
        }
        if (targetTangentData[i].length() > 0) {
            entries.append({
                               QSSGMesh::MeshInternal::getTargetTangentAttrName(i),
                               targetTangentData[i],
                               QSSGMesh::Mesh::ComponentType::Float32,
                               3
                           });
        }
        if (targetBinormalData[i].length() > 0) {
            entries.append({
                               QSSGMesh::MeshInternal::getTargetBinormalAttrName(i),
                               targetBinormalData[i],
                               QSSGMesh::Mesh::ComponentType::Float32,
                               3
                           });
        }
    }

    QVector<QSSGMesh::AssetMeshSubset> subsets;
    for (const SubsetEntryData &subset : subsetData) {
        subsets.append({
                           subset.name,
                           quint32(subset.indexLength),
                           quint32(subset.indexOffset),
                           0, // the builder will calculate bounds from the position data
                           subset.lightmapWidth,
                           subset.lightmapHeight
                       });
    }

    return QSSGMesh::Mesh::fromAssetData(entries, indexBufferData, indexType, subsets);
}

QT_END_NAMESPACE
