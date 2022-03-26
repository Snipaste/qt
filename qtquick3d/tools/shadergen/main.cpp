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

#include <QCoreApplication>

#include <QXmlStreamWriter>
#include <qlist.h>

#include <QCommandLineOption>
#include <QCommandLineParser>

#include <QtCore/qfile.h>
#include <QtCore/qdir.h>

#include <QtQuick3DUtils/private/qqsbcollection_p.h>

#include "genshaders.h"

#include "parser.h"

constexpr int DEAFULT_SEARCH_DEPTH = 0x10;

static int generateShaders(QVector<QString> &qsbcFiles,
                           const QVector<QString> &filePaths,
                           const QDir &sourceDir,
                           const QDir &outDir,
                           bool multilight,
                           bool verboseOutput,
                           bool dryRun)
{
    MaterialParser::SceneData sceneData;
    if (MaterialParser::parseQmlFiles(filePaths, sourceDir, sceneData, verboseOutput) == 0) {
        if (sceneData.hasData()) {
            GenShaders genShaders;
            if (!genShaders.process(sceneData, qsbcFiles, outDir, multilight, dryRun))
                return -1;
        } else if (verboseOutput) {
            if (!sceneData.viewport)
                qWarning() << "No View3D item found";
        }
    }
    return 0;
}

static int writeResourceFile(const QString &resourceFile,
                             const QVector<QString> &qsbcFiles,
                             const QDir &outDir)
{
    if (qsbcFiles.isEmpty())
        return -1;

    const QString outFilename = outDir.canonicalPath() + QDir::separator() + resourceFile;
    QFile outFile(outFilename);
    if (!outFile.open(QFile::WriteOnly | QFile::Text | QFile::Truncate)) {
        qWarning() << "Unable to create output file " << outFilename;
        return -1;
    }

    QXmlStreamWriter writer(&outFile);
    writer.setAutoFormatting(true);
    writer.writeStartElement("RCC");
        writer.writeStartElement("qresource");
        writer.writeAttribute("prefix", "/");
        for (const auto &f : qsbcFiles)
            writer.writeTextElement("file", f);
        writer.writeEndElement();
    writer.writeEndElement();
    outFile.close();

    return 0;
}

struct SearchDepthGuard
{
    explicit SearchDepthGuard(int m) : max(m) {}
    int value = 0;
    const int max = DEAFULT_SEARCH_DEPTH;
};

static void collectQmlFiles(const QList<QString> &pathArgs, QSet<QString> &filePaths, SearchDepthGuard &depth)
{
    QFileInfo fi;
    QDir dir;
    for (const auto &arg : pathArgs) {
        fi.setFile(arg);
        if (fi.isFile()) {
             if (fi.suffix() == QLatin1String("qml"))
                 filePaths.insert(fi.canonicalFilePath());
        } else if (fi.isDir() && depth.value <= depth.max) {
            dir.setPath(fi.filePath());
            const auto entries = dir.entryList(QDir::Filter::Dirs | QDir::Filter::Files | QDir::Filter::NoDotAndDotDot);
            const QString currentPath = QDir::currentPath();
            QDir::setCurrent(dir.path());
            ++depth.value;
            collectQmlFiles(entries, filePaths, depth);
            --depth.value;
            QDir::setCurrent(currentPath);
        }
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QCommandLineParser cmdLineparser;
    cmdLineparser.addHelpOption();
    // File options
    QCommandLineOption changeDirOption({QChar(u'C'), QLatin1String("directory")},
                                       QLatin1String("Change the working directory"),
                                       QLatin1String("dir"));
    cmdLineparser.addOption(changeDirOption);

    // Debug options
    QCommandLineOption verboseOutputOption({QChar(u'v'), QLatin1String("verbose")}, QLatin1String("Turn on verbose output."));
    cmdLineparser.addOption(verboseOutputOption);

    // Generator options
    QCommandLineOption dryRunOption({QChar(u'n'), QLatin1String("dry-run")}, QLatin1String("Runs as normal, but no files are created."));
    cmdLineparser.addOption(dryRunOption);

    QCommandLineOption outputDirOption({QChar(u'o'), QLatin1String("output-dir")}, QLatin1String("Output directory for generated files."), QLatin1String("file"));
    cmdLineparser.addOption(outputDirOption);

    QCommandLineOption resourceFileOption({QChar(u'r'), QLatin1String("resource-file")}, QLatin1String("Name of generated resource file."), QLatin1String("file"));
    cmdLineparser.addOption(resourceFileOption);

    QCommandLineOption dumpQsbcFileOption({QChar(u'l'), QLatin1String("list-qsbc")}, QLatin1String("Lists qsbc file content."));
    cmdLineparser.addOption(dumpQsbcFileOption);

    QCommandLineOption extractQsbFileOption({QChar(u'e'), QLatin1String("extract-qsb")}, QLatin1String("Extract qsb from collection."), QLatin1String("key:[desc|vert|frag]"));
    cmdLineparser.addOption(extractQsbFileOption);

    QCommandLineOption dirDepthOption(QLatin1String("depth"), QLatin1String("Override default max depth (16) value when traversing the filesystem."), QLatin1String("number"));
    cmdLineparser.addOption(dirDepthOption);

    cmdLineparser.process(a);

    if (cmdLineparser.isSet(changeDirOption)) {
        const auto value = cmdLineparser.value(changeDirOption);
        QFileInfo fi(value);
        if (!fi.isDir()) {
            qWarning("%s : %s - Not a directory", qPrintable(a.applicationName()), qPrintable(value));
            return -1;
        }
        QDir::setCurrent(value);
    }

    QSet<QString> filePaths;
    auto args = cmdLineparser.positionalArguments();

    const bool collectQmlFilesMode = !(cmdLineparser.isSet(dumpQsbcFileOption) || cmdLineparser.isSet(extractQsbFileOption));
    if (collectQmlFilesMode) {
        if (args.isEmpty())
            args.push_back(QDir::currentPath());

        int searchDepth = DEAFULT_SEARCH_DEPTH;
        if (cmdLineparser.isSet(dirDepthOption)) {
            bool ok = false;
            const int v = cmdLineparser.value(dirDepthOption).toInt(&ok);
            if (ok)
                searchDepth = v;
        }

        SearchDepthGuard depth(searchDepth);
        collectQmlFiles(args, filePaths, depth);
    } else if (!args.isEmpty()) {
        filePaths.insert(args.first());
    }

    if (filePaths.isEmpty()) {
        qWarning("No input file(s) found!");
        a.exit(-1);
        return -1;
    }

    if (cmdLineparser.isSet(dumpQsbcFileOption)) {
        const auto &f = *filePaths.cbegin();
        if (!f.isEmpty()) {
            QQsbCollection::dumpQsbcInfo(f);
            a.exit(0);
            return 0;
        }
    }

    static const auto printBytes = [](const QByteArray &ba) {
        for (const auto &b : ba)
            printf("%c", b);
    };

    if (cmdLineparser.isSet(extractQsbFileOption)) {
        const auto &f = *filePaths.cbegin();
        const auto k = cmdLineparser.value(extractQsbFileOption);
        const auto kl = QStringView(k).split(u':');

        bool ok = false;
        const auto &keyView = kl.at(0);
        const size_t key = keyView.toULong(&ok);
        if (ok) {
            enum ExtractWhat : quint8 { Desc = 0x1, Vert = 0x2, Frag = 0x4 };
            quint8 what = 0;
            if (kl.size() > 1) {
                const auto &rest = kl.at(1);
                const auto &options = rest.split(u'|');
                for (const auto &o : options) {
                    if (o == QLatin1String("desc"))
                        what |= ExtractWhat::Desc;
                    if (o == QLatin1String("vert"))
                        what |= ExtractWhat::Vert;
                    if (o == QLatin1String("frag"))
                        what |= ExtractWhat::Frag;
                }
            }
            QQsbCollection qsbc(f);
            if (qsbc.map(QQsbCollection::Read)) {
                const auto entries = qsbc.getEntries();
                const auto foundIt = entries.constFind(QQsbCollection::Entry{key});
                if (foundIt != entries.cend()) {
                    QByteArray desc;
                    QShader vertShader;
                    QShader fragShader;
                    QQsbShaderFeatureSet featureSet;
                    qsbc.extractQsbEntry(*foundIt, &desc, &featureSet, &vertShader, &fragShader);
                    if (what == 0)
                        qDebug("Entry with key %zu found.", key);
                    if (what & ExtractWhat::Desc)
                        printBytes(desc);
                    if (what & ExtractWhat::Vert)
                        printBytes(qUncompress(vertShader.serialized()));
                    if (what & ExtractWhat::Frag)
                        printBytes(qUncompress(fragShader.serialized()));
                } else {
                    qWarning("Entry with key %zu could not be found.", key);
                }
                qsbc.unmap();
            }
            a.exit(0);
            return 0;
        }

        qWarning("Command %s failed with input: %s and %s.", qPrintable(extractQsbFileOption.valueName()), qPrintable(f), qPrintable(k));
        a.exit(-1);
        return -1;
    }

    QString resourceFile = cmdLineparser.value(resourceFileOption);
    if (resourceFile.isEmpty())
        resourceFile = QStringLiteral("genshaders.qrc");

    const bool dryRun = cmdLineparser.isSet(dryRunOption);
    const QString &outputPath = cmdLineparser.isSet(outputDirOption) ? cmdLineparser.value(outputDirOption) : QDir::currentPath();
    QDir outDir;
    if (!outputPath.isEmpty() && !dryRun) {
        outDir.setPath(outputPath);
        if (outDir.exists(outputPath) || (!outDir.exists(outputPath) && outDir.mkpath(outputPath))) {
            outDir.setPath(outputPath);
            qDebug("Writing files to %s", qPrintable(outDir.canonicalPath()));
        } else {
            qDebug("Unable to change or create output folder %s", qPrintable(outputPath));
            return -1;
        }
    }

    const bool verboseOutput = cmdLineparser.isSet(verboseOutputOption);
    const bool multilight = false;

    QVector<QString> qsbcFiles;

    int ret = 0;
    if (filePaths.size())
        ret = generateShaders(qsbcFiles, filePaths.values(), QDir::currentPath(), outDir, multilight, verboseOutput, dryRun);

    if (ret == 0 && !dryRun)
        writeResourceFile(resourceFile, qsbcFiles, outDir);

    a.exit(ret);
    return ret;
}
