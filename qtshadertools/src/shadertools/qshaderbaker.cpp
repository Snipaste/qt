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

#include "qshaderbaker_p.h"
#include "qspirvcompiler_p.h"
#include "qspirvshader_p.h"
#include <QFileInfo>
#include <QFile>
#include <QDebug>

QT_BEGIN_NAMESPACE

/*!
    \class QShaderBaker
    \internal
    \inmodule QtShaderTools

    \brief Compiles a GLSL/Vulkan shader into SPIR-V, translates into other
    shading languages, and gathers reflection metadata.

    QShaderBaker takes a graphics (vertex, fragment, etc.) or compute shader,
    and produces multiple - either source or bytecode - variants of it,
    together with reflection information. The results are represented by a
    QShader instance, which also provides simple and fast serialization
    and deserialization.

    \note Applications and libraries are recommended to avoid using this class
    directly. Rather, all Qt users are encouraged to rely on offline
    compilation by invoking the \c qsb command-line tool at build time. This
    tool uses QShaderBaker itself and writes the serialized version of the
    generated QShader into a file. The usage of this class should be
    restricted to cases where run time compilation cannot be avoided, such as
    when working with user-provided shader source strings.

    The input format is always assumed to be Vulkan-flavored GLSL at the
    moment. See the
    \l{https://github.com/KhronosGroup/GLSL/blob/master/extensions/khr/GL_KHR_vulkan_glsl.txt}{GL_KHR_vulkan_glsl
    specification} for an overview, keeping in mind that the Qt Shader Tools
    module is meant to be used in combination with the QRhi classes from Qt
    Rendering Hardware Interface module, and therefore a number of concepts and
    constructs (push constants, storage buffers, subpasses, etc.) are not
    applicable at the moment. Additional options may be introduced in the
    future, for example, by enabling
    \l{https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/dx-graphics-hlsl}{HLSL}
    as a source format, once HLSL to SPIR-V compilation is deemed suitable.

    The reflection metadata is retrievable from the resulting QShader by
    calling QShader::description(). This is essential when having to
    discover what set of vertex inputs and shader resources a shader expects,
    and what the layouts of those are, as many modern graphics APIs offer no
    built-in shader reflection capabilities.

    \section2 Typical Workflow

    Let's assume an application has a vertex and fragment shader like the following:

    Vertex shader:
    \snippet color.vert 0

    Fragment shader:
    \snippet color.frag 0

    To get QShader instances that can be passed as-is to a
    QRhiGraphicsPipeline, there are two options: doing the shader pack
    generation off line, or at run time.

    The former involves running the \c qsb tool:

    \badcode
    qsb --glsl "100 es,120" --hlsl 50 --msl 12 color.vert -o color.vert.qsb
    qsb --glsl "100 es,120" --hlsl 50 --msl 12 color.frag -o color.frag.qsb
    \endcode

    The example uses the translation targets as appropriate for QRhi. This
    means GLSL/ES 100, GLSL 120, HLSL Shader Model 5.0, and Metal Shading
    Language 1.2.

    Note how the command line options correspond to what can be specified via
    setGeneratedShaders(). Once the resulting files are available, they can be
    shipped with the application (typically embedded into the executable the
    the Qt Resource System), and can be loaded and passed to
    QShader::fromSerialized() at run time.

    While not shown here, \c qsb can do more: it is also able to invoke \c fxc
    on Windows or the appropriate XCode tools on macOS to compile the generated
    HLSL or Metal shader code into bytecode and include the compiled versions
    in the QShader. After a baked shader pack is written into a file, its
    contents can be examined by running \c{qsb -d} on it. Run \c qsb with
    \c{--help} for more information.

    The alternative approach is to perform the same at run time. This involves
    creating a QShaderBaker instance, calling setSourceFileName(), and then
    setting up the translation targets via setGeneratedShaders():

    \badcode
        baker.setGeneratedShaderVariants({ QShader::StandardShader });
        QList<QShaderBaker::GeneratedShader> targets;
        targets.append({ QShader::SpirvShader, QShaderVersion(100) });
        targets.append({ QShader::GlslShader, QShaderVersion(100, QShaderVersion::GlslEs) });
        targets.append({ QShader::SpirvShader, QShaderVersion(120) });
        targets.append({ QShader::HlslShader, QShaderVersion(50) });
        targets.append({ QShader::MslShader, QShaderVersion(12) });
        baker.setGeneratedShaders(targets);
        QShader shaders = baker.bake();
        if (!shaders.isValid())
            qWarning() << baker.errorMessage();
    \endcode

    \sa QShader
 */

struct QShaderBakerPrivate
{
    bool readFile(const QString &fn);
    QPair<QByteArray, QByteArray> compile();
    QByteArray perTargetDefines(const QShaderBaker::GeneratedShader &key);

    QString sourceFileName;
    QByteArray source;
    QShader::Stage stage;
    QList<QShaderBaker::GeneratedShader> reqVersions;
    QList<QShader::Variant> variants;
    QByteArray preamble;
    int batchLoc = 7;
    bool perTargetEnabled = false;
    QShaderBaker::SpirvOptions spirvOptions;
    QSpirvCompiler compiler;
    QString errorMessage;
};

bool QShaderBakerPrivate::readFile(const QString &fn)
{
    QFile f(fn);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning("QShaderBaker: Failed to open %s", qPrintable(fn));
        return false;
    }
    source = f.readAll();
    sourceFileName = fn;
    return true;
}

/*!
    Constructs a new QShaderBaker.
 */
QShaderBaker::QShaderBaker()
    : d(new QShaderBakerPrivate)
{
}

/*!
    Destructor.
 */
QShaderBaker::~QShaderBaker()
{
    delete d;
}

/*!
    Sets the name of the shader source file to \a fileName. This is the file
    that will be read when calling bake(). The shader stage is deduced
    automatically from the file extension. When this is not desired or not
    possible, use the overload with the stage argument instead.

    The supported file extensions are:
    \list
    \li \c{.vert} - vertex shader
    \li \c{.frag} - fragment (pixel) shader
    \li \c{.tesc} - tessellation control (hull) shader
    \li \c{.tese} - tessellation evaluation (domain) shader
    \li \c{.geom} - geometry shader
    \li \c{.comp} - compute shader
    \endlist
 */
void QShaderBaker::setSourceFileName(const QString &fileName)
{
    if (!d->readFile(fileName))
        return;

    const QString suffix = QFileInfo(fileName).suffix();
    if (suffix == QStringLiteral("vert")) {
        d->stage = QShader::VertexStage;
    } else if (suffix == QStringLiteral("frag")) {
        d->stage = QShader::FragmentStage;
    } else if (suffix == QStringLiteral("tesc")) {
        d->stage = QShader::TessellationControlStage;
    } else if (suffix == QStringLiteral("tese")) {
        d->stage = QShader::TessellationEvaluationStage;
    } else if (suffix == QStringLiteral("geom")) {
        d->stage = QShader::GeometryStage;
    } else if (suffix == QStringLiteral("comp")) {
        d->stage = QShader::ComputeStage;
    } else {
        qWarning("QShaderBaker: Unknown shader stage, defaulting to vertex");
        d->stage = QShader::VertexStage;
    }
}

/*!
    Sets the name of the shader source file to \a fileName. This is the file
    that will be read when calling bake(). The shader stage is specified by \a
    stage.
 */
void QShaderBaker::setSourceFileName(const QString &fileName, QShader::Stage stage)
{
    if (d->readFile(fileName))
        d->stage = stage;
}

/*!
    Sets the source \a device. This allows using any QIODevice instead of just
    files. \a stage specifies the shader stage, while the optional \a fileName
    contains a filename that is used in the error messages.
 */
void QShaderBaker::setSourceDevice(QIODevice *device, QShader::Stage stage, const QString &fileName)
{
    setSourceString(device->readAll(), stage, fileName);
}

/*!
    Sets the input shader \a sourceString. \a stage specified the shader stage,
    while the optional \a fileName contains a filename that is used in the
    error messages.
 */
void QShaderBaker::setSourceString(const QByteArray &sourceString, QShader::Stage stage, const QString &fileName)
{
    d->sourceFileName = fileName; // for error messages, include handling, etc.
    d->source = sourceString;
    d->stage = stage;
}

/*!
    \typedef QShaderBaker::GeneratedShader

    Synonym for QPair<QShader::Source, QShaderVersion>.
*/

/*!
    Specifies what kind of shaders to compile or translate to. Nothing is
    generated by default so calling this function before bake() is mandatory

    \note when this function is not called or \a v is empty or contains only invalid
    entries, the resulting QShader will be empty and thus invalid.

    For example, the minimal possible baking target is SPIR-V, without any
    additional translations to other languages. To request this, do:

    \badcode
        baker.setGeneratedShaders({ QShader::SpirvShader, QShaderVersion(100) });
    \endcode

    \note QShaderBaker only handles the SPIR-V and human-readable source
    targets. Further compilation into API-specific intermediate formats, such
    as QShader::DxbcShader or QShader::MetalLibShader is implemented by the
    \c qsb command-line tool, and is not part of the QShaderBaker runtime API.
 */
void QShaderBaker::setGeneratedShaders(const QList<GeneratedShader> &v)
{
    d->reqVersions = v;
}

/*!
    Specifies which shader variants are generated. Each shader version can have
    multiple variants in the resulting QShader.

    In most cases \a v contains a single entry, QShader::StandardShader.

    \note when no variants are set, the resulting QShader will be empty and
    thus invalid.
 */
void QShaderBaker::setGeneratedShaderVariants(const QList<QShader::Variant> &v)
{
    d->variants = v;
}

/*!
    Specifies a custom \a preamble that is processed before the normal shader
    code.

    This is more than just prepending to the source string: the validity of the
    GLSL version directive, which is required to be placed before everything
    else, is not affected. Line numbers in the reported error messages also
    remain unchanged, ignoring the contents given in the \a preamble.

    One use case for preambles is to transparently insert dynamically generated
    \c{#define} statements.
 */
void QShaderBaker::setPreamble(const QByteArray &preamble)
{
    d->preamble = preamble;
}

/*!
    When generating a QShader::BatchableVertexShader variant, \a location
    specifies the input location for the inserted vertex input. The value is by
    default 7 and needs to be overridden only if the vertex shader already uses
    input location 7.
 */
void QShaderBaker::setBatchableVertexShaderExtraInputLocation(int location)
{
    d->batchLoc = location;
}

/*!
    Sets per-target compilation to \a enable. By default this is disabled,
    meaning that the Vulkan/GLSL source is compiled to SPIR-V once per variant.
    (so once by default, twice if it is a vertex shader and the Batchable
    variant as requested as well). The resulting SPIR-V is then translated to
    the various target languages (GLSL, HLSL, MSL).

    In per-target compilation mode, there is a separate GLSL to SPIR-V
    compilation step for each target, meaning for each GLSL/HLSL/MSL version
    requested via setGeneratedShaders(). The input source is the same, but with
    target-specific preprocessor defines inserted. This is significantly more
    time consuming, but allows applications to provide a single shader and use
    \c{#ifdef} blocks to differentiate. When this mode is disabled, the only
    way to achieve the same is to provide multiple versions of the shader file,
    process each separately, ship {.qsb} files for each, and choose the right
    file based on run time logic.

    The following macros will be automatically defined in this mode. Note that
    the macros are always tied to shading languages, not graphics APIs.

    \list

    \li \c{QSHADER_SPIRV} - defined when targeting SPIR-V (to be consumed,
    typically, by Vulkan).

    \li \c{QSHADER_SPIRV_VERSION} - the targeted SPIR-V version number, such as
    \c 100.

    \li \c{QSHADER_GLSL} - defined when targeting GLSL or GLSL ES (to be
    consumed, typically, by OpenGL or OpenGL ES)

    \li \c{QSHADER_GLSL_VERSION} - the targeted GLSL or GLSL ES version number,
    such as \c 100, \c 300, or \c 330.

    \li \c{QSHADER_GLSL_ES} - defined only when targeting GLSL ES

    \li \c{QSHADER_HLSL} - defined when targeting HLSL (to be consumed,
    typically, by Direct 3D)

    \li \c{QSHADER_HLSL_VERSION} - the targeted HLSL shader model version, such
    as \c 50

    \li \c{QSHADER_MSL} - defined when targeting the Metal Shading Language (to
    be consumed, typically, by Metal)

    \li \c{QSHADER_MSL_VERSION} - the targeted MSL version, such as \c 12 or
    \c 20.

    \endlist

    This allows writing shader code like the following.

    \badcode
      #if QSHADER_HLSL || QSHADER_MSL
      vec2 uv = vec2(uv_coord.x, 1.0 - uv_coord.y);
      #else
      vec2 uv = uv_coord;
      #endif
    \endcode

    \note Version numbers follow the GLSL-inspired QShaderVersion syntax and
    thus are a single integer always.

    \note There is only one QShaderDescription per QShader, no matter how many
    individual targets there are. Therefore members of uniform blocks, vertex
    inputs, etc. must not be made conditional using the macros described above.

    \warning Be aware of the differences between the concepts of graphics APIs
    and shading languages. QShaderBaker and the related tools work strictly
    with the concept of shading languages, ignoring how the results are
    consumed afterwards. Therefore, if the higher layers in the Qt graphics
    stack one day start using SPIR-V also for an API other than Vulkan, the
    assumption that QSHADER_SPIRV implies Vulkan will no longer hold.
 */
void QShaderBaker::setPerTargetCompilation(bool enable)
{
    d->perTargetEnabled = enable;
}

void QShaderBaker::setSpirvOptions(SpirvOptions options)
{
    d->spirvOptions = options;
}

inline size_t qHash(const QShaderBaker::GeneratedShader &k, size_t seed = 0)
{
    return qHash(k.first, seed) ^ k.second.version();
}

QPair<QByteArray, QByteArray> QShaderBakerPrivate::compile()
{
    QSpirvCompiler::Flags flags;
    if (spirvOptions.testFlag(QShaderBaker::SpirvOption::GenerateFullDebugInfo))
        flags |= QSpirvCompiler::FullDebugInfo;

    compiler.setFlags(flags);
    const QByteArray spirvBin = compiler.compileToSpirv();
    if (spirvBin.isEmpty()) {
        errorMessage = compiler.errorMessage();
        return {};
    }
    QByteArray batchableSpirvBin;
    if (stage == QShader::VertexStage && variants.contains(QShader::BatchableVertexShader)) {
        compiler.setFlags(flags | QSpirvCompiler::RewriteToMakeBatchableForSG);
        compiler.setSGBatchingVertexInputLocation(batchLoc);
        batchableSpirvBin = compiler.compileToSpirv();
        if (batchableSpirvBin.isEmpty()) {
            errorMessage = compiler.errorMessage();
            return {};
        }
    }
    return { spirvBin, batchableSpirvBin };
}

QByteArray QShaderBakerPrivate::perTargetDefines(const QShaderBaker::GeneratedShader &key)
{
    QByteArray preamble;
    switch (key.first) {
    case QShader::SpirvShader:
        preamble += QByteArrayLiteral("\n#define QSHADER_SPIRV 1\n#define QSHADER_SPIRV_VERSION ");
        preamble += QByteArray::number(key.second.version());
        preamble += QByteArrayLiteral("\n");
        break;
    case QShader::GlslShader:
        preamble += QByteArrayLiteral("\n#define QSHADER_GLSL 1\n#define QSHADER_GLSL_VERSION ");
        preamble += QByteArray::number(key.second.version());
        if (key.second.flags().testFlag(QShaderVersion::GlslEs))
            preamble += QByteArrayLiteral("\n#define QSHADER_GLSL_ES 1");
        preamble += QByteArrayLiteral("\n");
        break;
    case QShader::HlslShader:
        preamble += QByteArrayLiteral("\n#define QSHADER_HLSL 1\n#define QSHADER_HLSL_VERSION ");
        preamble += QByteArray::number(key.second.version());
        preamble += QByteArrayLiteral("\n");
        break;
    case QShader::MslShader:
        preamble += QByteArrayLiteral("\n#define QSHADER_MSL 1\n#define QSHADER_MSL_VERSION ");
        preamble += QByteArray::number(key.second.version());
        preamble += QByteArrayLiteral("\n");
        break;
    default:
        Q_UNREACHABLE();
    }
    return preamble;
}

/*!
    Runs the compilation and translation process.

    \return a QShader instance. To check if the process was successful,
    call QShader::isValid(). When that indicates \c false, call
    errorMessage() to retrieve the log.

    This is an expensive operation. When calling this from applications, it can
    be advisable to do it on a separate thread.

    \note QShaderBaker instances are reusable: after calling bake(), the same
    instance can be used with different inputs again. However, a QShaderBaker
    instance should only be used on one single thread during its lifetime.
 */
QShader QShaderBaker::bake()
{
    d->errorMessage.clear();

    if (d->source.isEmpty()) {
        d->errorMessage = QLatin1String("QShaderBaker: No source specified");
        return QShader();
    }

    d->compiler.setSourceString(d->source, d->stage, d->sourceFileName);

    // Normally one entry, for QShader::SpirvShader only. However, in
    // compile-per-target mode there is a separate SPIR-V binary generated for
    // each target (so for each GLSL/HLSL/MSL version requested).
    QHash<GeneratedShader, QByteArray> spirv;
    QHash<GeneratedShader, QByteArray> batchableSpirv;
    const auto compileSpirvAndBatchable = [this, &spirv, &batchableSpirv](const GeneratedShader &key) {
        const QPair<QByteArray, QByteArray> bin = d->compile();
        if (bin.first.isEmpty())
            return false;
        spirv.insert(key, bin.first);
        if (!bin.second.isEmpty())
            batchableSpirv.insert(key, bin.second);
        return true;
    };

    if (!d->perTargetEnabled) {
        d->compiler.setPreamble(d->preamble);
        if (!compileSpirvAndBatchable({ QShader::SpirvShader, {} }))
            return QShader();
    } else {
        // per-target compilation. the value here comes from the varying
        // preamble (and so preprocessor defines)
        for (GeneratedShader req: d->reqVersions) {
            d->compiler.setPreamble(d->preamble + d->perTargetDefines(req));
            if (!compileSpirvAndBatchable(req))
                return QShader();
        }
    }

    // Now spirv, and, if in use, batchableSpirv, contain at least one,
    // optionally more SPIR-V binaries.
    Q_ASSERT(!spirv.isEmpty() && (d->perTargetEnabled || spirv.count() == 1));

    QShader bs;
    bs.setStage(d->stage);

    QSpirvShader spirvShader;
    QSpirvShader batchableSpirvShader;
    // The QShaderDescription can be different for variants (we just have a
    // hardcoded rule to pick one), but cannot differ for targets (in
    // per-target mode, hence we can just pick the first SPIR-V binary and
    // generate the reflection data based on that)
    spirvShader.setSpirvBinary(spirv.constKeyValueBegin()->second);
    if (batchableSpirv.isEmpty()) {
        bs.setDescription(spirvShader.shaderDescription());
    } else {
        batchableSpirvShader.setSpirvBinary(batchableSpirv.constKeyValueBegin()->second);
        // prefer the batchable's reflection info with _qt_order and such present
        bs.setDescription(batchableSpirvShader.shaderDescription());
    }

    for (const GeneratedShader &req: d->reqVersions) {
        for (const QShader::Variant &v : d->variants) {
            if (v == QShader::BatchableVertexShader && d->stage != QShader::VertexStage)
                continue;
            QSpirvShader *currentSpirvShader = nullptr;
            if (d->perTargetEnabled) {
                // This is expensive too, in addition to the multiple
                // compilation rounds, but opting in to per-target mode is a
                // careful, conscious choice (hopefully), so it's fine.
                if (v == QShader::BatchableVertexShader)
                    batchableSpirvShader.setSpirvBinary(batchableSpirv[req]);
                else
                    spirvShader.setSpirvBinary(spirv[req]);
            }
            if (v == QShader::BatchableVertexShader)
                currentSpirvShader = &batchableSpirvShader;
            else
                currentSpirvShader = &spirvShader;
            Q_ASSERT(currentSpirvShader);
            Q_ASSERT(!currentSpirvShader->spirvBinary().isEmpty());
            const QShaderKey key(req.first, req.second, v);
            QShaderCode shader;
            shader.setEntryPoint(QByteArrayLiteral("main"));
            switch (req.first) {
            case QShader::SpirvShader:
                if (d->spirvOptions.testFlag(QShaderBaker::SpirvOption::StripDebugAndVarInfo)) {
                    QString errorMsg;
                    const QByteArray strippedSpirv = currentSpirvShader->remappedSpirvBinary(QSpirvShader::StripOnly, &errorMsg);
                    if (strippedSpirv.isEmpty()) {
                        d->errorMessage = errorMsg;
                        return QShader();
                    }
                    shader.setShader(strippedSpirv);
                } else {
                    shader.setShader(currentSpirvShader->spirvBinary());
                }
                break;
            case QShader::GlslShader:
            {
                QSpirvShader::GlslFlags flags;
                if (req.second.flags().testFlag(QShaderVersion::GlslEs))
                    flags |= QSpirvShader::GlslEs;
                shader.setShader(currentSpirvShader->translateToGLSL(req.second.version(), flags));
                if (shader.shader().isEmpty()) {
                    d->errorMessage = currentSpirvShader->translationErrorMessage();
                    return QShader();
                }
            }
                break;
            case QShader::HlslShader:
            {
                QShader::NativeResourceBindingMap nativeBindings;
                shader.setShader(currentSpirvShader->translateToHLSL(req.second.version(), &nativeBindings));
                if (shader.shader().isEmpty()) {
                    d->errorMessage = currentSpirvShader->translationErrorMessage();
                    return QShader();
                }
                bs.setResourceBindingMap(key, nativeBindings);
            }
                break;
            case QShader::MslShader:
            {
                QShader::NativeResourceBindingMap nativeBindings;
                shader.setShader(currentSpirvShader->translateToMSL(req.second.version(), &nativeBindings));
                if (shader.shader().isEmpty()) {
                    d->errorMessage = currentSpirvShader->translationErrorMessage();
                    return QShader();
                }
                shader.setEntryPoint(QByteArrayLiteral("main0"));
                bs.setResourceBindingMap(key, nativeBindings);
            }
                break;
            default:
                Q_UNREACHABLE();
            }
            bs.setShader(key, shader);
        }
    }

    return bs;
}

/*!
    \return the error message from the last bake() run, or an empty string if
    there was no error.

    \note Errors include file read errors, compilation, and translation
    failures. Not requesting any targets or variants does not count as an error
    even though the resulting QShader is invalid.
 */
QString QShaderBaker::errorMessage() const
{
    return d->errorMessage;
}

QT_END_NAMESPACE
