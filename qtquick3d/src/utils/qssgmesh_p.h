/****************************************************************************
**
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

#ifndef QSSGMESHUTILITIES_P_H
#define QSSGMESHUTILITIES_P_H

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

#include <QtQuick3DUtils/private/qtquick3dutilsglobal_p.h>

#include <QtQuick3DUtils/private/qssgbounds3_p.h>

#include <QtQuick3DUtils/private/qssgrenderbasetypes_p.h>

#include <QtCore/QString>
#include <QtCore/QByteArray>
#include <QtCore/QIODevice>

QT_BEGIN_NAMESPACE

namespace QSSGMesh {

struct AssetVertexEntry;
struct AssetMeshSubset;
struct RuntimeMeshData;

class Q_QUICK3DUTILS_EXPORT Mesh
{
public:
    enum class DrawMode {
        Points = 1,
        LineStrip,
        LineLoop,
        Lines,
        TriangleStrip,
        TriangleFan,
        Triangles
    };

    enum class Winding {
        Clockwise = 1,
        CounterClockwise
    };

    enum class ComponentType {
        UnsignedInt8 = 1,
        Int8,
        UnsignedInt16,
        Int16,
        UnsignedInt32,
        Int32,
        UnsignedInt64,
        Int64,
        Float16,
        Float32,
        Float64
    };

    struct VertexBufferEntry {
        ComponentType componentType = ComponentType::Float32;
        quint32 componentCount = 0;
        quint32 offset = 0;
        QByteArray name;

        QSSGRenderVertexBufferEntry toRenderVertexBufferEntry() const {
            return QSSGRenderVertexBufferEntry(name,
                                               QSSGRenderComponentType(componentType),
                                               componentCount,
                                               offset);
        }
    };

    struct VertexBuffer {
        quint32 stride = 0;
        QVector<VertexBufferEntry> entries;
        QByteArray data;
    };

    struct IndexBuffer {
        ComponentType componentType = ComponentType::UnsignedInt32;
        QByteArray data;
    };

    struct SubsetBounds {
        QVector3D min;
        QVector3D max;
    };

    struct Subset {
        QString name;
        SubsetBounds bounds;
        quint32 count = 0;
        quint32 offset = 0;
        QSize lightmapSizeHint;
    };

    // can just return by value (big data is all implicitly shared)
    VertexBuffer vertexBuffer() const { return m_vertexBuffer; }
    IndexBuffer indexBuffer() const { return m_indexBuffer; }
    QVector<Subset> subsets() const { return m_subsets; }

    // id 0 == first, otherwise has to match
    static Mesh loadMesh(QIODevice *device, quint32 id = 0);

    static QMap<quint32, Mesh> loadAll(QIODevice *device);

    static Mesh fromAssetData(const QVector<AssetVertexEntry> &vbufEntries,
                              const QByteArray &indexBufferData,
                              ComponentType indexComponentType,
                              const QVector<AssetMeshSubset> &subsets);

    static Mesh fromRuntimeData(const RuntimeMeshData &data,
                                QString *error);

    bool isValid() const { return !m_subsets.isEmpty(); }

    DrawMode drawMode() const { return m_drawMode; }
    Winding winding() const { return m_winding; }

    // id 0 == generate new id; otherwise uses it as-is, and must be an unused one
    quint32 save(QIODevice *device, quint32 id = 0) const;

private:
    DrawMode m_drawMode = DrawMode::Triangles;
    Winding m_winding = Winding::CounterClockwise;
    VertexBuffer m_vertexBuffer;
    IndexBuffer m_indexBuffer;
    QVector<Subset> m_subsets;
    friend struct MeshInternal;
};

struct Q_QUICK3DUTILS_EXPORT AssetVertexEntry // for asset importer plugins (Assimp, FBX)
{
    QByteArray name;
    QByteArray data;
    Mesh::ComponentType componentType = Mesh::ComponentType::Float32;
    quint32 componentCount = 0;
};

struct Q_QUICK3DUTILS_EXPORT AssetMeshSubset // for asset importer plugins (Assimp, FBX)
{
    QString name;
    quint32 count = 0;
    quint32 offset = 0;
    quint32 boundsPositionEntryIndex = std::numeric_limits<quint32>::max();
    quint32 lightmapWidth = 0;
    quint32 lightmapHeight = 0;
};

struct Q_QUICK3DUTILS_EXPORT RuntimeMeshData // for custom geometry (QQuick3DGeometry, QSSGRenderGeometry)
{
    struct Attribute {
        enum Semantic {
            IndexSemantic = 0,
            PositionSemantic,                       // attr_pos
            NormalSemantic,                         // attr_norm
            TexCoordSemantic,                       // attr_uv0
            TangentSemantic,                        // attr_textan
            BinormalSemantic,                       // attr_binormal
            JointSemantic,                          // attr_joints
            WeightSemantic,                         // attr_weights
            ColorSemantic,                          // attr_color
            TargetPositionSemantic,                 // attr_tpos0
            TargetNormalSemantic,                   // attr_tnorm0
            TargetTangentSemantic,                  // attr_ttan0
            TargetBinormalSemantic,                 // attr_tbinorm0
            TexCoord1Semantic,                      // attr_uv1
            TexCoord0Semantic = TexCoordSemantic    // attr_uv0
        };

        Semantic semantic = PositionSemantic;
        Mesh::ComponentType componentType = Mesh::ComponentType::Float32;
        int offset = 0;

        int componentCount() const {
            switch (semantic) {
            case IndexSemantic:             return 1;
            case PositionSemantic:          return 3;
            case NormalSemantic:            return 3;
            case TexCoord0Semantic:         return 2;
            case TexCoord1Semantic:         return 2;
            case TangentSemantic:           return 3;
            case BinormalSemantic:          return 3;
            case JointSemantic:             return 4;
            case WeightSemantic:            return 4;
            case ColorSemantic:             return 4;
            case TargetPositionSemantic:    return 3;
            case TargetNormalSemantic:      return 3;
            case TargetTangentSemantic:     return 3;
            case TargetBinormalSemantic:    return 3;
            default:
                Q_ASSERT(false);
                return 0;
            }
        }
    };

    static const int MAX_ATTRIBUTES = 16;

    void clear()
    {
        m_vertexBuffer.clear();
        m_indexBuffer.clear();
        m_subsets.clear();
        m_attributeCount = 0;
        m_primitiveType = Mesh::DrawMode::Triangles;
    }

    QByteArray m_vertexBuffer;
    QByteArray m_indexBuffer;
    QVector<Mesh::Subset> m_subsets;

    Attribute m_attributes[MAX_ATTRIBUTES];
    int m_attributeCount = 0;
    Mesh::DrawMode m_primitiveType = Mesh::DrawMode::Triangles;
    int m_stride = 0;
};

struct Q_QUICK3DUTILS_EXPORT MeshInternal
{
    struct MultiMeshInfo {
        quint32 fileId = 0;
        quint32 fileVersion = 0;
        QMap<quint32, quint64> meshEntries;
        static const quint32 FILE_ID = 555777497;
        static const quint32 FILE_VERSION = 1;
        bool isValid() const {
            return fileId == FILE_ID && fileVersion == FILE_VERSION;
        }
        static MultiMeshInfo withDefaults() {
            return { FILE_ID, FILE_VERSION, {} };
        }
    };

    struct MeshDataHeader {
        quint32 fileId = 0;
        quint16 fileVersion = 0;
        quint16 flags = 0;
        quint32 sizeInBytes = 0;

        static const quint32 FILE_ID = 3365961549;

        // Version 3 and 4 is only different in the "offset" values that are
        // all zeroes in version 4 because they are not used by the new mesh
        // reader. So to support both with the new loader, no branching is
        // needed at all, it just needs to accept both versions.
        static const quint32 LEGACY_MESH_FILE_VERSION = 3;
        // Version 5 differs from 4 with the added lightmapSizeHint per subset.
        // This needs branching in the deserializer.
        static const quint32 FILE_VERSION = 5;

        static MeshDataHeader withDefaults() {
            return { FILE_ID, FILE_VERSION, 0, 0 };
        }

        bool isValid() const {
            return fileId == FILE_ID
                    && fileVersion <= FILE_VERSION
                    && fileVersion >= LEGACY_MESH_FILE_VERSION;
        }

        bool hasLightmapSizeHint() const {
            return fileVersion >= 5;
        }
    };

    struct MeshOffsetTracker {
        quint32 startOffset = 0;
        quint32 byteCounter = 0;

        MeshOffsetTracker(int offset)
            : startOffset(offset) {}

        int offset() {
            return startOffset + byteCounter;
        }

        quint32 alignedAdvance(int advanceAmount) {
            advance(advanceAmount);
            quint32 alignmentAmount = 4 - (byteCounter % 4);
            byteCounter += alignmentAmount;
            return alignmentAmount;
        }

        void advance(int advanceAmount) {
            byteCounter += advanceAmount;
        }
    };

    struct Subset {
        QByteArray rawNameUtf16;
        quint32 nameLength = 0;
        Mesh::SubsetBounds bounds;
        quint32 offset = 0;
        quint32 count = 0;
        QSize lightmapSizeHint;

        Mesh::Subset toMeshSubset() const {
            Mesh::Subset subset;
            if (nameLength > 0)
                subset.name = QString::fromUtf16(reinterpret_cast<const char16_t *>(rawNameUtf16.constData()), nameLength - 1);
            subset.bounds.min = bounds.min;
            subset.bounds.max = bounds.max;
            subset.count = count;
            subset.offset = offset;
            subset.lightmapSizeHint = lightmapSizeHint;
            return subset;
        }
    };

    static MultiMeshInfo readFileHeader(QIODevice *device);
    static void writeFileHeader(QIODevice *device, const MultiMeshInfo &meshFileInfo);
    static quint64 readMeshData(QIODevice *device, quint64 offset, Mesh *mesh, MeshDataHeader *header);
    static void writeMeshHeader(QIODevice *device, const MeshDataHeader &header);
    static quint64 writeMeshData(QIODevice *device, const Mesh &mesh);

    static int byteSizeForComponentType(Mesh::ComponentType componentType) {
        switch (componentType) {
        case Mesh::ComponentType::UnsignedInt8:  return 1;
        case Mesh::ComponentType::Int8:  return 1;
        case Mesh::ComponentType::UnsignedInt16: return 2;
        case Mesh::ComponentType::Int16: return 2;
        case Mesh::ComponentType::UnsignedInt32: return 4;
        case Mesh::ComponentType::Int32: return 4;
        case Mesh::ComponentType::UnsignedInt64: return 8;
        case Mesh::ComponentType::Int64: return 8;
        case Mesh::ComponentType::Float16: return 2;
        case Mesh::ComponentType::Float32: return 4;
        case Mesh::ComponentType::Float64: return 8;
        default:
            Q_ASSERT(false);
            return 0;
        }
    }

    static const char *getPositionAttrName() { return "attr_pos"; }
    static const char *getNormalAttrName() { return "attr_norm"; }
    static const char *getUV0AttrName() { return "attr_uv0"; }
    static const char *getUV1AttrName() { return "attr_uv1"; }
    static const char *getTexTanAttrName() { return "attr_textan"; }
    static const char *getTexBinormalAttrName() { return "attr_binormal"; }
    static const char *getColorAttrName() { return "attr_color"; }
    static const char *getJointAttrName() { return "attr_joints"; }
    static const char *getWeightAttrName() { return "attr_weights"; }
    static const char *getMorphTargetAttrNamePrefix() { return "attr_t"; }
    static const char *getTargetPositionAttrName(int idx)
    {
        switch (idx) {
            case 0:
                return "attr_tpos0";
            case 1:
                return "attr_tpos1";
            case 2:
                return "attr_tpos2";
            case 3:
                return "attr_tpos3";
            case 4:
                return "attr_tpos4";
            case 5:
                return "attr_tpos5";
            case 6:
                return "attr_tpos6";
            case 7:
                return "attr_tpos7";
        }
        return "attr_unsupported";
    }
    static const char *getTargetNormalAttrName(int idx)
    {
        switch (idx) {
            case 0:
                return "attr_tnorm0";
            case 1:
                return "attr_tnorm1";
            case 2:
                return "attr_tnorm2";
            case 3:
                return "attr_tnorm3";
        }
        return "attr_unsupported";
    }
    static const char *getTargetTangentAttrName(int idx)
    {
        switch (idx) {
            case 0:
                return "attr_ttan0";
            case 1:
                return "attr_ttan1";
        }
        return "attr_unsupported";
    }
    static const char *getTargetBinormalAttrName(int idx)
    {
        switch (idx) {
            case 0:
                return "attr_tbinorm0";
            case 1:
                return "attr_tbinorm1";
        }
        return "attr_unsupported";
    }

    static QSSGBounds3 calculateSubsetBounds(const Mesh::VertexBufferEntry &entry,
                                             const QByteArray &vertexBufferData,
                                             quint32 vertexBufferStride,
                                             const QByteArray &indexBufferData,
                                             Mesh::ComponentType indexComponentType,
                                             quint32 subsetCount,
                                             quint32 subsetOffset);
};

} // namespace QSSGMesh

QT_END_NAMESPACE

#endif // QSSGMESHUTILITIES_P_H
