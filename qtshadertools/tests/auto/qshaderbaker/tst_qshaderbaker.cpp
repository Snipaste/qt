/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
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
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <QFile>
#include <QtShaderTools/private/qshaderbaker_p.h>
#include <QtGui/private/qshaderdescription_p.h>
#include <QtGui/private/qshader_p.h>

class tst_QShaderBaker : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();
    void emptyCompile();
    void noFileCompile();
    void noTargetsCompile();
    void noVariantsCompile();
    void simpleCompile();
    void simpleCompileNoSpirvSpecified();
    void simpleCompileCheckResults();
    void simpleCompileFromDevice();
    void simpleCompileFromString();
    void multiCompile();
    void reuse();
    void compileError();
    void translateError();
    void genVariants();
    void defines();
    void reflectArrayOfStructInBlock();
    void reflectCombinedImageSampler();
    void mslNativeBindingMap();
    void hlslNativeBindingMap();
    void reflectArraysOfSamplers();
    void perTargetCompileMode();
    void serializeDeserialize();
    void spirvOptions();
};

void tst_QShaderBaker::initTestCase()
{
}

void tst_QShaderBaker::cleanup()
{
}

void tst_QShaderBaker::emptyCompile()
{
    QShaderBaker baker;
    QShader s = baker.bake();
    QVERIFY(!s.isValid());
    QVERIFY(!baker.errorMessage().isEmpty());
    qDebug() << baker.errorMessage();
}

void tst_QShaderBaker::noFileCompile()
{
    QShaderBaker baker;
    baker.setSourceFileName(QLatin1String(":/data/nonexistant.vert"));
    QShader s = baker.bake();
    QVERIFY(!s.isValid());
    QVERIFY(!baker.errorMessage().isEmpty());
    qDebug() << baker.errorMessage();
}

void tst_QShaderBaker::noTargetsCompile()
{
    QShaderBaker baker;
    baker.setSourceFileName(QLatin1String(":/data/color.vert"));
    QShader s = baker.bake();
    // an empty shader pack is invalid
    QVERIFY(!s.isValid());
    // not an error from the baker's point of view however
    QVERIFY(baker.errorMessage().isEmpty());
}

void tst_QShaderBaker::noVariantsCompile()
{
    QShaderBaker baker;
    baker.setSourceFileName(QLatin1String(":/data/color.vert"));
    QList<QShaderBaker::GeneratedShader> targets;
    targets.append({ QShader::SpirvShader, QShaderVersion(100) });
    baker.setGeneratedShaders(targets);
    QShader s = baker.bake();
    // an empty shader pack is invalid
    QVERIFY(!s.isValid());
    // not an error from the baker's point of view however
    QVERIFY(baker.errorMessage().isEmpty());
}

void tst_QShaderBaker::simpleCompile()
{
    QShaderBaker baker;
    baker.setSourceFileName(QLatin1String(":/data/color.vert"));
    baker.setGeneratedShaderVariants({ QShader::StandardShader });
    QList<QShaderBaker::GeneratedShader> targets;
    targets.append({ QShader::SpirvShader, QShaderVersion(100) });
    baker.setGeneratedShaders(targets);
    QShader s = baker.bake();
    QVERIFY(s.isValid());
    QVERIFY(baker.errorMessage().isEmpty());
    QCOMPARE(s.availableShaders().count(), 1);
    QVERIFY(s.availableShaders().contains(QShaderKey(QShader::SpirvShader, QShaderVersion(100))));
}

void tst_QShaderBaker::simpleCompileNoSpirvSpecified()
{
    QShaderBaker baker;
    baker.setSourceFileName(QLatin1String(":/data/color.vert"));
    baker.setGeneratedShaderVariants({ QShader::StandardShader });
    QList<QShaderBaker::GeneratedShader> targets;
    targets.append({ QShader::GlslShader, QShaderVersion(330) });
    baker.setGeneratedShaders(targets);
    QShader s = baker.bake();
    QVERIFY(s.isValid());
    QVERIFY(baker.errorMessage().isEmpty());
    QCOMPARE(s.availableShaders().count(), 1);
    QVERIFY(s.availableShaders().contains(QShaderKey(QShader::GlslShader, QShaderVersion(330))));
    QVERIFY(s.shader(s.availableShaders().first()).shader().contains(QByteArrayLiteral("#version 330")));
}

void tst_QShaderBaker::simpleCompileCheckResults()
{
    QShaderBaker baker;
    baker.setSourceFileName(QLatin1String(":/data/color.vert"));
    baker.setGeneratedShaderVariants({ QShader::StandardShader });
    QList<QShaderBaker::GeneratedShader> targets;
    targets.append({ QShader::SpirvShader, QShaderVersion(100) });
    baker.setGeneratedShaders(targets);
    QShader s = baker.bake();
    QVERIFY(s.isValid());
    QVERIFY(baker.errorMessage().isEmpty());
    QCOMPARE(s.availableShaders().count(), 1);

    const QShaderCode shader = s.shader(QShaderKey(QShader::SpirvShader,
                                                   QShaderVersion(100)));
    QVERIFY(!shader.shader().isEmpty());
    QCOMPARE(shader.entryPoint(), QByteArrayLiteral("main"));

    const QShaderDescription desc = s.description();
    QVERIFY(desc.isValid());
    QCOMPARE(desc.inputVariables().count(), 2);
    for (const QShaderDescription::InOutVariable &v : desc.inputVariables()) {
        switch (v.location) {
        case 0:
            QCOMPARE(v.name, QByteArrayLiteral("position"));
            QCOMPARE(v.type, QShaderDescription::Vec4);
            break;
        case 1:
            QCOMPARE(v.name, QByteArrayLiteral("color"));
            QCOMPARE(v.type, QShaderDescription::Vec3);
            break;
        default:
            QFAIL(qPrintable(QStringLiteral("Unexpected location value: %1").arg(v.location)));
            break;
        }
    }
    QCOMPARE(desc.outputVariables().count(), 1);
    for (const QShaderDescription::InOutVariable &v : desc.outputVariables()) {
        switch (v.location) {
        case 0:
            QCOMPARE(v.name, QByteArrayLiteral("v_color"));
            QCOMPARE(v.type, QShaderDescription::Vec3);
            break;
        default:
            QFAIL(qPrintable(QStringLiteral("Unexpected location value: %1").arg(v.location)));
            break;
        }
    }
    QCOMPARE(desc.uniformBlocks().count(), 1);
    const QShaderDescription::UniformBlock blk = desc.uniformBlocks().first();
    QCOMPARE(blk.blockName, QByteArrayLiteral("buf"));
    QCOMPARE(blk.structName, QByteArrayLiteral("ubuf"));
    QCOMPARE(blk.size, 68);
    QCOMPARE(blk.binding, 0);
    QCOMPARE(blk.descriptorSet, 0);
    QCOMPARE(blk.members.count(), 2);
    for (int i = 0; i < blk.members.count(); ++i) {
        const QShaderDescription::BlockVariable v = blk.members[i];
        switch (i) {
        case 0:
            QCOMPARE(v.offset, 0);
            QCOMPARE(v.size, 64);
            QCOMPARE(v.name, QByteArrayLiteral("mvp"));
            QCOMPARE(v.type, QShaderDescription::Mat4);
            QCOMPARE(v.matrixStride, 16);
            break;
        case 1:
            QCOMPARE(v.offset, 64);
            QCOMPARE(v.size, 4);
            QCOMPARE(v.name, QByteArrayLiteral("opacity"));
            QCOMPARE(v.type, QShaderDescription::Float);
            break;
        default:
            QFAIL(qPrintable(QStringLiteral("Too many blocks").arg(blk.members.count())));
            break;
        }
    }
}

void tst_QShaderBaker::simpleCompileFromDevice()
{
    QFile f(QLatin1String(":/data/color.vert"));
    QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));

    QShaderBaker baker;
    baker.setSourceDevice(&f, QShader::VertexStage);
    baker.setGeneratedShaderVariants({ QShader::StandardShader });
    QList<QShaderBaker::GeneratedShader> targets;
    targets.append({ QShader::SpirvShader, QShaderVersion(100) });
    baker.setGeneratedShaders(targets);
    QShader s = baker.bake();
    QVERIFY(s.isValid());
    QVERIFY(baker.errorMessage().isEmpty());
    QCOMPARE(s.availableShaders().count(), 1);
}

void tst_QShaderBaker::simpleCompileFromString()
{
    QFile f(QLatin1String(":/data/color.vert"));
    QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));
    const QByteArray contents = f.readAll();
    f.close();
    QVERIFY(!contents.isEmpty());

    QShaderBaker baker;
    baker.setSourceString(contents, QShader::VertexStage);
    baker.setGeneratedShaderVariants({ QShader::StandardShader });
    QList<QShaderBaker::GeneratedShader> targets;
    targets.append({ QShader::SpirvShader, QShaderVersion(100) });
    baker.setGeneratedShaders(targets);
    QShader s = baker.bake();
    QVERIFY(s.isValid());
    QVERIFY(baker.errorMessage().isEmpty());
    QCOMPARE(s.availableShaders().count(), 1);
}

void tst_QShaderBaker::multiCompile()
{
    QShaderBaker baker;
    baker.setSourceFileName(QLatin1String(":/data/color.vert"));
    baker.setGeneratedShaderVariants({ QShader::StandardShader });
    QList<QShaderBaker::GeneratedShader> targets;
    targets.append({ QShader::SpirvShader, QShaderVersion(100) });
    targets.append({ QShader::GlslShader, QShaderVersion(100, QShaderVersion::GlslEs) });
    targets.append({ QShader::GlslShader, QShaderVersion(120) });
    targets.append({ QShader::HlslShader, QShaderVersion(50) });
    targets.append({ QShader::MslShader, QShaderVersion(12) });
    baker.setGeneratedShaders(targets);
    QShader s = baker.bake();
    QVERIFY(s.isValid());
    QVERIFY(baker.errorMessage().isEmpty());
    QCOMPARE(s.availableShaders().count(), 5);

    for (const QShaderBaker::GeneratedShader &genShader : targets) {
        const QShaderKey key(genShader.first, genShader.second);
        const QShaderCode shader = s.shader(key);
        QVERIFY(!shader.shader().isEmpty());
        if (genShader.first != QShader::MslShader)
            QCOMPARE(shader.entryPoint(), QByteArrayLiteral("main"));
    }
}

void tst_QShaderBaker::reuse()
{
    QShaderBaker baker;
    baker.setSourceFileName(QLatin1String(":/data/color.vert"));
    baker.setGeneratedShaderVariants({ QShader::StandardShader });
    QList<QShaderBaker::GeneratedShader> targets;
    targets.append({ QShader::SpirvShader, QShaderVersion(100) });
    baker.setGeneratedShaders(targets);
    QShader s = baker.bake();
    QVERIFY(s.isValid());
    QVERIFY(baker.errorMessage().isEmpty());
    QCOMPARE(s.availableShaders().count(), 1);

    baker.setSourceFileName(QLatin1String(":/data/color.frag"));
    targets.clear();
    targets.append({ QShader::SpirvShader, QShaderVersion(100) });
    targets.append({ QShader::GlslShader, QShaderVersion(100, QShaderVersion::GlslEs) });
    targets.append({ QShader::GlslShader, QShaderVersion(120) });
    targets.append({ QShader::HlslShader, QShaderVersion(50) });
    targets.append({ QShader::MslShader, QShaderVersion(12) });
    baker.setGeneratedShaders(targets);
    s = baker.bake();
    QVERIFY(s.isValid());
    QVERIFY(baker.errorMessage().isEmpty());
    QCOMPARE(s.availableShaders().count(), 5);
}

void tst_QShaderBaker::compileError()
{
    QShaderBaker baker;
    baker.setSourceFileName(QLatin1String(":/data/error.vert"));
    baker.setGeneratedShaderVariants({ QShader::StandardShader });
    QList<QShaderBaker::GeneratedShader> targets;
    targets.append({ QShader::SpirvShader, QShaderVersion(100) });
    baker.setGeneratedShaders(targets);
    QShader s = baker.bake();
    QVERIFY(!s.isValid());
    QVERIFY(!baker.errorMessage().isEmpty());
    qDebug() << baker.errorMessage();
}

void tst_QShaderBaker::translateError()
{
    // assume the shader here fails in SPIRV-Cross with "cbuffer cannot be expressed with either HLSL packing layout or packoffset"
    QShaderBaker baker;
    baker.setSourceFileName(QLatin1String(":/data/hlsl_cbuf_error.frag"));
    baker.setGeneratedShaderVariants({ QShader::StandardShader });
    QList<QShaderBaker::GeneratedShader> targets;
    targets.append({ QShader::HlslShader, QShaderVersion(50) });
    baker.setGeneratedShaders(targets);
    QShader s = baker.bake();
    QVERIFY(!s.isValid());
    QVERIFY(!baker.errorMessage().isEmpty());
    qDebug() << baker.errorMessage();
}

void tst_QShaderBaker::genVariants()
{
    QShaderBaker baker;
    baker.setSourceFileName(QLatin1String(":/data/color.vert"));
    baker.setGeneratedShaderVariants({
                                         QShader::StandardShader,
                                         QShader::BatchableVertexShader
                                     });
    QList<QShaderBaker::GeneratedShader> targets;
    targets.append({ QShader::SpirvShader, QShaderVersion(100) });
    targets.append({ QShader::GlslShader, QShaderVersion(100, QShaderVersion::GlslEs) });
    targets.append({ QShader::GlslShader, QShaderVersion(330) });
    targets.append({ QShader::GlslShader, QShaderVersion(120) });
    targets.append({ QShader::HlslShader, QShaderVersion(50) });
    targets.append({ QShader::MslShader, QShaderVersion(12) });
    baker.setGeneratedShaders(targets);
    QShader s = baker.bake();
    QVERIFY(s.isValid());
    QVERIFY(baker.errorMessage().isEmpty());
    QCOMPARE(s.availableShaders().count(), 2 * 6);

    int batchableVariantCount = 0;
    int batchableGlslVariantCount = 0;
    for (const QShaderKey &key : s.availableShaders()) {
        if (key.sourceVariant() == QShader::BatchableVertexShader) {
            ++batchableVariantCount;
            if (key.source() == QShader::GlslShader) {
                ++batchableGlslVariantCount;
                const QByteArray src = s.shader(key).shader();
                QVERIFY(src.contains(QByteArrayLiteral("_qt_order * ")));
            }
        } else {
            if (key.source() == QShader::GlslShader) {
                // make sure it did not get rewritten
                const QByteArray src = s.shader(key).shader();
                QVERIFY(!src.contains(QByteArrayLiteral("_qt_order * ")));
            }
        }
    }
    QCOMPARE(batchableVariantCount, 6);
    QCOMPARE(batchableGlslVariantCount, 3);
}

void tst_QShaderBaker::defines()
{
    QShaderBaker baker;
    baker.setSourceFileName(QLatin1String(":/data/defines.frag"));
    baker.setGeneratedShaderVariants({ QShader::StandardShader });
    baker.setGeneratedShaders({ { QShader::SpirvShader, QShaderVersion(100) } });
    QShader s = baker.bake();
    QVERIFY(!s.isValid());
    QVERIFY(!baker.errorMessage().isEmpty());
    qDebug() << baker.errorMessage();

    QByteArray preamble;
    preamble = QByteArrayLiteral("#define DO_NOT_BREAK\n");
    baker.setPreamble(preamble);
    s = baker.bake();
    QVERIFY(s.isValid());
    QVERIFY(baker.errorMessage().isEmpty());

    QShaderDescription desc = s.description();
    QCOMPARE(desc.uniformBlocks().count(), 1);
    QShaderDescription::UniformBlock blk = desc.uniformBlocks().first();
    QCOMPARE(blk.members.count(), 2);
    bool opacity_ok = false;
    for (int i = 0; i < blk.members.count(); ++i) {
        const QShaderDescription::BlockVariable v = blk.members[i];
        if (v.name == QByteArrayLiteral("opacity")) {
            opacity_ok = v.type == QShaderDescription::Vec4;
            break;
        }
    }
    QVERIFY(opacity_ok);

    preamble += QByteArrayLiteral("#define OPACITY_SIZE 1\n");
    baker.setPreamble(preamble);
    s = baker.bake();
    QVERIFY(s.isValid());
    QVERIFY(baker.errorMessage().isEmpty());

    desc = s.description();
    blk = desc.uniformBlocks().first();
    opacity_ok = false;
    for (int i = 0; i < blk.members.count(); ++i) {
        const QShaderDescription::BlockVariable v = blk.members[i];
        if (v.name == QByteArrayLiteral("opacity")) {
            opacity_ok = v.type == QShaderDescription::Float;
            break;
        }
    }
    QVERIFY(opacity_ok);
}

void tst_QShaderBaker::reflectArrayOfStructInBlock()
{
    QShaderBaker baker;
    baker.setSourceFileName(QLatin1String(":/data/array_of_struct_in_ubuf.frag"));
    baker.setGeneratedShaderVariants({ QShader::StandardShader });
    QList<QShaderBaker::GeneratedShader> targets;
    targets.append({ QShader::SpirvShader, QShaderVersion(100) });
    targets.append({ QShader::GlslShader, QShaderVersion(100, QShaderVersion::GlslEs) });
    targets.append({ QShader::GlslShader, QShaderVersion(120) });
    targets.append({ QShader::GlslShader, QShaderVersion(150) });
    targets.append({ QShader::HlslShader, QShaderVersion(50) });
    targets.append({ QShader::MslShader, QShaderVersion(12) });
    baker.setGeneratedShaders(targets);
    QShader s = baker.bake();
    QVERIFY(s.isValid());
    QVERIFY(baker.errorMessage().isEmpty());

    QShaderDescription desc = s.description();

    QCOMPARE(desc.inputVariables().count(), 3);
    QCOMPARE(desc.outputVariables().count(), 1);
    QCOMPARE(desc.uniformBlocks().count(), 1);

    const QList<QShaderDescription::InOutVariable> inputs = desc.inputVariables();
    for (const auto &var : inputs) {
        switch (var.location) {
        case 0:
            QCOMPARE(var.name, QByteArrayLiteral("vECVertNormal"));
            QCOMPARE(var.type, QShaderDescription::Vec3);
            break;
        case 1:
            QCOMPARE(var.name, QByteArrayLiteral("vECVertPos"));
            QCOMPARE(var.type, QShaderDescription::Vec3);
            break;
        case 2:
            QCOMPARE(var.name, QByteArrayLiteral("vDiffuseAdjust"));
            QCOMPARE(var.type, QShaderDescription::Vec3);
            break;
        default:
            QFAIL(qPrintable(QStringLiteral("Unexpected input variable: %1").arg(var.location)));
            return;
        }
    }

    const QList<QShaderDescription::InOutVariable> outputs = desc.outputVariables();
    QCOMPARE(outputs.first().location, 0);
    QCOMPARE(outputs.first().name, QByteArrayLiteral("fragColor"));
    QCOMPARE(outputs.first().type, QShaderDescription::Vec4);

    const QList<QShaderDescription::UniformBlock> ublocks = desc.uniformBlocks();
    const QShaderDescription::UniformBlock ub = ublocks.first();
    QCOMPARE(ub.binding, 1);
    QCOMPARE(ub.blockName, QByteArrayLiteral("buf"));
    QCOMPARE(ub.structName, QByteArrayLiteral("ubuf"));
    QCOMPARE(ub.size, 768);
    QCOMPARE(ub.members.count(), 7);

    for (const QShaderDescription::BlockVariable &var : ub.members) {
        if (var.name == QByteArrayLiteral("ECCameraPosition")) {
            QCOMPARE(var.offset, 0);
            QCOMPARE(var.size, 12);
            QCOMPARE(var.type, QShaderDescription::Vec3);
        } else if (var.name == QByteArrayLiteral("ka")) {
            QCOMPARE(var.offset, 16);
            QCOMPARE(var.size, 12);
            QCOMPARE(var.type, QShaderDescription::Vec3);
        } else if (var.name == QByteArrayLiteral("kd")) {
            QCOMPARE(var.offset, 32);
            QCOMPARE(var.size, 12);
            QCOMPARE(var.type, QShaderDescription::Vec3);
        } else if (var.name == QByteArrayLiteral("ks")) {
            QCOMPARE(var.offset, 48);
            QCOMPARE(var.size, 12);
            QCOMPARE(var.type, QShaderDescription::Vec3);
        } else if (var.name == QByteArrayLiteral("numLights")) {
            QCOMPARE(var.offset, 704);
            QCOMPARE(var.size, 4);
            QCOMPARE(var.type, QShaderDescription::Int);
        } else if (var.name == QByteArrayLiteral("mm")) {
            QCOMPARE(var.offset, 720);
            QCOMPARE(var.size, 48);
            QCOMPARE(var.type, QShaderDescription::Mat3);
            QCOMPARE(var.matrixStride, 16);
            QCOMPARE(var.matrixIsRowMajor, true);
        } else if (var.name == QByteArrayLiteral("lights")) {
            QCOMPARE(var.offset, 64);
            QCOMPARE(var.size, 640);
            QCOMPARE(var.type, QShaderDescription::Struct);
            QCOMPARE(var.arrayDims, QList<int>() << 10);
            QCOMPARE(var.structMembers.count(), 7);
            for (const QShaderDescription::BlockVariable &structVar : var.structMembers) {
                if (structVar.name == QByteArrayLiteral("ECLightPosition")) {
                    QCOMPARE(structVar.offset, 0);
                    QCOMPARE(structVar.size, 12);
                    QCOMPARE(structVar.type, QShaderDescription::Vec3);
                } else if (structVar.name == QByteArrayLiteral("attenuation")) {
                    QCOMPARE(structVar.offset, 16);
                    QCOMPARE(structVar.size, 12);
                    QCOMPARE(structVar.type, QShaderDescription::Vec3);
                } else if (structVar.name == QByteArrayLiteral("color")) {
                    QCOMPARE(structVar.offset, 32);
                    QCOMPARE(structVar.size, 12);
                    QCOMPARE(structVar.type, QShaderDescription::Vec3);
                } else if (structVar.name == QByteArrayLiteral("intensity")) {
                    QCOMPARE(structVar.offset, 44);
                    QCOMPARE(structVar.size, 4);
                    QCOMPARE(structVar.type, QShaderDescription::Float);
                } else if (structVar.name == QByteArrayLiteral("specularExp")) {
                    QCOMPARE(structVar.offset, 48);
                    QCOMPARE(structVar.size, 4);
                    QCOMPARE(structVar.type, QShaderDescription::Float);
                } else if (structVar.name == QByteArrayLiteral("__dummy0")) {
                    QCOMPARE(structVar.offset, 52);
                    QCOMPARE(structVar.size, 4);
                    QCOMPARE(structVar.type, QShaderDescription::Float);
                } else if (structVar.name == QByteArrayLiteral("__dummy1")) {
                    QCOMPARE(structVar.offset, 56);
                    QCOMPARE(structVar.size, 4);
                    QCOMPARE(structVar.type, QShaderDescription::Float);
                } else {
                    QFAIL(qPrintable(
                            QStringLiteral(
                                    "Unexpected member in 'lights' struct in uniform block: %1")
                                    .arg(structVar.name)));
                }
            }
        } else {
            QFAIL(qPrintable(QStringLiteral("Unexpected uniform block member: %1").arg(var.name)));
        }
    }
}

void tst_QShaderBaker::reflectCombinedImageSampler()
{
    QShaderBaker baker;
    baker.setSourceFileName(QLatin1String(":/data/sgtexture.frag"));
    baker.setGeneratedShaderVariants({ QShader::StandardShader });
    QList<QShaderBaker::GeneratedShader> targets;
    targets.append({ QShader::SpirvShader, QShaderVersion(100) });
    targets.append({ QShader::GlslShader, QShaderVersion(100, QShaderVersion::GlslEs) });
    targets.append({ QShader::GlslShader, QShaderVersion(120) });
    targets.append({ QShader::GlslShader, QShaderVersion(150) });
    targets.append({ QShader::HlslShader, QShaderVersion(50) });
    targets.append({ QShader::MslShader, QShaderVersion(12) });
    baker.setGeneratedShaders(targets);
    QShader s = baker.bake();
    QVERIFY(s.isValid());
    QVERIFY(baker.errorMessage().isEmpty());

    QShaderDescription desc = s.description();
    QCOMPARE(desc.inputVariables().count(), 1);
    QCOMPARE(desc.outputVariables().count(), 1);
    QCOMPARE(desc.uniformBlocks().count(), 1);
    QCOMPARE(desc.combinedImageSamplers().count(), 2);

    auto inputVar = desc.inputVariables().first();
    QCOMPARE(inputVar.location, 0);
    QCOMPARE(inputVar.name, QByteArrayLiteral("qt_TexCoord"));
    QCOMPARE(inputVar.type, QShaderDescription::Vec2);

    auto outputVar = desc.outputVariables().first();
    QCOMPARE(outputVar.location, 0);
    QCOMPARE(outputVar.name, QByteArrayLiteral("fragColor"));
    QCOMPARE(outputVar.type, QShaderDescription::Vec4);

    auto block = desc.uniformBlocks().first();
    QCOMPARE(block.binding, 0);
    QCOMPARE(block.size, 68);
    QCOMPARE(block.blockName, QByteArrayLiteral("buf"));
    QCOMPARE(block.structName, QByteArrayLiteral("ubuf"));
    QCOMPARE(block.members.count(), 2);
    for (int i = 0; i < block.members.count(); ++i) {
        const QShaderDescription::BlockVariable &blockVar(block.members[i]);
        switch (i) {
        case 0:
            QCOMPARE(blockVar.name, QByteArrayLiteral("qt_Matrix"));
            QCOMPARE(blockVar.offset, 0);
            QCOMPARE(blockVar.size, 64);
            QCOMPARE(blockVar.type, QShaderDescription::Mat4);
            QCOMPARE(blockVar.matrixStride, 16);
            QCOMPARE(blockVar.matrixIsRowMajor, false);
            break;
        case 1:
            QCOMPARE(blockVar.name, QByteArrayLiteral("opacity"));
            QCOMPARE(blockVar.offset, 64);
            QCOMPARE(blockVar.size, 4);
            QCOMPARE(blockVar.type, QShaderDescription::Float);
            break;
        default:
            break;
        }
    }

    for (const QShaderDescription::InOutVariable &imSampVar : desc.combinedImageSamplers()) {
        switch (imSampVar.binding) {
        case 1:
            QCOMPARE(imSampVar.name, QByteArrayLiteral("qt_Texture"));
            QCOMPARE(imSampVar.type, QShaderDescription::Sampler2D);
            break;
        case 2:
            QCOMPARE(imSampVar.name, QByteArrayLiteral("t1"));
            QCOMPARE(imSampVar.type, QShaderDescription::Sampler2D);
            break;
        default:
            QFAIL(qPrintable(QStringLiteral("Unexpected combined image sampler: %1")
                                     .arg(imSampVar.binding)));
            return;
        }
    }
}

void tst_QShaderBaker::mslNativeBindingMap()
{
    QShaderBaker baker;
    baker.setSourceFileName(QLatin1String(":/data/sgtexture.frag"));
    baker.setGeneratedShaderVariants({ QShader::StandardShader });
    QList<QShaderBaker::GeneratedShader> targets;
    targets.append({ QShader::SpirvShader, QShaderVersion(100) });
    targets.append({ QShader::GlslShader, QShaderVersion(100, QShaderVersion::GlslEs) });
    targets.append({ QShader::GlslShader, QShaderVersion(120) });
    targets.append({ QShader::GlslShader, QShaderVersion(150) });
    targets.append({ QShader::HlslShader, QShaderVersion(50) });
    targets.append({ QShader::MslShader, QShaderVersion(12) });
    baker.setGeneratedShaders(targets);
    QShader s = baker.bake();
    QVERIFY(s.isValid());
    QVERIFY(baker.errorMessage().isEmpty());

    const QShaderKey mslShaderKey(QShader::MslShader, QShaderVersion(12));
    QVERIFY(s.nativeResourceBindingMap(mslShaderKey));

    const QShader::NativeResourceBindingMap *nativeBindingMap = s.nativeResourceBindingMap(mslShaderKey);
    QCOMPARE(nativeBindingMap->count(), 3);
    QVERIFY(nativeBindingMap->contains(0)); // uniform block
    QVERIFY(nativeBindingMap->contains(1)); // combined image sampler, maps to a texture and sampler in MSL
    QVERIFY(nativeBindingMap->contains(2)); // same

    // MSL has per-resource namespaces
    QPair<int, int> nativeBindingPair = nativeBindingMap->value(0);
    QCOMPARE(nativeBindingPair.first, 0); // uniform buffer

    nativeBindingPair = nativeBindingMap->value(1);
    QCOMPARE(nativeBindingPair.first, 0); // texture
    QCOMPARE(nativeBindingPair.second, 0); // sampler

    nativeBindingPair = nativeBindingMap->value(2);
    QCOMPARE(nativeBindingPair.first, 1); // texture
    QCOMPARE(nativeBindingPair.second, 1); // sampler

    baker.setSourceFileName(QLatin1String(":/data/manyresources.frag"));
    targets = { { QShader::SpirvShader, QShaderVersion(100) },
                { QShader::MslShader, QShaderVersion(12) } };
    baker.setGeneratedShaders(targets);
    s = baker.bake();
    QVERIFY(s.isValid());
    QVERIFY(baker.errorMessage().isEmpty());

    QVERIFY(s.nativeResourceBindingMap(mslShaderKey));
    nativeBindingMap = s.nativeResourceBindingMap(mslShaderKey);
    QCOMPARE(nativeBindingMap->count(), 8);

    // rather won't compare with exact values, do not make assumptions, just
    // make sure there is a valid value for each since all resources are used
    // in the shader code
    QVERIFY(nativeBindingMap->value(1).first != -1);
    QVERIFY(nativeBindingMap->value(2).first != -1);
    QVERIFY(nativeBindingMap->value(3).first != -1);
    QVERIFY(nativeBindingMap->value(4).first != -1);
    QVERIFY(nativeBindingMap->value(5).first != -1);
    QVERIFY(nativeBindingMap->value(10).first != -1);
    QVERIFY(nativeBindingMap->value(11).first != -1);
    QVERIFY(nativeBindingMap->value(12).first != -1);
}

void tst_QShaderBaker::hlslNativeBindingMap()
{
    QShaderBaker baker;
    baker.setSourceFileName(QLatin1String(":/data/manyresources.frag"));
    baker.setGeneratedShaderVariants({ QShader::StandardShader });
    QList<QShaderBaker::GeneratedShader> targets;
    targets.append({ QShader::SpirvShader, QShaderVersion(100) });
    targets.append({ QShader::HlslShader, QShaderVersion(50) });
    baker.setGeneratedShaders(targets);
    QShader s = baker.bake();
    QVERIFY(s.isValid());
    QVERIFY(baker.errorMessage().isEmpty());

    const QShaderKey shaderKey(QShader::HlslShader, QShaderVersion(50));
    QVERIFY(s.nativeResourceBindingMap(shaderKey));
    const QShader::NativeResourceBindingMap *nativeBindingMap = s.nativeResourceBindingMap(shaderKey);
    QCOMPARE(nativeBindingMap->count(), 8);

    // do not assume anything about the ordering, so verify that we have valid
    // values, not the register binding values themselves

    // combined image samplers
    QVERIFY(nativeBindingMap->value(1).first != -1);
    QVERIFY(nativeBindingMap->value(1).second == nativeBindingMap->value(1).first);
    QVERIFY(nativeBindingMap->value(2).first != -1);
    QVERIFY(nativeBindingMap->value(2).second == nativeBindingMap->value(2).first);
    QVERIFY(nativeBindingMap->value(3).first != -1);
    QVERIFY(nativeBindingMap->value(3).second == nativeBindingMap->value(3).first);

    QVERIFY(nativeBindingMap->value(1).first != nativeBindingMap->value(2).first);
    QVERIFY(nativeBindingMap->value(1).first != nativeBindingMap->value(3).first);

    // storage images
    QVERIFY(nativeBindingMap->value(4).first != -1);
    QVERIFY(nativeBindingMap->value(5).first != -1);
    QVERIFY(nativeBindingMap->value(4).first != nativeBindingMap->value(5).first);

    // storage buffer
    QVERIFY(nativeBindingMap->value(10).first != -1);

    // uniform buffers
    QVERIFY(nativeBindingMap->value(11).first != -1);
    QVERIFY(nativeBindingMap->value(12).first != -1);
    QVERIFY(nativeBindingMap->value(11).first != nativeBindingMap->value(12).first);
}

void tst_QShaderBaker::reflectArraysOfSamplers()
{
    QShaderBaker baker;
    baker.setSourceFileName(QLatin1String(":/data/arrays_of_samplers.frag"));
    baker.setGeneratedShaderVariants({ QShader::StandardShader });
    QList<QShaderBaker::GeneratedShader> targets;
    targets.append({ QShader::SpirvShader, QShaderVersion(100) });
    targets.append({ QShader::GlslShader, QShaderVersion(100, QShaderVersion::GlslEs) });
    targets.append({ QShader::GlslShader, QShaderVersion(120) });
    targets.append({ QShader::GlslShader, QShaderVersion(150) });
    targets.append({ QShader::HlslShader, QShaderVersion(50) });
    targets.append({ QShader::MslShader, QShaderVersion(20) }); // arrays of textures needs MSL 2.0
    baker.setGeneratedShaders(targets);
    QShader s = baker.bake();
    QVERIFY(baker.errorMessage().isEmpty());
    QVERIFY(s.isValid());

    QShaderDescription desc = s.description();

    QCOMPARE(desc.inputVariables().count(), 1);
    QCOMPARE(desc.outputVariables().count(), 1);
    QCOMPARE(desc.combinedImageSamplers().count(), 4);

    for (const QShaderDescription::InOutVariable &var : desc.combinedImageSamplers()) {
        switch (var.binding) {
        case 4:
            QCOMPARE(var.type, QShaderDescription::Sampler2D);
            QVERIFY(var.arrayDims.isEmpty());
            break;
        case 8:
            QCOMPARE(var.type, QShaderDescription::Sampler2D);
            QCOMPARE(var.arrayDims.count(), 1);
            QCOMPARE(var.arrayDims.first(), 4);
            break;
        case 9:
            QCOMPARE(var.type, QShaderDescription::SamplerCube);
            QCOMPARE(var.arrayDims.count(), 1);
            QCOMPARE(var.arrayDims.first(), 4);
            break;
        case 10:
            QCOMPARE(var.type, QShaderDescription::Sampler2DArray);
            QVERIFY(var.arrayDims.isEmpty());
            break;
        default:
            QFAIL(qPrintable(QStringLiteral("Wrong binding value: %1").arg(var.binding)));
        }
    }
}

void tst_QShaderBaker::perTargetCompileMode()
{
    QShaderBaker baker;
    baker.setSourceFileName(QLatin1String(":/data/color_pertarget.vert"));
    baker.setGeneratedShaderVariants({
                                         QShader::StandardShader,
                                         QShader::BatchableVertexShader
                                     });
    QList<QShaderBaker::GeneratedShader> targets;
    targets.append({ QShader::SpirvShader, QShaderVersion(100) });
    targets.append({ QShader::GlslShader, QShaderVersion(100, QShaderVersion::GlslEs) });
    targets.append({ QShader::GlslShader, QShaderVersion(330) });
    targets.append({ QShader::GlslShader, QShaderVersion(120) });
    targets.append({ QShader::HlslShader, QShaderVersion(50) });
    targets.append({ QShader::MslShader, QShaderVersion(12) });
    baker.setGeneratedShaders(targets);

    baker.setPreamble(QByteArrayLiteral("#define MY_VALUE 321\n#define MY_MACRO\n"));

    baker.setPerTargetCompilation(true);

    QShader s = baker.bake();
    if (!s.isValid())
        qDebug() << baker.errorMessage();
    QVERIFY(s.isValid());
    QVERIFY(baker.errorMessage().isEmpty());
    QCOMPARE(s.availableShaders().count(), 2 * 6);

    int batchableVariantCount = 0;
    int batchableGlslVariantCount = 0;
    for (const QShaderKey &key : s.availableShaders()) {
        if (key.sourceVariant() == QShader::BatchableVertexShader) {
            ++batchableVariantCount;
            if (key.source() == QShader::GlslShader) {
                ++batchableGlslVariantCount;
                const QByteArray src = s.shader(key).shader();
                QVERIFY(src.contains(QByteArrayLiteral("_qt_order * ")));
            }
        } else {
            if (key.source() == QShader::GlslShader) {
                // make sure it did not get rewritten
                const QByteArray src = s.shader(key).shader();
                QVERIFY(!src.contains(QByteArrayLiteral("_qt_order * ")));
            }
        }
    }
    QCOMPARE(batchableVariantCount, 6);
    QCOMPARE(batchableGlslVariantCount, 3);

    // now compile something that succeeds only if the QSHADER_SPIRV macros are
    // present as expected
    baker.setSourceFileName(QLatin1String(":/data/color_spirv_only.vert"));
    baker.setGeneratedShaderVariants({ QShader::StandardShader });
    targets.clear();
    targets.append({ QShader::SpirvShader, QShaderVersion(100) });
    baker.setGeneratedShaders(targets);
    baker.setPreamble(QByteArray());
    s = baker.bake();
    if (!s.isValid())
        qDebug() << baker.errorMessage();
    QVERIFY(s.isValid());
    QVERIFY(baker.errorMessage().isEmpty());
    QCOMPARE(s.availableShaders().count(), 1);

    // now something similar, with GLSL. This will mean no QSHADER_SPIRV,
    // QSHADER_HLSL, QSHADER_MSL are defined (and is an error if they are)
    baker.setSourceFileName(QLatin1String(":/data/color_glsl_only.vert"));
    baker.setGeneratedShaderVariants({ QShader::StandardShader });
    targets.clear();
    targets.append({ QShader::GlslShader, QShaderVersion(330) });
    targets.append({ QShader::GlslShader, QShaderVersion(440) });
    baker.setGeneratedShaders(targets);
    s = baker.bake();
    if (!s.isValid())
        qDebug() << baker.errorMessage();
    QVERIFY(s.isValid());
    QVERIFY(baker.errorMessage().isEmpty());
    QCOMPARE(s.availableShaders().count(), 2);

    // HLSL 5.0 + MSL 1.2 (fails with others)
    baker.setSourceFileName(QLatin1String(":/data/color_hlsl_msl_only.vert"));
    baker.setGeneratedShaderVariants({ QShader::StandardShader });
    targets.clear();
    targets.append({ QShader::HlslShader, QShaderVersion(50) });
    targets.append({ QShader::MslShader, QShaderVersion(12) });
    baker.setGeneratedShaders(targets);
    s = baker.bake();
    if (!s.isValid())
        qDebug() << baker.errorMessage();
    QVERIFY(s.isValid());
    QVERIFY(baker.errorMessage().isEmpty());
    QCOMPARE(s.availableShaders().count(), 2);
}

void tst_QShaderBaker::serializeDeserialize()
{
    QShaderBaker baker;
    baker.setSourceFileName(QLatin1String(":/data/color.vert"));
    baker.setGeneratedShaderVariants({ QShader::StandardShader });
    QList<QShaderBaker::GeneratedShader> targets;
    targets.append({ QShader::SpirvShader, QShaderVersion(100) });
    targets.append({ QShader::GlslShader, QShaderVersion(100, QShaderVersion::GlslEs) });
    targets.append({ QShader::GlslShader, QShaderVersion(120) });
    targets.append({ QShader::HlslShader, QShaderVersion(50) });
    targets.append({ QShader::MslShader, QShaderVersion(12) });
    baker.setGeneratedShaders(targets);
    QShader s = baker.bake();
    QVERIFY(s.isValid());
    QVERIFY(baker.errorMessage().isEmpty());
    QCOMPARE(s.availableShaders().count(), 5);

    QByteArray data = s.serialized();
    QVERIFY(!data.isEmpty());

    QShader s2;
    QVERIFY(!s2.isValid());
    s2 = QShader::fromSerialized(data);
    QVERIFY(s2.isValid());
    QCOMPARE(s, s2);
}

void tst_QShaderBaker::spirvOptions()
{
    QShaderBaker baker;
    baker.setSourceFileName(QLatin1String(":/data/color.vert"));
    baker.setGeneratedShaderVariants({ QShader::StandardShader });
    QList<QShaderBaker::GeneratedShader> targets;
    targets.append({ QShader::SpirvShader, QShaderVersion(100) });
    baker.setGeneratedShaders(targets);

    // no options specified (no full debug info, no strip)
    QShader s = baker.bake();
    QVERIFY(s.isValid());
    QVERIFY(baker.errorMessage().isEmpty());
    QCOMPARE(s.availableShaders().count(), 1);
    const QShaderKey key(QShader::SpirvShader, QShaderVersion(100));
    QVERIFY(s.availableShaders().contains(key));
    QCOMPARE(s.shader(key).entryPoint(), "main");
    QByteArray bin = s.shader(key).shader();

    // full debug info
    baker.setSpirvOptions(QShaderBaker::SpirvOption::GenerateFullDebugInfo);
    s = baker.bake();
    QVERIFY(s.isValid());
    QVERIFY(baker.errorMessage().isEmpty());
    QCOMPARE(s.availableShaders().count(), 1);
    QCOMPARE(s.shader(key).entryPoint(), "main");
    QByteArray debugBin = s.shader(key).shader();

    QVERIFY(bin != debugBin);
    QVERIFY(debugBin.size() > bin.size());

    // strip all (variable names, etc.)
    baker.setSpirvOptions(QShaderBaker::SpirvOption::StripDebugAndVarInfo);
    s = baker.bake();
    QVERIFY(s.isValid());
    QVERIFY(baker.errorMessage().isEmpty());
    QCOMPARE(s.availableShaders().count(), 1);
    QCOMPARE(s.shader(key).entryPoint(), "main");
    QByteArray strippedBin = s.shader(key).shader();

    QVERIFY(strippedBin != debugBin);
    QVERIFY(debugBin.size() > strippedBin.size());
    QVERIFY(strippedBin != bin);
    QVERIFY(bin.size() > strippedBin.size());
}

#include <tst_qshaderbaker.moc>
QTEST_MAIN(tst_QShaderBaker)
