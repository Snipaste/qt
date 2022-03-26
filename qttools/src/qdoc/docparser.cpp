/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
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
#include "docparser.h"

#include "codemarker.h"
#include "doc.h"
#include "docprivate.h"
#include "editdistance.h"
#include "macro.h"
#include "openedlist.h"
#include "tokenizer.h"

#include <QtCore/qfile.h>
#include <QtCore/qregularexpression.h>
#include <QtCore/qtextstream.h>

#include <cctype>
#include <climits>

QT_BEGIN_NAMESPACE

DocUtilities &DocParser::s_utilities = DocUtilities::instance();

enum {
    CMD_A,
    CMD_ANNOTATEDLIST,
    CMD_B,
    CMD_BADCODE,
    CMD_BOLD,
    CMD_BR,
    CMD_BRIEF,
    CMD_C,
    CMD_CAPTION,
    CMD_CODE,
    CMD_CODELINE,
    CMD_DIV,
    CMD_DOTS,
    CMD_E,
    CMD_ELSE,
    CMD_ENDCODE,
    CMD_ENDDIV,
    CMD_ENDFOOTNOTE,
    CMD_ENDIF,
    CMD_ENDLEGALESE,
    CMD_ENDLINK,
    CMD_ENDLIST,
    CMD_ENDMAPREF,
    CMD_ENDOMIT,
    CMD_ENDQUOTATION,
    CMD_ENDRAW,
    CMD_ENDSECTION1,
    CMD_ENDSECTION2,
    CMD_ENDSECTION3,
    CMD_ENDSECTION4,
    CMD_ENDSIDEBAR,
    CMD_ENDTABLE,
    CMD_FOOTNOTE,
    CMD_GENERATELIST,
    CMD_HEADER,
    CMD_HR,
    CMD_I,
    CMD_IF,
    CMD_IMAGE,
    CMD_IMPORTANT,
    CMD_INCLUDE,
    CMD_INLINEIMAGE,
    CMD_INDEX,
    CMD_INPUT,
    CMD_KEYWORD,
    CMD_L,
    CMD_LEGALESE,
    CMD_LI,
    CMD_LINK,
    CMD_LIST,
    CMD_META,
    CMD_NEWCODE,
    CMD_NOTE,
    CMD_O,
    CMD_OLDCODE,
    CMD_OMIT,
    CMD_OMITVALUE,
    CMD_OVERLOAD,
    CMD_PRINTLINE,
    CMD_PRINTTO,
    CMD_PRINTUNTIL,
    CMD_QUOTATION,
    CMD_QUOTEFILE,
    CMD_QUOTEFROMFILE,
    CMD_QUOTEFUNCTION,
    CMD_RAW,
    CMD_ROW,
    CMD_SA,
    CMD_SECTION1,
    CMD_SECTION2,
    CMD_SECTION3,
    CMD_SECTION4,
    CMD_SIDEBAR,
    CMD_SINCELIST,
    CMD_SKIPLINE,
    CMD_SKIPTO,
    CMD_SKIPUNTIL,
    CMD_SNIPPET,
    CMD_SPAN,
    CMD_SUB,
    CMD_SUP,
    CMD_TABLE,
    CMD_TABLEOFCONTENTS,
    CMD_TARGET,
    CMD_TT,
    CMD_UICONTROL,
    CMD_UNDERLINE,
    CMD_UNICODE,
    CMD_VALUE,
    CMD_WARNING,
    CMD_QML,
    CMD_ENDQML,
    CMD_CPP,
    CMD_ENDCPP,
    CMD_QMLTEXT,
    CMD_ENDQMLTEXT,
    CMD_CPPTEXT,
    CMD_ENDCPPTEXT,
    CMD_JS,
    CMD_ENDJS,
    NOT_A_CMD
};

static struct
{
    const char *english;
    int no;
    QString *alias;
} cmds[] = { { "a", CMD_A, nullptr },
             { "annotatedlist", CMD_ANNOTATEDLIST, nullptr },
             { "b", CMD_B, nullptr },
             { "badcode", CMD_BADCODE, nullptr },
             { "bold", CMD_BOLD, nullptr },
             { "br", CMD_BR, nullptr },
             { "brief", CMD_BRIEF, nullptr },
             { "c", CMD_C, nullptr },
             { "caption", CMD_CAPTION, nullptr },
             { "code", CMD_CODE, nullptr },
             { "codeline", CMD_CODELINE, nullptr },
             { "div", CMD_DIV, nullptr },
             { "dots", CMD_DOTS, nullptr },
             { "e", CMD_E, nullptr },
             { "else", CMD_ELSE, nullptr },
             { "endcode", CMD_ENDCODE, nullptr },
             { "enddiv", CMD_ENDDIV, nullptr },
             { "endfootnote", CMD_ENDFOOTNOTE, nullptr },
             { "endif", CMD_ENDIF, nullptr },
             { "endlegalese", CMD_ENDLEGALESE, nullptr },
             { "endlink", CMD_ENDLINK, nullptr },
             { "endlist", CMD_ENDLIST, nullptr },
             { "endmapref", CMD_ENDMAPREF, nullptr },
             { "endomit", CMD_ENDOMIT, nullptr },
             { "endquotation", CMD_ENDQUOTATION, nullptr },
             { "endraw", CMD_ENDRAW, nullptr },
             { "endsection1", CMD_ENDSECTION1, nullptr }, // ### don't document for now
             { "endsection2", CMD_ENDSECTION2, nullptr }, // ### don't document for now
             { "endsection3", CMD_ENDSECTION3, nullptr }, // ### don't document for now
             { "endsection4", CMD_ENDSECTION4, nullptr }, // ### don't document for now
             { "endsidebar", CMD_ENDSIDEBAR, nullptr },
             { "endtable", CMD_ENDTABLE, nullptr },
             { "footnote", CMD_FOOTNOTE, nullptr },
             { "generatelist", CMD_GENERATELIST, nullptr },
             { "header", CMD_HEADER, nullptr },
             { "hr", CMD_HR, nullptr },
             { "i", CMD_I, nullptr },
             { "if", CMD_IF, nullptr },
             { "image", CMD_IMAGE, nullptr },
             { "important", CMD_IMPORTANT, nullptr },
             { "include", CMD_INCLUDE, nullptr },
             { "inlineimage", CMD_INLINEIMAGE, nullptr },
             { "index", CMD_INDEX, nullptr }, // ### don't document for now
             { "input", CMD_INPUT, nullptr },
             { "keyword", CMD_KEYWORD, nullptr },
             { "l", CMD_L, nullptr },
             { "legalese", CMD_LEGALESE, nullptr },
             { "li", CMD_LI, nullptr },
             { "link", CMD_LINK, nullptr },
             { "list", CMD_LIST, nullptr },
             { "meta", CMD_META, nullptr },
             { "newcode", CMD_NEWCODE, nullptr },
             { "note", CMD_NOTE, nullptr },
             { "o", CMD_O, nullptr },
             { "oldcode", CMD_OLDCODE, nullptr },
             { "omit", CMD_OMIT, nullptr },
             { "omitvalue", CMD_OMITVALUE, nullptr },
             { "overload", CMD_OVERLOAD, nullptr },
             { "printline", CMD_PRINTLINE, nullptr },
             { "printto", CMD_PRINTTO, nullptr },
             { "printuntil", CMD_PRINTUNTIL, nullptr },
             { "quotation", CMD_QUOTATION, nullptr },
             { "quotefile", CMD_QUOTEFILE, nullptr },
             { "quotefromfile", CMD_QUOTEFROMFILE, nullptr },
             { "quotefunction", CMD_QUOTEFUNCTION, nullptr },
             { "raw", CMD_RAW, nullptr },
             { "row", CMD_ROW, nullptr },
             { "sa", CMD_SA, nullptr },
             { "section1", CMD_SECTION1, nullptr },
             { "section2", CMD_SECTION2, nullptr },
             { "section3", CMD_SECTION3, nullptr },
             { "section4", CMD_SECTION4, nullptr },
             { "sidebar", CMD_SIDEBAR, nullptr },
             { "sincelist", CMD_SINCELIST, nullptr },
             { "skipline", CMD_SKIPLINE, nullptr },
             { "skipto", CMD_SKIPTO, nullptr },
             { "skipuntil", CMD_SKIPUNTIL, nullptr },
             { "snippet", CMD_SNIPPET, nullptr },
             { "span", CMD_SPAN, nullptr },
             { "sub", CMD_SUB, nullptr },
             { "sup", CMD_SUP, nullptr },
             { "table", CMD_TABLE, nullptr },
             { "tableofcontents", CMD_TABLEOFCONTENTS, nullptr },
             { "target", CMD_TARGET, nullptr },
             { "tt", CMD_TT, nullptr },
             { "uicontrol", CMD_UICONTROL, nullptr },
             { "underline", CMD_UNDERLINE, nullptr },
             { "unicode", CMD_UNICODE, nullptr },
             { "value", CMD_VALUE, nullptr },
             { "warning", CMD_WARNING, nullptr },
             { "qml", CMD_QML, nullptr },
             { "endqml", CMD_ENDQML, nullptr },
             { "cpp", CMD_CPP, nullptr },
             { "endcpp", CMD_ENDCPP, nullptr },
             { "qmltext", CMD_QMLTEXT, nullptr },
             { "endqmltext", CMD_ENDQMLTEXT, nullptr },
             { "cpptext", CMD_CPPTEXT, nullptr },
             { "endcpptext", CMD_ENDCPPTEXT, nullptr },
             { "js", CMD_JS, nullptr },
             { "endjs", CMD_ENDJS, nullptr },
             { nullptr, 0, nullptr } };

int DocParser::s_tabSize;
QStringList DocParser::s_exampleFiles;
QStringList DocParser::s_exampleDirs;
QStringList DocParser::s_sourceFiles;
QStringList DocParser::s_sourceDirs;
QStringList DocParser::s_ignoreWords;
bool DocParser::s_quoting = false;

static QString cleanLink(const QString &link)
{
    qsizetype colonPos = link.indexOf(':');
    if ((colonPos == -1) || (!link.startsWith("file:") && !link.startsWith("mailto:")))
        return link;
    return link.mid(colonPos + 1).simplified();
}

void DocParser::initialize(const Config &config)
{
    s_tabSize = config.getInt(CONFIG_TABSIZE);
    s_exampleFiles = config.getCanonicalPathList(CONFIG_EXAMPLES);
    s_exampleDirs = config.getCanonicalPathList(CONFIG_EXAMPLEDIRS);
    s_sourceFiles = config.getCanonicalPathList(CONFIG_SOURCES);
    s_sourceDirs = config.getCanonicalPathList(CONFIG_SOURCEDIRS);
    s_ignoreWords = config.getStringList(CONFIG_IGNOREWORDS);

    int i = 0;
    while (cmds[i].english) {
        cmds[i].alias = new QString(Doc::alias(cmds[i].english));
        s_utilities.cmdHash.insert(*cmds[i].alias, cmds[i].no);

        if (cmds[i].no != i)
            Location::internalError(QStringLiteral("command %1 missing").arg(i));
        ++i;
    }

    // If any of the formats define quotinginformation, activate quoting
    DocParser::s_quoting = config.getBool(CONFIG_QUOTINGINFORMATION);
    for (const auto &format : config.getOutputFormats())
        DocParser::s_quoting = DocParser::s_quoting
                || config.getBool(format + Config::dot + CONFIG_QUOTINGINFORMATION);
}

void DocParser::terminate()
{
    s_exampleFiles.clear();
    s_exampleDirs.clear();
    s_sourceFiles.clear();
    s_sourceDirs.clear();

    int i = 0;
    while (cmds[i].english) {
        delete cmds[i].alias;
        cmds[i].alias = nullptr;
        ++i;
    }
}

/*!
  Parse the \a source string to build a Text data structure
  in \a docPrivate. The Text data structure is a linked list
  of Atoms.

  \a metaCommandSet is the set of metacommands that may be
  found in \a source. These metacommands are not markup text
  commands. They are topic commands and related metacommands.
 */
void DocParser::parse(const QString &source, DocPrivate *docPrivate,
                      const QSet<QString> &metaCommandSet, const QSet<QString> &possibleTopics)
{
    m_input = source;
    m_position = 0;
    m_inputLength = m_input.length();
    m_cachedLocation = docPrivate->m_start_loc;
    m_cachedPosition = 0;
    m_private = docPrivate;
    m_private->m_text << Atom::Nop;
    m_private->m_topics.clear();

    m_paragraphState = OutsideParagraph;
    m_inTableHeader = false;
    m_inTableRow = false;
    m_inTableItem = false;
    m_indexStartedParagraph = false;
    m_pendingParagraphLeftType = Atom::Nop;
    m_pendingParagraphRightType = Atom::Nop;

    m_braceDepth = 0;
    m_currentSection = Doc::NoSection;
    m_openedCommands.push(CMD_OMIT);
    m_quoter.reset();

    CodeMarker *marker = nullptr;
    Atom *currentLinkAtom = nullptr;
    QString p1, p2;
    QStack<bool> preprocessorSkipping;
    int numPreprocessorSkipping = 0;

    while (m_position < m_inputLength) {
        QChar ch = m_input.at(m_position);

        switch (ch.unicode()) {
        case '\\': {
            QString cmdStr;
            m_backslashPosition = m_position;
            ++m_position;
            while (m_position < m_inputLength) {
                ch = m_input.at(m_position);
                if (ch.isLetterOrNumber()) {
                    cmdStr += ch;
                    ++m_position;
                } else {
                    break;
                }
            }
            m_endPosition = m_position;
            if (cmdStr.isEmpty()) {
                if (m_position < m_inputLength) {
                    enterPara();
                    if (m_input.at(m_position).isSpace()) {
                        skipAllSpaces();
                        appendChar(QLatin1Char(' '));
                    } else {
                        appendChar(m_input.at(m_position++));
                    }
                }
            } else {
                // Ignore quoting atoms to make appendToCode()
                // append to the correct atom.
                if (!s_quoting || !isQuote(m_private->m_text.lastAtom()))
                    m_lastAtom = m_private->m_text.lastAtom();

                int cmd = s_utilities.cmdHash.value(cmdStr, NOT_A_CMD);
                switch (cmd) {
                case CMD_A:
                    enterPara();
                    p1 = getArgument();
                    append(Atom::FormattingLeft, ATOM_FORMATTING_PARAMETER);
                    append(Atom::String, p1);
                    append(Atom::FormattingRight, ATOM_FORMATTING_PARAMETER);
                    m_private->m_params.insert(p1);
                    break;
                case CMD_BADCODE:
                    leavePara();
                    append(Atom::CodeBad,
                           getCode(CMD_BADCODE, marker, getMetaCommandArgument(cmdStr)));
                    break;
                case CMD_BR:
                    enterPara();
                    append(Atom::BR);
                    break;
                case CMD_BOLD:
                    location().warning(QStringLiteral("'\\bold' is deprecated. Use '\\b'"));
                    Q_FALLTHROUGH();
                case CMD_B:
                    startFormat(ATOM_FORMATTING_BOLD, cmd);
                    break;
                case CMD_BRIEF:
                    leavePara();
                    enterPara(Atom::BriefLeft, Atom::BriefRight);
                    break;
                case CMD_C:
                    enterPara();
                    p1 = untabifyEtc(getArgument(true));
                    marker = CodeMarker::markerForCode(p1);
                    append(Atom::C, marker->markedUpCode(p1, nullptr, location()));
                    break;
                case CMD_CAPTION:
                    leavePara();
                    enterPara(Atom::CaptionLeft, Atom::CaptionRight);
                    break;
                case CMD_CODE:
                    leavePara();
                    append(Atom::Code, getCode(CMD_CODE, nullptr, getMetaCommandArgument(cmdStr)));
                    break;
                case CMD_QML:
                    leavePara();
                    append(Atom::Qml,
                           getCode(CMD_QML, CodeMarker::markerForLanguage(QLatin1String("QML")),
                                   getMetaCommandArgument(cmdStr)));
                    break;
                case CMD_QMLTEXT:
                    append(Atom::QmlText);
                    break;
                case CMD_JS:
                    leavePara();
                    append(Atom::JavaScript,
                           getCode(CMD_JS,
                                   CodeMarker::markerForLanguage(QLatin1String("JavaScript")),
                                   getMetaCommandArgument(cmdStr)));
                    break;
                case CMD_DIV:
                    leavePara();
                    p1 = getArgument(true);
                    append(Atom::DivLeft, p1);
                    m_openedCommands.push(cmd);
                    break;
                case CMD_ENDDIV:
                    leavePara();
                    append(Atom::DivRight);
                    closeCommand(cmd);
                    break;
                case CMD_CODELINE:
                    if (s_quoting) {
                        append(Atom::CodeQuoteCommand, cmdStr);
                        append(Atom::CodeQuoteArgument, " ");
                    }
                    if (isCode(m_lastAtom) && m_lastAtom->string().endsWith("\n\n"))
                        m_lastAtom->chopString();
                    appendToCode("\n");
                    break;
                case CMD_DOTS: {
                    QString arg = getOptionalArgument();
                    if (arg.isEmpty())
                        arg = "4";
                    if (s_quoting) {
                        append(Atom::CodeQuoteCommand, cmdStr);
                        append(Atom::CodeQuoteArgument, arg);
                    }
                    if (isCode(m_lastAtom) && m_lastAtom->string().endsWith("\n\n"))
                        m_lastAtom->chopString();

                    int indent = arg.toInt();
                    for (int i = 0; i < indent; ++i)
                        appendToCode(" ");
                    appendToCode("...\n");
                    break;
                }
                case CMD_ELSE:
                    if (!preprocessorSkipping.empty()) {
                        if (preprocessorSkipping.top()) {
                            --numPreprocessorSkipping;
                        } else {
                            ++numPreprocessorSkipping;
                        }
                        preprocessorSkipping.top() = !preprocessorSkipping.top();
                        (void)getRestOfLine(); // ### should ensure that it's empty
                        if (numPreprocessorSkipping)
                            skipToNextPreprocessorCommand();
                    } else {
                        location().warning(
                                QStringLiteral("Unexpected '\\%1'").arg(cmdName(CMD_ELSE)));
                    }
                    break;
                case CMD_ENDCODE:
                    closeCommand(cmd);
                    break;
                case CMD_ENDQML:
                    closeCommand(cmd);
                    break;
                case CMD_ENDQMLTEXT:
                    append(Atom::EndQmlText);
                    break;
                case CMD_ENDJS:
                    closeCommand(cmd);
                    break;
                case CMD_ENDFOOTNOTE:
                    if (closeCommand(cmd)) {
                        leavePara();
                        append(Atom::FootnoteRight);
                    }
                    break;
                case CMD_ENDIF:
                    if (preprocessorSkipping.count() > 0) {
                        if (preprocessorSkipping.pop())
                            --numPreprocessorSkipping;
                        (void)getRestOfLine(); // ### should ensure that it's empty
                        if (numPreprocessorSkipping)
                            skipToNextPreprocessorCommand();
                    } else {
                        location().warning(
                                QStringLiteral("Unexpected '\\%1'").arg(cmdName(CMD_ENDIF)));
                    }
                    break;
                case CMD_ENDLEGALESE:
                    if (closeCommand(cmd)) {
                        leavePara();
                        append(Atom::LegaleseRight);
                    }
                    break;
                case CMD_ENDLINK:
                    if (closeCommand(cmd)) {
                        if (m_private->m_text.lastAtom()->type() == Atom::String
                            && m_private->m_text.lastAtom()->string().endsWith(QLatin1Char(' ')))
                            m_private->m_text.lastAtom()->chopString();
                        append(Atom::FormattingRight, ATOM_FORMATTING_LINK);
                    }
                    break;
                case CMD_ENDLIST:
                    if (closeCommand(cmd)) {
                        leavePara();
                        if (m_openedLists.top().isStarted()) {
                            append(Atom::ListItemRight, m_openedLists.top().styleString());
                            append(Atom::ListRight, m_openedLists.top().styleString());
                        }
                        m_openedLists.pop();
                    }
                    break;
                case CMD_ENDOMIT:
                    closeCommand(cmd);
                    break;
                case CMD_ENDQUOTATION:
                    if (closeCommand(cmd)) {
                        leavePara();
                        append(Atom::QuotationRight);
                    }
                    break;
                case CMD_ENDRAW:
                    location().warning(
                            QStringLiteral("Unexpected '\\%1'").arg(cmdName(CMD_ENDRAW)));
                    break;
                case CMD_ENDSECTION1:
                    endSection(Doc::Section1, cmd);
                    break;
                case CMD_ENDSECTION2:
                    endSection(Doc::Section2, cmd);
                    break;
                case CMD_ENDSECTION3:
                    endSection(Doc::Section3, cmd);
                    break;
                case CMD_ENDSECTION4:
                    endSection(Doc::Section4, cmd);
                    break;
                case CMD_ENDSIDEBAR:
                    if (closeCommand(cmd)) {
                        leavePara();
                        append(Atom::SidebarRight);
                    }
                    break;
                case CMD_ENDTABLE:
                    if (closeCommand(cmd)) {
                        leaveTableRow();
                        append(Atom::TableRight);
                    }
                    break;
                case CMD_FOOTNOTE:
                    if (openCommand(cmd)) {
                        enterPara();
                        append(Atom::FootnoteLeft);
                    }
                    break;
                case CMD_ANNOTATEDLIST:
                    append(Atom::AnnotatedList, getArgument());
                    break;
                case CMD_SINCELIST:
                    append(Atom::SinceList, getRestOfLine().simplified());
                    break;
                case CMD_GENERATELIST: {
                    QString arg1 = getArgument();
                    QString arg2 = getOptionalArgument();
                    if (!arg2.isEmpty())
                        arg1 += " " + arg2;
                    append(Atom::GeneratedList, arg1);
                } break;
                case CMD_HEADER:
                    if (m_openedCommands.top() == CMD_TABLE) {
                        leaveTableRow();
                        append(Atom::TableHeaderLeft);
                        m_inTableHeader = true;
                    } else {
                        if (m_openedCommands.contains(CMD_TABLE))
                            location().warning(QStringLiteral("Cannot use '\\%1' within '\\%2'")
                                                       .arg(cmdName(CMD_HEADER),
                                                            cmdName(m_openedCommands.top())));
                        else
                            location().warning(
                                    QStringLiteral("Cannot use '\\%1' outside of '\\%2'")
                                            .arg(cmdName(CMD_HEADER), cmdName(CMD_TABLE)));
                    }
                    break;
                case CMD_I:
                    location().warning(QStringLiteral(
                            "'\\i' is deprecated. Use '\\e' for italic or '\\li' for list item"));
                    Q_FALLTHROUGH();
                case CMD_E:
                    startFormat(ATOM_FORMATTING_ITALIC, cmd);
                    break;
                case CMD_HR:
                    leavePara();
                    append(Atom::HR);
                    break;
                case CMD_IF:
                    preprocessorSkipping.push(!Tokenizer::isTrue(getRestOfLine()));
                    if (preprocessorSkipping.top())
                        ++numPreprocessorSkipping;
                    if (numPreprocessorSkipping)
                        skipToNextPreprocessorCommand();
                    break;
                case CMD_IMAGE:
                    leaveValueList();
                    append(Atom::Image, getArgument());
                    append(Atom::ImageText, getRestOfLine());
                    break;
                case CMD_IMPORTANT:
                    leavePara();
                    enterPara(Atom::ImportantLeft, Atom::ImportantRight);
                    break;
                case CMD_INCLUDE:
                case CMD_INPUT: {
                    QString fileName = getArgument();
                    QString identifier = getRestOfLine();
                    include(fileName, identifier);
                    break;
                }
                case CMD_INLINEIMAGE:
                    enterPara();
                    append(Atom::InlineImage, getArgument());
                    append(Atom::ImageText, getRestOfLine());
                    append(Atom::String, " ");
                    break;
                case CMD_INDEX:
                    if (m_paragraphState == OutsideParagraph) {
                        enterPara();
                        m_indexStartedParagraph = true;
                    } else {
                        const Atom *last = m_private->m_text.lastAtom();
                        if (m_indexStartedParagraph
                            && (last->type() != Atom::FormattingRight
                                || last->string() != ATOM_FORMATTING_INDEX))
                            m_indexStartedParagraph = false;
                    }
                    startFormat(ATOM_FORMATTING_INDEX, cmd);
                    break;
                case CMD_KEYWORD:
                    insertTarget(getRestOfLine(), true);
                    break;
                case CMD_L:
                    enterPara();
                    if (isLeftBracketAhead())
                        p2 = getBracketedArgument();
                    if (isLeftBraceAhead()) {
                        p1 = getArgument();
                        append(p1, p2);
                        if (!p2.isEmpty() && !(m_private->m_text.lastAtom()->error().isEmpty()))
                            location().warning(
                                    QStringLiteral(
                                            "Check parameter in '[ ]' of '\\l' command: '%1', "
                                            "possible misspelling, or unrecognized module name")
                                            .arg(m_private->m_text.lastAtom()->error()));
                        if (isLeftBraceAhead()) {
                            currentLinkAtom = m_private->m_text.lastAtom();
                            startFormat(ATOM_FORMATTING_LINK, cmd);
                        } else {
                            append(Atom::FormattingLeft, ATOM_FORMATTING_LINK);
                            append(Atom::String, cleanLink(p1));
                            append(Atom::FormattingRight, ATOM_FORMATTING_LINK);
                        }
                    } else {
                        p1 = getArgument();
                        append(p1, p2);
                        if (!p2.isEmpty() && !(m_private->m_text.lastAtom()->error().isEmpty()))
                            location().warning(
                                    QStringLiteral(
                                            "Check parameter in '[ ]' of '\\l' command: '%1', "
                                            "possible misspelling, or unrecognized module name")
                                            .arg(m_private->m_text.lastAtom()->error()));
                        append(Atom::FormattingLeft, ATOM_FORMATTING_LINK);
                        append(Atom::String, cleanLink(p1));
                        append(Atom::FormattingRight, ATOM_FORMATTING_LINK);
                    }
                    p2.clear();
                    break;
                case CMD_LEGALESE:
                    leavePara();
                    if (openCommand(cmd))
                        append(Atom::LegaleseLeft);
                    docPrivate->m_hasLegalese = true;
                    break;
                case CMD_LINK:
                    if (openCommand(cmd)) {
                        enterPara();
                        p1 = getArgument();
                        append(p1);
                        append(Atom::FormattingLeft, ATOM_FORMATTING_LINK);
                        skipSpacesOrOneEndl();
                    }
                    break;
                case CMD_LIST:
                    if (openCommand(cmd)) {
                        leavePara();
                        m_openedLists.push(OpenedList(location(), getOptionalArgument()));
                    }
                    break;
                case CMD_META:
                    m_private->constructExtra();
                    p1 = getArgument();
                    m_private->extra->m_metaMap.insert(p1, getArgument());
                    break;
                case CMD_NEWCODE:
                    location().warning(
                            QStringLiteral("Unexpected '\\%1'").arg(cmdName(CMD_NEWCODE)));
                    break;
                case CMD_NOTE:
                    leavePara();
                    enterPara(Atom::NoteLeft, Atom::NoteRight);
                    break;
                case CMD_O:
                    location().warning(QStringLiteral("'\\o' is deprecated. Use '\\li'"));
                    Q_FALLTHROUGH();
                case CMD_LI:
                    leavePara();
                    if (m_openedCommands.top() == CMD_LIST) {
                        if (m_openedLists.top().isStarted())
                            append(Atom::ListItemRight, m_openedLists.top().styleString());
                        else
                            append(Atom::ListLeft, m_openedLists.top().styleString());
                        m_openedLists.top().next();
                        append(Atom::ListItemNumber, m_openedLists.top().numberString());
                        append(Atom::ListItemLeft, m_openedLists.top().styleString());
                        enterPara();
                    } else if (m_openedCommands.top() == CMD_TABLE) {
                        p1 = "1,1";
                        p2.clear();
                        if (isLeftBraceAhead()) {
                            p1 = getArgument();
                            if (isLeftBraceAhead())
                                p2 = getArgument();
                        }

                        if (!m_inTableHeader && !m_inTableRow) {
                            location().warning(
                                    QStringLiteral("Missing '\\%1' or '\\%2' before '\\%3'")
                                            .arg(cmdName(CMD_HEADER), cmdName(CMD_ROW),
                                                 cmdName(CMD_LI)));
                            append(Atom::TableRowLeft);
                            m_inTableRow = true;
                        } else if (m_inTableItem) {
                            append(Atom::TableItemRight);
                            m_inTableItem = false;
                        }

                        append(Atom::TableItemLeft, p1, p2);
                        m_inTableItem = true;
                    } else
                        location().warning(
                                QStringLiteral("Command '\\%1' outside of '\\%2' and '\\%3'")
                                        .arg(cmdName(cmd), cmdName(CMD_LIST), cmdName(CMD_TABLE)));
                    break;
                case CMD_OLDCODE:
                    leavePara();
                    append(Atom::CodeOld, getCode(CMD_OLDCODE, marker));
                    append(Atom::CodeNew, getCode(CMD_NEWCODE, marker));
                    break;
                case CMD_OMIT:
                    getUntilEnd(cmd);
                    break;
                case CMD_OMITVALUE: {
                    p1 = getArgument();
                    if (!m_private->m_enumItemList.contains(p1))
                        m_private->m_enumItemList.append(p1);
                    if (!m_private->m_omitEnumItemList.contains(p1))
                        m_private->m_omitEnumItemList.append(p1);
                    skipSpacesOrOneEndl();
                    // Skip potential description paragraph
                    while (m_position < m_inputLength && !isBlankLine()) {
                        skipAllSpaces();
                        if (qsizetype pos = m_position; pos < m_input.size()
                                && m_input.at(pos++).unicode() == '\\') {
                            QString nextCmdStr;
                            while (pos < m_input.size() && m_input[pos].isLetterOrNumber())
                                nextCmdStr += m_input[pos++];
                            int nextCmd = s_utilities.cmdHash.value(cmdStr, NOT_A_CMD);
                            if (nextCmd == cmd || nextCmd == CMD_VALUE)
                                break;
                        }
                        getRestOfLine();
                    }
                    break;
                }
                case CMD_PRINTLINE: {
                    leavePara();
                    QString rest = getRestOfLine();
                    if (s_quoting) {
                        append(Atom::CodeQuoteCommand, cmdStr);
                        append(Atom::CodeQuoteArgument, rest);
                    }
                    appendToCode(m_quoter.quoteLine(location(), cmdStr, rest));
                    break;
                }
                case CMD_PRINTTO: {
                    leavePara();
                    QString rest = getRestOfLine();
                    if (s_quoting) {
                        append(Atom::CodeQuoteCommand, cmdStr);
                        append(Atom::CodeQuoteArgument, rest);
                    }
                    appendToCode(m_quoter.quoteTo(location(), cmdStr, rest));
                    break;
                }
                case CMD_PRINTUNTIL: {
                    leavePara();
                    QString rest = getRestOfLine();
                    if (s_quoting) {
                        append(Atom::CodeQuoteCommand, cmdStr);
                        append(Atom::CodeQuoteArgument, rest);
                    }
                    appendToCode(m_quoter.quoteUntil(location(), cmdStr, rest));
                    break;
                }
                case CMD_QUOTATION:
                    if (openCommand(cmd)) {
                        leavePara();
                        append(Atom::QuotationLeft);
                    }
                    break;
                case CMD_QUOTEFILE: {
                    leavePara();
                    QString fileName = getArgument();
                    Doc::quoteFromFile(location(), m_quoter, fileName);
                    if (s_quoting) {
                        append(Atom::CodeQuoteCommand, cmdStr);
                        append(Atom::CodeQuoteArgument, fileName);
                    }
                    append(Atom::Code, m_quoter.quoteTo(location(), cmdStr, QString()));
                    m_quoter.reset();
                    break;
                }
                case CMD_QUOTEFROMFILE: {
                    leavePara();
                    QString arg = getArgument();
                    if (s_quoting) {
                        append(Atom::CodeQuoteCommand, cmdStr);
                        append(Atom::CodeQuoteArgument, arg);
                    }
                    Doc::quoteFromFile(location(), m_quoter, arg);
                    break;
                }
                case CMD_QUOTEFUNCTION: {
                    leavePara();
                    marker = quoteFromFile();
                    p1 = getRestOfLine();
                    if (s_quoting) {
                        append(Atom::CodeQuoteCommand, cmdStr);
                        append(Atom::CodeQuoteArgument, slashed(marker->functionEndRegExp(p1)));
                    }
                    m_quoter.quoteTo(location(), cmdStr, slashed(marker->functionBeginRegExp(p1)));
                    append(Atom::Code,
                           m_quoter.quoteUntil(location(), cmdStr,
                                               slashed(marker->functionEndRegExp(p1))));
                    m_quoter.reset();
                    break;
                }
                case CMD_RAW:
                    leavePara();
                    p1 = getRestOfLine();
                    if (p1.isEmpty())
                        location().warning(QStringLiteral("Missing format name after '\\%1'")
                                                   .arg(cmdName(CMD_RAW)));
                    append(Atom::FormatIf, p1);
                    append(Atom::RawString, untabifyEtc(getUntilEnd(cmd)));
                    append(Atom::FormatElse);
                    append(Atom::FormatEndif);
                    break;
                case CMD_ROW:
                    if (m_openedCommands.top() == CMD_TABLE) {
                        p1.clear();
                        if (isLeftBraceAhead())
                            p1 = getArgument(true);
                        leaveTableRow();
                        append(Atom::TableRowLeft, p1);
                        m_inTableRow = true;
                    } else {
                        if (m_openedCommands.contains(CMD_TABLE))
                            location().warning(QStringLiteral("Cannot use '\\%1' within '\\%2'")
                                                       .arg(cmdName(CMD_ROW),
                                                            cmdName(m_openedCommands.top())));
                        else
                            location().warning(QStringLiteral("Cannot use '\\%1' outside of '\\%2'")
                                                       .arg(cmdName(CMD_ROW), cmdName(CMD_TABLE)));
                    }
                    break;
                case CMD_SA:
                    parseAlso();
                    break;
                case CMD_SECTION1:
                    startSection(Doc::Section1, cmd);
                    break;
                case CMD_SECTION2:
                    startSection(Doc::Section2, cmd);
                    break;
                case CMD_SECTION3:
                    startSection(Doc::Section3, cmd);
                    break;
                case CMD_SECTION4:
                    startSection(Doc::Section4, cmd);
                    break;
                case CMD_SIDEBAR:
                    if (openCommand(cmd)) {
                        leavePara();
                        append(Atom::SidebarLeft);
                    }
                    break;
                case CMD_SKIPLINE: {
                    leavePara();
                    QString rest = getRestOfLine();
                    if (s_quoting) {
                        append(Atom::CodeQuoteCommand, cmdStr);
                        append(Atom::CodeQuoteArgument, rest);
                    }
                    m_quoter.quoteLine(location(), cmdStr, rest);
                    break;
                }
                case CMD_SKIPTO: {
                    leavePara();
                    QString rest = getRestOfLine();
                    if (s_quoting) {
                        append(Atom::CodeQuoteCommand, cmdStr);
                        append(Atom::CodeQuoteArgument, rest);
                    }
                    m_quoter.quoteTo(location(), cmdStr, rest);
                    break;
                }
                case CMD_SKIPUNTIL: {
                    leavePara();
                    QString rest = getRestOfLine();
                    if (s_quoting) {
                        append(Atom::CodeQuoteCommand, cmdStr);
                        append(Atom::CodeQuoteArgument, rest);
                    }
                    m_quoter.quoteUntil(location(), cmdStr, rest);
                    break;
                }
                case CMD_SPAN:
                    p1 = ATOM_FORMATTING_SPAN + getArgument(true);
                    startFormat(p1, cmd);
                    break;
                case CMD_SNIPPET: {
                    leavePara();
                    QString snippet = getArgument();
                    QString identifier = getRestOfLine();
                    if (s_quoting) {
                        append(Atom::SnippetCommand, cmdStr);
                        append(Atom::SnippetLocation, snippet);
                        append(Atom::SnippetIdentifier, identifier);
                    }
                    marker = Doc::quoteFromFile(location(), m_quoter, snippet);
                    appendToCode(m_quoter.quoteSnippet(location(), identifier), marker->atomType());
                    break;
                }
                case CMD_SUB:
                    startFormat(ATOM_FORMATTING_SUBSCRIPT, cmd);
                    break;
                case CMD_SUP:
                    startFormat(ATOM_FORMATTING_SUPERSCRIPT, cmd);
                    break;
                case CMD_TABLE:
                    p1 = getOptionalArgument();
                    p2 = getOptionalArgument();
                    if (openCommand(cmd)) {
                        leavePara();
                        append(Atom::TableLeft, p1, p2);
                        m_inTableHeader = false;
                        m_inTableRow = false;
                        m_inTableItem = false;
                    }
                    break;
                case CMD_TABLEOFCONTENTS:
                    p1 = "1";
                    if (isLeftBraceAhead())
                        p1 = getArgument();
                    p1 += QLatin1Char(',');
                    p1 += QString::number((int)getSectioningUnit());
                    append(Atom::TableOfContents, p1);
                    break;
                case CMD_TARGET:
                    insertTarget(getRestOfLine(), false);
                    break;
                case CMD_TT:
                    startFormat(ATOM_FORMATTING_TELETYPE, cmd);
                    break;
                case CMD_UICONTROL:
                    startFormat(ATOM_FORMATTING_UICONTROL, cmd);
                    break;
                case CMD_UNDERLINE:
                    startFormat(ATOM_FORMATTING_UNDERLINE, cmd);
                    break;
                case CMD_UNICODE: {
                    enterPara();
                    p1 = getArgument();
                    bool ok;
                    uint unicodeChar = p1.toUInt(&ok, 0);
                    if (!ok || (unicodeChar == 0x0000) || (unicodeChar > 0xFFFE))
                        location().warning(
                                QStringLiteral("Invalid Unicode character '%1' specified with '%2'")
                                        .arg(p1, cmdName(CMD_UNICODE)));
                    else
                        append(Atom::String, QChar(unicodeChar));
                    break;
                }
                case CMD_VALUE:
                    leaveValue();
                    if (m_openedLists.top().style() == OpenedList::Value) {
                        QString p2;
                        p1 = getArgument();
                        if (p1.startsWith(QLatin1String("[since "))
                            && p1.endsWith(QLatin1String("]"))) {
                            p2 = p1.mid(7, p1.length() - 8);
                            p1 = getArgument();
                        }
                        if (!m_private->m_enumItemList.contains(p1))
                            m_private->m_enumItemList.append(p1);

                        m_openedLists.top().next();
                        append(Atom::ListTagLeft, ATOM_LIST_VALUE);
                        append(Atom::String, p1);
                        append(Atom::ListTagRight, ATOM_LIST_VALUE);
                        if (!p2.isEmpty()) {
                            append(Atom::SinceTagLeft, ATOM_LIST_VALUE);
                            append(Atom::String, p2);
                            append(Atom::SinceTagRight, ATOM_LIST_VALUE);
                        }
                        append(Atom::ListItemLeft, ATOM_LIST_VALUE);

                        skipSpacesOrOneEndl();
                        if (isBlankLine())
                            append(Atom::Nop);
                    } else {
                        // ### unknown problems
                    }
                    break;
                case CMD_WARNING:
                    leavePara();
                    enterPara(Atom::WarningLeft, Atom::WarningRight);
                    break;
                case CMD_OVERLOAD:
                    m_private->m_metacommandsUsed.insert(cmdStr);
                    p1.clear();
                    if (!isBlankLine())
                        p1 = getRestOfLine();
                    if (!p1.isEmpty()) {
                        append(Atom::ParaLeft);
                        append(Atom::String, "This function overloads ");
                        append(Atom::AutoLink, p1);
                        append(Atom::String, ".");
                        append(Atom::ParaRight);
                    } else {
                        append(Atom::ParaLeft);
                        append(Atom::String, "This is an overloaded function.");
                        append(Atom::ParaRight);
                        p1 = getMetaCommandArgument(cmdStr);
                    }
                    m_private->m_metaCommandMap[cmdStr].append(ArgPair(p1, QString()));
                    break;
                case NOT_A_CMD:
                    if (metaCommandSet.contains(cmdStr)) {
                        QString arg;
                        QString bracketedArg;
                        m_private->m_metacommandsUsed.insert(cmdStr);
                        if (isLeftBracketAhead())
                            bracketedArg = getBracketedArgument();
                        // Force a linebreak after \obsolete or \deprecated
                        // to treat potential arguments as a new text paragraph.
                        if (m_position < m_inputLength
                            && (cmdStr == QLatin1String("obsolete")
                                || cmdStr == QLatin1String("deprecated")))
                            m_input[m_position] = '\n';
                        else
                            arg = getMetaCommandArgument(cmdStr);
                        m_private->m_metaCommandMap[cmdStr].append(ArgPair(arg, bracketedArg));
                        if (possibleTopics.contains(cmdStr)) {
                            if (!cmdStr.endsWith(QLatin1String("propertygroup")))
                                m_private->m_topics.append(Topic(cmdStr, arg));
                        }
                    } else if (s_utilities.macroHash.contains(cmdStr)) {
                        const Macro &macro = s_utilities.macroHash.value(cmdStr);
                        int numPendingFi = 0;
                        int numFormatDefs = 0;
                        QString matchExpr;
                        for (auto it = macro.m_otherDefs.constBegin();
                             it != macro.m_otherDefs.constEnd(); ++it) {
                            if (it.key() == "match") {
                                matchExpr = it.value();
                            } else {
                                append(Atom::FormatIf, it.key());
                                expandMacro(cmdStr, *it, macro.numParams);
                                ++numFormatDefs;
                                if (it == macro.m_otherDefs.constEnd()) {
                                    append(Atom::FormatEndif);
                                } else {
                                    append(Atom::FormatElse);
                                    ++numPendingFi;
                                }
                            }
                        }
                        while (numPendingFi-- > 0)
                            append(Atom::FormatEndif);

                        if (!macro.m_defaultDef.isEmpty()) {
                            if (numFormatDefs > 0) {
                                macro.m_defaultDefLocation.warning(
                                        QStringLiteral("Macro cannot have both "
                                                       "format-specific and qdoc-"
                                                       "syntax definitions"));
                            } else {
                                QString expanded = expandMacroToString(cmdStr, macro.m_defaultDef,
                                                                       macro.numParams, matchExpr);
                                m_input.replace(m_backslashPosition,
                                                m_endPosition - m_backslashPosition, expanded);
                                m_inputLength = m_input.length();
                                m_position = m_backslashPosition;
                            }
                        }
                    } else if (isAutoLinkString(cmdStr)) {
                        appendWord(cmdStr);
                    } else {
                        if (!cmdStr.endsWith("propertygroup")) {
                            // The QML and JS property group commands are no longer required
                            // for grouping QML and JS properties. They are allowed but ignored.
                            location().warning(QStringLiteral("Unknown command '\\%1'").arg(cmdStr),
                                               detailsUnknownCommand(metaCommandSet, cmdStr));
                        }
                        enterPara();
                        append(Atom::UnknownCommand, cmdStr);
                    }
                }
            } // case '\\' (qdoc markup command)
            break;
        }
        case '{':
            enterPara();
            appendChar('{');
            ++m_braceDepth;
            ++m_position;
            break;
        case '}': {
            --m_braceDepth;
            ++m_position;

            auto format = m_pendingFormats.find(m_braceDepth);
            if (format == m_pendingFormats.end()) {
                enterPara();
                appendChar('}');
            } else {
                append(Atom::FormattingRight, *format);
                if (*format == ATOM_FORMATTING_INDEX) {
                    if (m_indexStartedParagraph)
                        skipAllSpaces();
                } else if (*format == ATOM_FORMATTING_LINK) {
                    // hack for C++ to support links like
                    // \l{QString::}{count()}
                    if (currentLinkAtom && currentLinkAtom->string().endsWith("::")) {
                        QString suffix =
                                Text::subText(currentLinkAtom, m_private->m_text.lastAtom())
                                        .toString();
                        currentLinkAtom->appendString(suffix);
                    }
                    currentLinkAtom = nullptr;
                }
                m_pendingFormats.erase(format);
            }
            break;
        }
            // Do not parse content after '//!' comments
        case '/': {
            if (m_position + 2 < m_inputLength)
                if (m_input.at(m_position + 1) == '/')
                    if (m_input.at(m_position + 2) == '!') {
                        m_position += 2;
                        getRestOfLine();
                        break;
                    }
            Q_FALLTHROUGH(); // fall through
        }
        default: {
            bool newWord;
            switch (m_private->m_text.lastAtom()->type()) {
            case Atom::ParaLeft:
                newWord = true;
                break;
            default:
                newWord = false;
            }

            if (m_paragraphState == OutsideParagraph) {
                if (ch.isSpace()) {
                    ++m_position;
                    newWord = false;
                } else {
                    enterPara();
                    newWord = true;
                }
            } else {
                if (ch.isSpace()) {
                    ++m_position;
                    if ((ch == '\n')
                        && (m_paragraphState == InSingleLineParagraph || isBlankLine())) {
                        leavePara();
                        newWord = false;
                    } else {
                        appendChar(' ');
                        newWord = true;
                    }
                } else {
                    newWord = true;
                }
            }

            if (newWord) {
                qsizetype startPos = m_position;
                // No auto-linking inside links
                bool autolink = (!m_pendingFormats.isEmpty() &&
                        m_pendingFormats.last() == ATOM_FORMATTING_LINK) ?
                            false : isAutoLinkString(m_input, m_position);
                if (m_position == startPos) {
                    if (!ch.isSpace()) {
                        appendChar(ch);
                        ++m_position;
                    }
                } else {
                    QString word = m_input.mid(startPos, m_position - startPos);
                    if (autolink) {
                        if (s_ignoreWords.contains(word) || word.startsWith(QString("__")))
                            appendWord(word);
                        else
                            append(Atom::AutoLink, word);
                    } else {
                        appendWord(word);
                    }
                }
            }
        } // default:
        } // switch (ch.unicode())
    }
    leaveValueList();

    // for compatibility
    if (m_openedCommands.top() == CMD_LEGALESE) {
        append(Atom::LegaleseRight);
        m_openedCommands.pop();
    }

    if (m_openedCommands.top() != CMD_OMIT) {
        location().warning(
                QStringLiteral("Missing '\\%1'").arg(endCmdName(m_openedCommands.top())));
    } else if (preprocessorSkipping.count() > 0) {
        location().warning(QStringLiteral("Missing '\\%1'").arg(cmdName(CMD_ENDIF)));
    }

    if (m_currentSection > Doc::NoSection) {
        append(Atom::SectionRight, QString::number(m_currentSection));
        m_currentSection = Doc::NoSection;
    }

    m_private->m_text.stripFirstAtom();
}

/*!
  Returns the current location.
 */
Location &DocParser::location()
{
    while (!m_openedInputs.isEmpty() && m_openedInputs.top() <= m_position) {
        m_cachedLocation.pop();
        m_cachedPosition = m_openedInputs.pop();
    }
    while (m_cachedPosition < m_position)
        m_cachedLocation.advance(m_input.at(m_cachedPosition++));
    return m_cachedLocation;
}

QString DocParser::detailsUnknownCommand(const QSet<QString> &metaCommandSet, const QString &str)
{
    QSet<QString> commandSet = metaCommandSet;
    int i = 0;
    while (cmds[i].english != nullptr) {
        commandSet.insert(*cmds[i].alias);
        ++i;
    }

    if (s_utilities.aliasMap.contains(str))
        return QStringLiteral("The command '\\%1' was renamed '\\%2' by the configuration"
                              " file. Use the new name.")
                .arg(str, s_utilities.aliasMap[str]);

    QString best = nearestName(str, commandSet);
    if (best.isEmpty())
        return QString();
    return QStringLiteral("Maybe you meant '\\%1'?").arg(best);
}

void DocParser::insertTarget(const QString &target, bool keyword)
{
    if (m_targetMap.contains(target)) {
        location().warning(QStringLiteral("Duplicate target name '%1'").arg(target));
        m_targetMap[target].warning(QStringLiteral("(The previous occurrence is here)"));
    } else {
        m_targetMap.insert(target, location());
        m_private->constructExtra();
        if (keyword) {
            append(Atom::Keyword, target);
            m_private->extra->m_keywords.append(m_private->m_text.lastAtom());
        } else {
            append(Atom::Target, target);
            m_private->extra->m_targets.append(m_private->m_text.lastAtom());
        }
    }
}

void DocParser::include(const QString &fileName, const QString &identifier)
{
    if (location().depth() > 16)
        location().fatal(QStringLiteral("Too many nested '\\%1's").arg(cmdName(CMD_INCLUDE)));
    QString filePath = Config::instance().getIncludeFilePath(fileName);
    if (filePath.isEmpty()) {
        location().warning(QStringLiteral("Cannot find qdoc include file '%1'").arg(fileName));
    } else {
        QFile inFile(filePath);
        if (!inFile.open(QFile::ReadOnly)) {
            location().warning(
                    QStringLiteral("Cannot open qdoc include file '%1'").arg(filePath));
        } else {
            location().push(fileName);
            QTextStream inStream(&inFile);
            QString includedStuff = inStream.readAll();
            inFile.close();

            if (identifier.isEmpty()) {
                m_input.insert(m_position, includedStuff);
                m_inputLength = m_input.length();
                m_openedInputs.push(m_position + includedStuff.length());
            } else {
                QStringList lineBuffer = includedStuff.split(QLatin1Char('\n'));
                int i = 0;
                int startLine = -1;
                while (i < lineBuffer.size()) {
                    if (lineBuffer[i].startsWith("//!")) {
                        if (lineBuffer[i].contains(identifier)) {
                            startLine = i + 1;
                            break;
                        }
                    }
                    ++i;
                }
                if (startLine < 0) {
                    location().warning(
                            QStringLiteral("Cannot find '%1' in '%2'").arg(identifier, filePath));
                    return;
                }
                QString result;
                i = startLine;
                do {
                    if (lineBuffer[i].startsWith("//!")) {
                        if (i < lineBuffer.size()) {
                            if (lineBuffer[i].contains(identifier)) {
                                break;
                            }
                        }
                    } else
                        result += lineBuffer[i] + QLatin1Char('\n');
                    ++i;
                } while (i < lineBuffer.size());
                if (result.isEmpty()) {
                    location().warning(QStringLiteral("Empty qdoc snippet '%1' in '%2'")
                                               .arg(identifier, filePath));
                } else {
                    m_input.insert(m_position, result);
                    m_inputLength = m_input.length();
                    m_openedInputs.push(m_position + result.length());
                }
            }
        }
    }
}

void DocParser::startFormat(const QString &format, int cmd)
{
    enterPara();

    for (const auto &item : qAsConst(m_pendingFormats)) {
        if (item == format) {
            location().warning(QStringLiteral("Cannot nest '\\%1' commands").arg(cmdName(cmd)));
            return;
        }
    }

    append(Atom::FormattingLeft, format);

    if (isLeftBraceAhead()) {
        skipSpacesOrOneEndl();
        m_pendingFormats.insert(m_braceDepth, format);
        ++m_braceDepth;
        ++m_position;
    } else {
        append(Atom::String, getArgument());
        append(Atom::FormattingRight, format);
        if (format == ATOM_FORMATTING_INDEX && m_indexStartedParagraph) {
            skipAllSpaces();
            m_indexStartedParagraph = false;
        }
    }
}

bool DocParser::openCommand(int cmd)
{
    int outer = m_openedCommands.top();
    bool ok = true;

    if (cmd != CMD_LINK) {
        if (outer == CMD_LIST) {
            ok = (cmd == CMD_FOOTNOTE || cmd == CMD_LIST);
        } else if (outer == CMD_SIDEBAR) {
            ok = (cmd == CMD_LIST || cmd == CMD_QUOTATION || cmd == CMD_SIDEBAR);
        } else if (outer == CMD_QUOTATION) {
            ok = (cmd == CMD_LIST);
        } else if (outer == CMD_TABLE) {
            ok = (cmd == CMD_LIST || cmd == CMD_FOOTNOTE || cmd == CMD_QUOTATION);
        } else if (outer == CMD_FOOTNOTE || outer == CMD_LINK) {
            ok = false;
        }
    }

    if (ok) {
        m_openedCommands.push(cmd);
    } else {
        location().warning(
                QStringLiteral("Can't use '\\%1' in '\\%2'").arg(cmdName(cmd), cmdName(outer)));
    }
    return ok;
}

/*!
    Returns \c true if \a word qualifies for auto-linking.
*/
inline bool DocParser::isAutoLinkString(const QString &word)
{
    qsizetype start = 0;
    return isAutoLinkString(word, start);
}

bool DocParser::isAutoLinkString(const QString &word, qsizetype &curPos)
{
    qsizetype len = word.size();
    qsizetype startPos = curPos;
    int numUppercase = 0;
    int numLowercase = 0;
    int numStrangeSymbols = 0;

    while (curPos < len) {
        unsigned char latin1Ch = word.at(curPos).toLatin1();
        if (islower(latin1Ch)) {
            ++numLowercase;
            ++curPos;
        } else if (isupper(latin1Ch)) {
            if (curPos > startPos)
                ++numUppercase;
            ++curPos;
        } else if (isdigit(latin1Ch)) {
            if (curPos > startPos)
                ++curPos;
            else
                break;
        } else if (latin1Ch == '_' || latin1Ch == '@') {
            ++numStrangeSymbols;
            ++curPos;
        } else if ((latin1Ch == ':') && (curPos < len - 1)
                   && (word.at(curPos + 1) == QLatin1Char(':'))) {
            ++numStrangeSymbols;
            curPos += 2;
        } else if (latin1Ch == '(') {
            if (curPos > startPos) {
                if ((curPos < len - 1) && (word.at(curPos + 1) == QLatin1Char(')'))) {
                    ++numStrangeSymbols;
                    m_position += 2;
                    break;
                } else {
                    break;
                }
            } else {
                break;
            }
        } else {
            break;
        }
    }
    return ((numUppercase >= 1 && numLowercase >= 2) || numStrangeSymbols > 0);
}

bool DocParser::closeCommand(int endCmd)
{
    if (endCmdFor(m_openedCommands.top()) == endCmd && m_openedCommands.size() > 1) {
        m_openedCommands.pop();
        return true;
    } else {
        bool contains = false;
        QStack<int> opened2 = m_openedCommands;
        while (opened2.size() > 1) {
            if (endCmdFor(opened2.top()) == endCmd) {
                contains = true;
                break;
            }
            opened2.pop();
        }

        if (contains) {
            while (endCmdFor(m_openedCommands.top()) != endCmd && m_openedCommands.size() > 1) {
                location().warning(
                        QStringLiteral("Missing '\\%1' before '\\%2'")
                                .arg(endCmdName(m_openedCommands.top()), cmdName(endCmd)));
                m_openedCommands.pop();
            }
        } else {
            location().warning(QStringLiteral("Unexpected '\\%1'").arg(cmdName(endCmd)));
        }
        return false;
    }
}

void DocParser::startSection(Doc::Sections unit, int cmd)
{
    leaveValueList();

    if (m_currentSection == Doc::NoSection) {
        m_currentSection = (Doc::Sections)(unit);
        m_private->constructExtra();
    } else
        endSection(unit, cmd);

    append(Atom::SectionLeft, QString::number(unit));
    m_private->constructExtra();
    m_private->extra->m_tableOfContents.append(m_private->m_text.lastAtom());
    m_private->extra->m_tableOfContentsLevels.append(unit);
    enterPara(Atom::SectionHeadingLeft, Atom::SectionHeadingRight, QString::number(unit));
    m_currentSection = unit;
}

void DocParser::endSection(int, int) // (int unit, int endCmd)
{
    leavePara();
    append(Atom::SectionRight, QString::number(m_currentSection));
    m_currentSection = (Doc::NoSection);
}

void DocParser::parseAlso()
{
    leavePara();
    skipSpacesOnLine();
    while (m_position < m_inputLength && m_input[m_position] != '\n') {
        QString target;
        QString str;
        bool skipMe = false;

        if (m_input[m_position] == '{') {
            target = getArgument();
            skipSpacesOnLine();
            if (m_position < m_inputLength && m_input[m_position] == '{') {
                str = getArgument();

                // hack for C++ to support links like \l{QString::}{count()}
                if (target.endsWith("::"))
                    target += str;
            } else {
                str = target;
            }
        } else {
            target = getArgument();
            str = cleanLink(target);
            if (target == QLatin1String("and") || target == QLatin1String("."))
                skipMe = true;
        }

        if (!skipMe) {
            Text also;
            also << Atom(Atom::Link, target) << Atom(Atom::FormattingLeft, ATOM_FORMATTING_LINK)
                 << str << Atom(Atom::FormattingRight, ATOM_FORMATTING_LINK);
            m_private->addAlso(also);
        }

        skipSpacesOnLine();
        if (m_position < m_inputLength && m_input[m_position] == ',') {
            m_position++;
            skipSpacesOrOneEndl();
        } else if (m_position >= m_inputLength || m_input[m_position] != '\n') {
            location().warning(QStringLiteral("Missing comma in '\\%1'").arg(cmdName(CMD_SA)));
        }
    }
}

void DocParser::append(Atom::AtomType type, const QString &string)
{
    Atom::AtomType lastType = m_private->m_text.lastAtom()->type();
    if ((lastType == Atom::Code)
        && m_private->m_text.lastAtom()->string().endsWith(QLatin1String("\n\n")))
        m_private->m_text.lastAtom()->chopString();
    m_private->m_text << Atom(type, string);
}

void DocParser::append(const QString &string)
{
    Atom::AtomType lastType = m_private->m_text.lastAtom()->type();
    if ((lastType == Atom::Code)
        && m_private->m_text.lastAtom()->string().endsWith(QLatin1String("\n\n")))
        m_private->m_text.lastAtom()->chopString();
    m_private->m_text << Atom(Atom::Link, string);
}

void DocParser::append(Atom::AtomType type, const QString &p1, const QString &p2)
{
    Atom::AtomType lastType = m_private->m_text.lastAtom()->type();
    if ((lastType == Atom::Code)
        && m_private->m_text.lastAtom()->string().endsWith(QLatin1String("\n\n")))
        m_private->m_text.lastAtom()->chopString();
    m_private->m_text << Atom(type, p1, p2);
}

void DocParser::append(const QString &p1, const QString &p2)
{
    Atom::AtomType lastType = m_private->m_text.lastAtom()->type();
    if ((lastType == Atom::Code)
        && m_private->m_text.lastAtom()->string().endsWith(QLatin1String("\n\n")))
        m_private->m_text.lastAtom()->chopString();
    if (p2.isEmpty())
        m_private->m_text << Atom(Atom::Link, p1);
    else
        m_private->m_text << LinkAtom(p1, p2);
}

void DocParser::appendChar(QChar ch)
{
    if (m_private->m_text.lastAtom()->type() != Atom::String)
        append(Atom::String);
    Atom *atom = m_private->m_text.lastAtom();
    if (ch == QLatin1Char(' ')) {
        if (!atom->string().endsWith(QLatin1Char(' ')))
            atom->appendChar(QLatin1Char(' '));
    } else
        atom->appendChar(ch);
}

void DocParser::appendWord(const QString &word)
{
    if (m_private->m_text.lastAtom()->type() != Atom::String) {
        append(Atom::String, word);
    } else
        m_private->m_text.lastAtom()->appendString(word);
}

void DocParser::appendToCode(const QString &markedCode)
{
    if (!isCode(m_lastAtom)) {
        append(Atom::Code);
        m_lastAtom = m_private->m_text.lastAtom();
    }
    m_lastAtom->appendString(markedCode);
}

void DocParser::appendToCode(const QString &markedCode, Atom::AtomType defaultType)
{
    if (!isCode(m_lastAtom)) {
        append(defaultType, markedCode);
        m_lastAtom = m_private->m_text.lastAtom();
    } else {
        m_lastAtom->appendString(markedCode);
    }
}

void DocParser::enterPara(Atom::AtomType leftType, Atom::AtomType rightType, const QString &string)
{
    if (m_paragraphState == OutsideParagraph) {

        if ((m_private->m_text.lastAtom()->type() != Atom::ListItemLeft)
            && (m_private->m_text.lastAtom()->type() != Atom::DivLeft)) {
            leaveValueList();
        }

        append(leftType, string);
        m_indexStartedParagraph = false;
        m_pendingParagraphLeftType = leftType;
        m_pendingParagraphRightType = rightType;
        m_pendingParagraphString = string;
        if (leftType == Atom::SectionHeadingLeft) {
            m_paragraphState = InSingleLineParagraph;
        } else {
            m_paragraphState = InMultiLineParagraph;
        }
        skipSpacesOrOneEndl();
    }
}

void DocParser::leavePara()
{
    if (m_paragraphState != OutsideParagraph) {
        if (!m_pendingFormats.isEmpty()) {
            location().warning(QStringLiteral("Missing '}'"));
            m_pendingFormats.clear();
        }

        if (m_private->m_text.lastAtom()->type() == m_pendingParagraphLeftType) {
            m_private->m_text.stripLastAtom();
        } else {
            if (m_private->m_text.lastAtom()->type() == Atom::String
                && m_private->m_text.lastAtom()->string().endsWith(QLatin1Char(' '))) {
                m_private->m_text.lastAtom()->chopString();
            }
            append(m_pendingParagraphRightType, m_pendingParagraphString);
        }
        m_paragraphState = OutsideParagraph;
        m_indexStartedParagraph = false;
        m_pendingParagraphRightType = Atom::Nop;
        m_pendingParagraphString.clear();
    }
}

void DocParser::leaveValue()
{
    leavePara();
    if (m_openedLists.isEmpty()) {
        m_openedLists.push(OpenedList(OpenedList::Value));
        append(Atom::ListLeft, ATOM_LIST_VALUE);
    } else {
        if (m_private->m_text.lastAtom()->type() == Atom::Nop)
            m_private->m_text.stripLastAtom();
        append(Atom::ListItemRight, ATOM_LIST_VALUE);
    }
}

void DocParser::leaveValueList()
{
    leavePara();
    if (!m_openedLists.isEmpty() && (m_openedLists.top().style() == OpenedList::Value)) {
        if (m_private->m_text.lastAtom()->type() == Atom::Nop)
            m_private->m_text.stripLastAtom();
        append(Atom::ListItemRight, ATOM_LIST_VALUE);
        append(Atom::ListRight, ATOM_LIST_VALUE);
        m_openedLists.pop();
    }
}

void DocParser::leaveTableRow()
{
    if (m_inTableItem) {
        leavePara();
        append(Atom::TableItemRight);
        m_inTableItem = false;
    }
    if (m_inTableHeader) {
        append(Atom::TableHeaderRight);
        m_inTableHeader = false;
    }
    if (m_inTableRow) {
        append(Atom::TableRowRight);
        m_inTableRow = false;
    }
}

CodeMarker *DocParser::quoteFromFile()
{
    return Doc::quoteFromFile(location(), m_quoter, getArgument());
}

/*!
  Expands a macro in-place in input.

  Expects the current \e pos in the input to point to a backslash, and the macro to have a
  default definition. Format-specific macros are currently not expanded.

  \note In addition to macros, a valid use for a backslash in an argument include
  escaping non-alnum characters, and splitting a single argument across multiple
  lines by escaping newlines. Escaping is also handled here.

  Returns \c true on successful macro expansion.
 */
bool DocParser::expandMacro()
{
    Q_ASSERT(m_input[m_position].unicode() == '\\');

    QString cmdStr;
    qsizetype backslashPos = m_position++;
    while (m_position < m_input.length() && m_input[m_position].isLetterOrNumber())
        cmdStr += m_input[m_position++];

    m_endPosition = m_position;
    if (!cmdStr.isEmpty()) {
        if (s_utilities.macroHash.contains(cmdStr)) {
            const Macro &macro = s_utilities.macroHash.value(cmdStr);
            if (!macro.m_defaultDef.isEmpty()) {
                QString expanded = expandMacroToString(cmdStr, macro.m_defaultDef, macro.numParams,
                                                       macro.m_otherDefs.value("match"));
                m_input.replace(backslashPos, m_position - backslashPos, expanded);
                m_inputLength = m_input.length();
                m_position = backslashPos;
                return true;
            } else {
                location().warning(QStringLiteral("Macro '%1' does not have a default definition")
                                           .arg(cmdStr));
            }
        } else {
            location().warning(QStringLiteral("Unknown macro '%1'").arg(cmdStr));
            m_position = ++backslashPos;
        }
    } else if (m_input[m_position].isSpace()) {
        skipAllSpaces();
    } else if (m_input[m_position].unicode() == '\\') {
        // allow escaping a backslash
        m_input.remove(m_position--, 1);
        --m_inputLength;
    }
    return false;
}

void DocParser::expandMacro(const QString &name, const QString &def, int numParams)
{
    if (numParams == 0) {
        append(Atom::RawString, def);
    } else {
        QStringList args;
        QString rawString;

        for (int i = 0; i < numParams; ++i) {
            if (numParams == 1 || isLeftBraceAhead()) {
                args << getArgument();
            } else {
                location().warning(QStringLiteral("Macro '\\%1' invoked with too few"
                                                  " arguments (expected %2, got %3)")
                                           .arg(name)
                                           .arg(numParams)
                                           .arg(i));
                numParams = i;
                break;
            }
        }

        int j = 0;
        while (j < def.size()) {
            int paramNo;
            if (((paramNo = def[j].unicode()) >= 1) && (paramNo <= numParams)) {
                if (!rawString.isEmpty()) {
                    append(Atom::RawString, rawString);
                    rawString.clear();
                }
                append(Atom::String, args[paramNo - 1]);
                j += 1;
            } else {
                rawString += def[j++];
            }
        }
        if (!rawString.isEmpty())
            append(Atom::RawString, rawString);
    }
}

QString DocParser::expandMacroToString(const QString &name, const QString &def, int numParams,
                                       const QString &matchExpr)
{
    QString rawString;

    if (numParams == 0) {
        rawString = def;
    } else {
        QStringList args;
        for (int i = 0; i < numParams; ++i) {
            if (numParams == 1 || isLeftBraceAhead()) {
                args << getArgument(true);
            } else {
                location().warning(QStringLiteral("Macro '\\%1' invoked with too few"
                                                  " arguments (expected %2, got %3)")
                                           .arg(name)
                                           .arg(numParams)
                                           .arg(i));
                numParams = i;
                break;
            }
        }

        int j = 0;
        while (j < def.size()) {
            int paramNo;
            if (((paramNo = def[j].unicode()) >= 1) && (paramNo <= numParams)) {
                rawString += args[paramNo - 1];
                j += 1;
            } else {
                rawString += def[j++];
            }
        }
    }
    if (matchExpr.isEmpty())
        return rawString;

    QString result;
    QRegularExpression re(matchExpr);
    int capStart = (re.captureCount() > 0) ? 1 : 0;
    qsizetype i = 0;
    QRegularExpressionMatch match;
    while ((match = re.match(rawString, i)).hasMatch()) {
        for (int c = capStart; c <= re.captureCount(); ++c)
            result += match.captured(c);
        i = match.capturedEnd();
    }

    return result;
}

Doc::Sections DocParser::getSectioningUnit()
{
    QString name = getOptionalArgument();

    if (name == "section1") {
        return Doc::Section1;
    } else if (name == "section2") {
        return Doc::Section2;
    } else if (name == "section3") {
        return Doc::Section3;
    } else if (name == "section4") {
        return Doc::Section4;
    } else if (name.isEmpty()) {
        return Doc::NoSection;
    } else {
        location().warning(QStringLiteral("Invalid section '%1'").arg(name));
        return Doc::NoSection;
    }
}

/*!
  Gets an argument that is enclosed in braces and returns it
  without the enclosing braces. On entry, the current character
  is the left brace. On exit, the current character is the one
  that comes after the right brace.

  If \a verbatim is true, extra whitespace is retained in the
  returned string. Otherwise, extra whitespace is removed.
 */
QString DocParser::getBracedArgument(bool verbatim)
{
    QString arg;
    int delimDepth = 0;
    if (m_position < m_input.length() && m_input[m_position] == '{') {
        ++m_position;
        while (m_position < m_input.length() && delimDepth >= 0) {
            switch (m_input[m_position].unicode()) {
            case '{':
                ++delimDepth;
                arg += QLatin1Char('{');
                ++m_position;
                break;
            case '}':
                --delimDepth;
                if (delimDepth >= 0)
                    arg += QLatin1Char('}');
                ++m_position;
                break;
            case '\\':
                if (verbatim || !expandMacro())
                    arg += m_input[m_position++];
                break;
            default:
                if (m_input[m_position].isSpace() && !verbatim)
                    arg += QChar(' ');
                else
                    arg += m_input[m_position];
                ++m_position;
            }
        }
        if (delimDepth > 0)
            location().warning(QStringLiteral("Missing '}'"));
    }
    m_endPosition = m_position;
    return arg;
}

/*!
  Typically, an argument ends at the next white-space. However,
  braces can be used to group words:

  {a few words}

  Also, opening and closing parentheses have to match. Thus,

  printf("%d\n", x)

  is an argument too, although it contains spaces. Finally,
  trailing punctuation is not included in an argument, nor is 's.
*/
QString DocParser::getArgument(bool verbatim)
{
    skipSpacesOrOneEndl();

    int delimDepth = 0;
    qsizetype startPos = m_position;
    QString arg = getBracedArgument(verbatim);
    if (arg.isEmpty()) {
        while ((m_position < m_input.length())
               && ((delimDepth > 0) || ((delimDepth == 0) && !m_input[m_position].isSpace()))) {
            switch (m_input[m_position].unicode()) {
            case '(':
            case '[':
            case '{':
                ++delimDepth;
                arg += m_input[m_position];
                ++m_position;
                break;
            case ')':
            case ']':
            case '}':
                --delimDepth;
                if (m_position == startPos || delimDepth >= 0) {
                    arg += m_input[m_position];
                    ++m_position;
                }
                break;
            case '\\':
                if (verbatim || !expandMacro())
                    arg += m_input[m_position++];
                break;
            default:
                arg += m_input[m_position];
                ++m_position;
            }
        }
        m_endPosition = m_position;
        if ((arg.length() > 1) && (QString(".,:;!?").indexOf(m_input[m_position - 1]) != -1)
            && !arg.endsWith("...")) {
            arg.truncate(arg.length() - 1);
            --m_position;
        }
        if (arg.length() > 2 && m_input.mid(m_position - 2, 2) == "'s") {
            arg.truncate(arg.length() - 2);
            m_position -= 2;
        }
    }
    return arg.simplified();
}

/*!
  Gets an argument that is enclosed in brackets and returns it
  without the enclosing brackets. On entry, the current character
  is the left bracket. On exit, the current character is the one
  that comes after the right bracket.
 */
QString DocParser::getBracketedArgument()
{
    QString arg;
    int delimDepth = 0;
    skipSpacesOrOneEndl();
    if (m_position < m_input.length() && m_input[m_position] == '[') {
        ++m_position;
        while (m_position < m_input.length() && delimDepth >= 0) {
            switch (m_input[m_position].unicode()) {
            case '[':
                ++delimDepth;
                arg += QLatin1Char('[');
                ++m_position;
                break;
            case ']':
                --delimDepth;
                if (delimDepth >= 0)
                    arg += QLatin1Char(']');
                ++m_position;
                break;
            case '\\':
                arg += m_input[m_position];
                ++m_position;
                break;
            default:
                arg += m_input[m_position];
                ++m_position;
            }
        }
        if (delimDepth > 0)
            location().warning(QStringLiteral("Missing ']'"));
    }
    return arg;
}

QString DocParser::getOptionalArgument()
{
    skipSpacesOrOneEndl();
    if (m_position + 1 < m_input.length() && m_input[m_position] == '\\'
        && m_input[m_position + 1].isLetterOrNumber()) {
        return QString();
    } else {
        return getArgument();
    }
}

QString DocParser::getRestOfLine()
{
    QString t;

    skipSpacesOnLine();

    bool trailingSlash = false;

    do {
        qsizetype begin = m_position;

        while (m_position < m_input.size() && m_input[m_position] != '\n') {
            if (m_input[m_position] == '\\' && !trailingSlash) {
                trailingSlash = true;
                ++m_position;
                while ((m_position < m_input.size()) && m_input[m_position].isSpace()
                       && (m_input[m_position] != '\n'))
                    ++m_position;
            } else {
                trailingSlash = false;
                ++m_position;
            }
        }

        if (!t.isEmpty())
            t += QLatin1Char(' ');
        t += m_input.mid(begin, m_position - begin).simplified();

        if (trailingSlash) {
            t.chop(1);
            t = t.simplified();
        }
        if (m_position < m_input.size())
            ++m_position;
    } while (m_position < m_input.size() && trailingSlash);

    return t;
}

/*!
  The metacommand argument is normally the remaining text to
  the right of the metacommand itself. The extra blanks are
  stripped and the argument string is returned.
 */
QString DocParser::getMetaCommandArgument(const QString &cmdStr)
{
    skipSpacesOnLine();

    qsizetype begin = m_position;
    int parenDepth = 0;

    while (m_position < m_input.size() && (m_input[m_position] != '\n' || parenDepth > 0)) {
        if (m_input.at(m_position) == '(')
            ++parenDepth;
        else if (m_input.at(m_position) == ')')
            --parenDepth;
        else if (m_input.at(m_position) == '\\' && expandMacro())
            continue;
        ++m_position;
    }
    if (m_position == m_input.size() && parenDepth > 0) {
        m_position = begin;
        location().warning(QStringLiteral("Unbalanced parentheses in '%1'").arg(cmdStr));
    }

    QString t = m_input.mid(begin, m_position - begin).simplified();
    skipSpacesOnLine();
    return t;
}

QString DocParser::getUntilEnd(int cmd)
{
    int endCmd = endCmdFor(cmd);
    QRegularExpression rx("\\\\" + cmdName(endCmd) + "\\b");
    QString t;
    auto match = rx.match(m_input, m_position);

    if (!match.hasMatch()) {
        location().warning(QStringLiteral("Missing '\\%1'").arg(cmdName(endCmd)));
        m_position = m_input.length();
    } else {
        qsizetype end = match.capturedStart();
        t = m_input.mid(m_position, end - m_position);
        m_position = match.capturedEnd();
    }
    return t;
}

QString DocParser::getCode(int cmd, CodeMarker *marker, const QString &argStr)
{
    QString code = untabifyEtc(getUntilEnd(cmd));

    if (!argStr.isEmpty()) {
        QStringList args = argStr.split(" ", Qt::SkipEmptyParts);
        qsizetype paramNo, j = 0;
        while (j < code.size()) {
            if (code[j] == '\\' && j < code.size() - 1 && (paramNo = code[j + 1].digitValue()) >= 1
                && paramNo <= args.size()) {
                QString p = args[paramNo - 1];
                code.replace(j, 2, p);
                j += qMin(1, p.size());
            } else {
                ++j;
            }
        }
    }

    int indent = indentLevel(code);
    code = dedent(indent, code);
    if (marker == nullptr)
        marker = CodeMarker::markerForCode(code);
    return marker->markedUpCode(code, nullptr, location());
}

bool DocParser::isBlankLine()
{
    qsizetype i = m_position;

    while (i < m_inputLength && m_input[i].isSpace()) {
        if (m_input[i] == '\n')
            return true;
        ++i;
    }
    return false;
}

bool DocParser::isLeftBraceAhead()
{
    int numEndl = 0;
    qsizetype i = m_position;

    while (i < m_inputLength && m_input[i].isSpace() && numEndl < 2) {
        // ### bug with '\\'
        if (m_input[i] == '\n')
            numEndl++;
        ++i;
    }
    return numEndl < 2 && i < m_inputLength && m_input[i] == '{';
}

bool DocParser::isLeftBracketAhead()
{
    int numEndl = 0;
    qsizetype i = m_position;

    while (i < m_inputLength && m_input[i].isSpace() && numEndl < 2) {
        // ### bug with '\\'
        if (m_input[i] == '\n')
            numEndl++;
        ++i;
    }
    return numEndl < 2 && i < m_inputLength && m_input[i] == '[';
}

/*!
  Skips to the next non-space character or EOL.
 */
void DocParser::skipSpacesOnLine()
{
    while ((m_position < m_input.length()) && m_input[m_position].isSpace()
           && (m_input[m_position].unicode() != '\n'))
        ++m_position;
}

/*!
  Skips spaces and one EOL.
 */
void DocParser::skipSpacesOrOneEndl()
{
    qsizetype firstEndl = -1;
    while (m_position < m_input.length() && m_input[m_position].isSpace()) {
        QChar ch = m_input[m_position];
        if (ch == '\n') {
            if (firstEndl == -1) {
                firstEndl = m_position;
            } else {
                m_position = firstEndl;
                break;
            }
        }
        ++m_position;
    }
}

void DocParser::skipAllSpaces()
{
    while (m_position < m_inputLength && m_input[m_position].isSpace())
        ++m_position;
}

void DocParser::skipToNextPreprocessorCommand()
{
    QRegularExpression rx("\\\\(?:" + cmdName(CMD_IF) + QLatin1Char('|') + cmdName(CMD_ELSE)
                          + QLatin1Char('|') + cmdName(CMD_ENDIF) + ")\\b");
    auto match = rx.match(m_input, m_position + 1); // ### + 1 necessary?

    if (!match.hasMatch())
        m_position = m_input.length();
    else
        m_position = match.capturedStart();
}

int DocParser::endCmdFor(int cmd)
{
    switch (cmd) {
    case CMD_BADCODE:
        return CMD_ENDCODE;
    case CMD_CODE:
        return CMD_ENDCODE;
    case CMD_DIV:
        return CMD_ENDDIV;
    case CMD_QML:
        return CMD_ENDQML;
    case CMD_QMLTEXT:
        return CMD_ENDQMLTEXT;
    case CMD_JS:
        return CMD_ENDJS;
    case CMD_FOOTNOTE:
        return CMD_ENDFOOTNOTE;
    case CMD_LEGALESE:
        return CMD_ENDLEGALESE;
    case CMD_LINK:
        return CMD_ENDLINK;
    case CMD_LIST:
        return CMD_ENDLIST;
    case CMD_NEWCODE:
        return CMD_ENDCODE;
    case CMD_OLDCODE:
        return CMD_NEWCODE;
    case CMD_OMIT:
        return CMD_ENDOMIT;
    case CMD_QUOTATION:
        return CMD_ENDQUOTATION;
    case CMD_RAW:
        return CMD_ENDRAW;
    case CMD_SECTION1:
        return CMD_ENDSECTION1;
    case CMD_SECTION2:
        return CMD_ENDSECTION2;
    case CMD_SECTION3:
        return CMD_ENDSECTION3;
    case CMD_SECTION4:
        return CMD_ENDSECTION4;
    case CMD_SIDEBAR:
        return CMD_ENDSIDEBAR;
    case CMD_TABLE:
        return CMD_ENDTABLE;
    default:
        return cmd;
    }
}

QString DocParser::cmdName(int cmd)
{
    return *cmds[cmd].alias;
}

QString DocParser::endCmdName(int cmd)
{
    return cmdName(endCmdFor(cmd));
}

QString DocParser::untabifyEtc(const QString &str)
{
    QString result;
    result.reserve(str.length());
    int column = 0;

    for (const auto &character : str) {
        if (character == QLatin1Char('\r'))
            continue;
        if (character == QLatin1Char('\t')) {
            result += &"        "[column % s_tabSize];
            column = ((column / s_tabSize) + 1) * s_tabSize;
            continue;
        }
        if (character == QLatin1Char('\n')) {
            while (result.endsWith(QLatin1Char(' ')))
                result.chop(1);
            result += character;
            column = 0;
            continue;
        }
        result += character;
        ++column;
    }

    while (result.endsWith("\n\n"))
        result.truncate(result.length() - 1);
    while (result.startsWith(QLatin1Char('\n')))
        result = result.mid(1);

    return result;
}

int DocParser::indentLevel(const QString &str)
{
    int minIndent = INT_MAX;
    int column = 0;

    for (const auto &character : str) {
        if (character == '\n') {
            column = 0;
        } else {
            if (character != ' ' && column < minIndent)
                minIndent = column;
            ++column;
        }
    }
    return minIndent;
}

QString DocParser::dedent(int level, const QString &str)
{
    if (level == 0)
        return str;

    QString result;
    int column = 0;

    for (const auto &character : str) {
        if (character == QLatin1Char('\n')) {
            result += '\n';
            column = 0;
        } else {
            if (column >= level)
                result += character;
            ++column;
        }
    }
    return result;
}

QString DocParser::slashed(const QString &str)
{
    QString result = str;
    result.replace(QLatin1Char('/'), "\\/");
    return QLatin1Char('/') + result + QLatin1Char('/');
}

/*!
 Returns \c true if \a atom represents a code snippet.
 */
bool DocParser::isCode(const Atom *atom)
{
    Atom::AtomType type = atom->type();
    return (type == Atom::Code || type == Atom::Qml || type == Atom::JavaScript);
}

/*!
 Returns \c true if \a atom represents quoting information.
 */
bool DocParser::isQuote(const Atom *atom)
{
    Atom::AtomType type = atom->type();
    return (type == Atom::CodeQuoteArgument || type == Atom::CodeQuoteCommand
            || type == Atom::SnippetCommand || type == Atom::SnippetIdentifier
            || type == Atom::SnippetLocation);
}

QT_END_NAMESPACE
