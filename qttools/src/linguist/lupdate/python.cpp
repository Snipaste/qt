/****************************************************************************
**
** Copyright (C) 2002-2007 Detlev Offenbach <detlev@die-offenbachs.de>
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Linguist of the Qt Toolkit.
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

#include <translator.h>
#include "lupdate.h"

#include <QtCore/qhash.h>
#include <QtCore/qstring.h>
#include <QtCore/qtextstream.h>
#include <QtCore/qstack.h>

#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstring>

QT_BEGIN_NAMESPACE

static const char MagicComment[] = "TRANSLATOR ";

/*
  The first part of this source file is the Python tokenizer.  We skip
  most of Python; the only tokens that interest us are defined here.
*/

enum Token { Tok_Eof, Tok_class, Tok_return, Tok_tr,
             Tok_trUtf8, Tok_translate, Tok_Ident,
             Tok_Comment, Tok_Dot, Tok_String,
             Tok_LeftParen, Tok_RightParen,
             Tok_Comma, Tok_None, Tok_Integer};

/*
  The tokenizer maintains the following global variables. The names
  should be self-explanatory.
*/
static QString yyFileName;
static int yyCh;
static QByteArray yyIdent;
static char yyComment[65536];
static size_t yyCommentLen;
static char yyString[65536];
static size_t yyStringLen;
static int yyParenDepth;
static int yyLineNo;
static int yyCurLineNo;

static QByteArray extraComment;
static QByteArray id;

QHash<QByteArray, Token> tokens = {
    {"None", Tok_None},
    {"class", Tok_class},
    {"return", Tok_return},
    {"__tr", Tok_tr}, // Legacy?
    {"__trUtf8", Tok_trUtf8}
};

// the file to read from (if reading from a file)
static FILE *yyInFile;

// the string to read from and current position in the string (otherwise)
static int yyInPos;
static int buf;

static int (*getChar)();
static int (*peekChar)();

static int yyIndentationSize;
static int yyContinuousSpaceCount;
static bool yyCountingIndentation;

// (Context, indentation level) pair.
using ContextPair = QPair<QByteArray, int>;
// Stack of (Context, indentation level) pairs.
using ContextStack = QStack<ContextPair>;
static ContextStack yyContextStack;

static int yyContextPops;

static int getCharFromFile()
{
    int c;

    if (buf < 0) {
        c = getc(yyInFile);
    } else {
        c = buf;
        buf = -1;
    }
    if (c == '\n') {
        yyCurLineNo++;
        yyCountingIndentation = true;
        yyContinuousSpaceCount = 0;
    } else if (yyCountingIndentation && (c == 32 || c == 9)) {
        yyContinuousSpaceCount++;
    } else {
        if (yyIndentationSize == 1 && yyContinuousSpaceCount > yyIndentationSize)
            yyIndentationSize = yyContinuousSpaceCount;
        if (yyCountingIndentation && yyContextStack.count() > 1) {
            ContextPair& top = yyContextStack.top();
            if (top.second == 0 && yyContinuousSpaceCount > 0) {
                top.second = yyContinuousSpaceCount;
                yyContinuousSpaceCount = 0;
            } else if (yyContinuousSpaceCount < top.second) {
                yyContextPops = (top.second - yyContinuousSpaceCount) / yyIndentationSize;
            }
        }
        yyCountingIndentation = false;
    }
    return c;
}

static int peekCharFromFile()
{
    int c = getc(yyInFile);
    buf = c;
    return c;
}

static void startTokenizer(const QString &fileName, int (*getCharFunc)(),
                           int (*peekCharFunc)())
{
    yyInPos = 0;
    buf = -1;
    getChar = getCharFunc;
    peekChar = peekCharFunc;

    yyFileName = fileName;
    yyCh = getChar();
    yyParenDepth = 0;
    yyCurLineNo = 1;

    yyIndentationSize = 1;
    yyContinuousSpaceCount = 0;
    yyCountingIndentation = false;
    yyContextStack.clear();
    yyContextPops = 0;
}

static Token parseString()
{
    static const char tab[] = "abfnrtv";
    static const char backTab[] = "\a\b\f\n\r\t\v";

    int quoteChar = yyCh;
    bool tripleQuote = false;
    bool singleQuote = true;
    bool in = false;

    yyCh = getChar();

    while (yyCh != EOF) {
        if (singleQuote && (yyCh == '\n' || (in && yyCh == quoteChar)))
            break;

        if (yyCh == quoteChar) {
            if (peekChar() == quoteChar) {
                yyCh = getChar();
                if (!tripleQuote) {
                    tripleQuote = true;
                    singleQuote = false;
                    in = true;
                    yyCh = getChar();
                } else {
                    yyCh = getChar();
                    if (yyCh == quoteChar) {
                        tripleQuote = false;
                        break;
                    }
                }
            } else if (tripleQuote) {
                if (yyStringLen < sizeof(yyString) - 1)
                    yyString[yyStringLen++] = char(yyCh);
                yyCh = getChar();
                continue;
            } else {
                break;
            }
        } else {
            in = true;
        }

        if (yyCh == '\\') {
            yyCh = getChar();

            if (yyCh == 'x') {
                QByteArray hex = "0";

                yyCh = getChar();
                while (std::isxdigit(yyCh)) {
                    hex += char(yyCh);
                    yyCh = getChar();
                }
                uint n;
#ifdef Q_CC_MSVC
                sscanf_s(hex, "%x", &n);
#else
                std::sscanf(hex, "%x", &n);
#endif
                if (yyStringLen < sizeof(yyString) - 1)
                    yyString[yyStringLen++] = char(n);
            } else if (yyCh >= '0' && yyCh < '8') {
                QByteArray oct;
                int n = 0;

                do {
                    oct += char(yyCh);
                    ++n;
                    yyCh = getChar();
                } while (yyCh >= '0' && yyCh < '8' && n < 3);
#ifdef Q_CC_MSVC
                sscanf_s(oct, "%o", &n);
#else
                std::sscanf(oct, "%o", &n);
#endif
                if (yyStringLen < sizeof(yyString) - 1)
                    yyString[yyStringLen++] = char(n);
            } else {
                const char *p = std::strchr(tab, yyCh);
                if (yyStringLen < sizeof(yyString) - 1) {
                    yyString[yyStringLen++] = (p == nullptr)
                            ? char(yyCh) : backTab[p - tab];
                }
                yyCh = getChar();
            }
        } else {
            char *yStart = yyString + yyStringLen;
            char *yp = yStart;
            while (yyCh != EOF && (tripleQuote || yyCh != '\n') && yyCh != quoteChar
                   && yyCh != '\\') {
                *yp++ = char(yyCh);
                yyCh = getChar();
            }
            yyStringLen += yp - yStart;
        }
    }
    yyString[yyStringLen] = '\0';

    if (yyCh != quoteChar) {
        printf("%c\n", yyCh);

        qWarning("%s:%d: Unterminated string",
                 qPrintable(yyFileName), yyLineNo);
    }

    if (yyCh == EOF)
        return Tok_Eof;
    yyCh = getChar();
    return Tok_String;
}

static QByteArray readLine()
{
    QByteArray result;
    while (true) {
        yyCh = getChar();
        if (yyCh == EOF || yyCh == '\n')
            break;
        result.append(char(yyCh));
    }
    return result;
}

static Token getToken()
{
    yyIdent.clear();
    yyCommentLen = 0;
    yyStringLen = 0;
    while (yyCh != EOF) {
        yyLineNo = yyCurLineNo;

        if (std::isalpha(yyCh) || yyCh == '_') {
            do {
                yyIdent.append(char(yyCh));
                yyCh = getChar();
            } while (std::isalnum(yyCh) || yyCh == '_');

            return tokens.value(yyIdent, Tok_Ident);
        }
        switch (yyCh) {
        case '#':
            switch (getChar()) {
            case ':':
                extraComment = readLine().trimmed();
                break;
            case '=':
                id = readLine().trimmed();
                break;
            case EOF:
            case '\n':
                break;
            default:
                do {
                    yyCh = getChar();
                } while (yyCh != EOF && yyCh != '\n');
                break;
            }
            break;
        case '"':
        case '\'':
            return parseString();
        case '(':
            yyParenDepth++;
            yyCh = getChar();
            return Tok_LeftParen;
        case ')':
            yyParenDepth--;
            yyCh = getChar();
            return Tok_RightParen;
        case ',':
            yyCh = getChar();
            return Tok_Comma;
        case '.':
            yyCh = getChar();
            return Tok_Dot;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':  {
            QByteArray ba;
            ba += char(yyCh);
            yyCh = getChar();
            const bool hex = yyCh == 'x';
            if (hex) {
                ba += char(yyCh);
                yyCh = getChar();
            }
            while ((hex ? std::isxdigit(yyCh) : std::isdigit(yyCh))) {
                ba += char(yyCh);
                yyCh = getChar();
            }
            bool ok;
            auto v = ba.toLongLong(&ok);
            Q_UNUSED(v);
            if (ok)
                return Tok_Integer;
            break;
        }
        default:
            yyCh = getChar();
        }
    }
    return Tok_Eof;
}

/*
  The second part of this source file is the parser. It accomplishes
  a very easy task: It finds all strings inside a tr() or translate()
  call, and possibly finds out the context of the call. It supports
  three cases:
  (1) the context is specified, as in FunnyDialog.tr("Hello") or
     translate("FunnyDialog", "Hello");
  (2) the call appears within an inlined function;
  (3) the call appears within a function defined outside the class definition.
*/

static Token yyTok;

static bool match(Token t)
{
    const bool matches = (yyTok == t);
    if (matches)
        yyTok = getToken();
    return matches;
}

static bool matchString(QByteArray *s)
{
    const bool matches = (yyTok == Tok_String);
    s->clear();
    while (yyTok == Tok_String) {
        *s += yyString;
        yyTok = getToken();
    }
    return matches;
}

static bool matchEncoding(bool *utf8)
{
    // Remove any leading module paths.
    if (yyTok == Tok_Ident && std::strcmp(yyIdent, "PySide6") == 0) {
        yyTok = getToken();

        if (yyTok != Tok_Dot)
            return false;

        yyTok = getToken();
    }

    if (yyTok == Tok_Ident && (std::strcmp(yyIdent, "QtGui") == 0
                               || std::strcmp(yyIdent, "QtCore") == 0)) {
        yyTok = getToken();

        if (yyTok != Tok_Dot)
            return false;

        yyTok = getToken();
    }

    if (yyTok == Tok_Ident) {
        if (std::strcmp(yyIdent, "QApplication") == 0
            || std::strcmp(yyIdent, "QGuiApplication") == 0
            || std::strcmp(yyIdent, "QCoreApplication") == 0) {
            yyTok = getToken();

            if (yyTok == Tok_Dot)
                yyTok = getToken();
        }

        *utf8 = QByteArray(yyIdent).endsWith("UTF8");
        yyTok = getToken();
        return true;
    }
    return false;
}

static bool matchStringOrNone(QByteArray *s)
{
    bool matches = matchString(s);

    if (!matches)
        matches = match(Tok_None);

    return matches;
}

/*
 * match any expression that can return a number, which can be
 * 1. Literal number (e.g. '11')
 * 2. simple identifier (e.g. 'm_count')
 * 3. simple function call (e.g. 'size()')
 * 4. function call on an object (e.g. 'list.size()')
 *
 * Other cases:
 * size(2,4)
 * list().size()
 * list(a,b).size(2,4)
 * etc...
 */
static bool matchExpression()
{
    if (match(Tok_Integer))
        return true;

    int parenlevel = 0;
    while (match(Tok_Ident) || parenlevel > 0) {
        if (yyTok == Tok_RightParen) {
            if (parenlevel == 0)
                break;
            --parenlevel;
            yyTok = getToken();
        } else if (yyTok == Tok_LeftParen) {
            yyTok = getToken();
            if (yyTok == Tok_RightParen) {
                yyTok = getToken();
            } else {
                ++parenlevel;
            }
        } else if (yyTok == Tok_Ident) {
            continue;
        } else if (parenlevel == 0) {
            return false;
        }
    }
    return true;
}

static bool parseTranslate(QByteArray *text, QByteArray *context, QByteArray *comment,
                           bool *utf8, bool *plural)
{
    text->clear();
    context->clear();
    comment->clear();
    *utf8 = false;
    *plural = false;

    yyTok = getToken();
    if (!match(Tok_LeftParen) || !matchString(context) || !match(Tok_Comma)
        || !matchString(text)) {
        return false;
    }

    if (match(Tok_RightParen))
        return true;

    // look for comment
    if (!match(Tok_Comma) || !matchStringOrNone(comment))
        return false;

    if (match(Tok_RightParen))
        return true;

    // look for encoding
    if (!match(Tok_Comma))
        return false;

    if (matchEncoding(utf8)) {
        if (!match(Tok_RightParen)) {
            // look for the plural quantifier,
            // this can be a number, an identifier or a function call,
            // so for simplicity we mark it as plural if we know we have a comma instead of an
            // right parentheses.
            *plural = match(Tok_Comma);
        }
        return true;
    }

    // This can be a QTranslator::translate("context", "source", "comment", n) plural translation
    if (!matchExpression() || !match(Tok_RightParen))
        return false;
    *plural = true;
    return true;
}

static inline void setMessageParameters(TranslatorMessage *message)
{
    if (!extraComment.isEmpty()) {
        message->setExtraComment(QString::fromUtf8(extraComment));
        extraComment.clear();
    }
    if (!id.isEmpty()) {
        message->setId(QString::fromUtf8(id));
        id.clear();
    }
}

static void parse(Translator &tor, ConversionData &cd,
                  const QByteArray &initialContext = {},
                  const QByteArray &defaultContext = {})
{
    QByteArray context;
    QByteArray text;
    QByteArray comment;
    QByteArray prefix;
    bool utf8 = false;

    yyContextStack.push({initialContext, 0});

    yyTok = getToken();
    while (yyTok != Tok_Eof) {

        if (yyContextPops > 0) {
            for ( int i = 0; i < yyContextPops; i++)
                yyContextStack.pop();
            yyContextPops = 0;
        }

        switch (yyTok) {
            case Tok_class:
                yyTok = getToken();
                yyContextStack.push({yyIdent, 0});
                yyContinuousSpaceCount = 0;
                yyTok = getToken();
                break;
            case Tok_tr:
            case Tok_trUtf8:
                utf8 = true;
                yyTok = getToken();
                if (match(Tok_LeftParen) && matchString(&text)) {
                    comment.clear();
                    bool plural = false;

                    if (match(Tok_RightParen)) {
                        // There is no comment or plural arguments.
                    } else if (match(Tok_Comma) && matchStringOrNone(&comment)) {
                        // There is a comment argument.
                        if (match(Tok_RightParen)) {
                            // There is no plural argument.
                        } else if (match(Tok_Comma)) {
                            // There is a plural argument.
                            plural = true;
                        }
                    }

                    if (prefix.isEmpty())
                        context = defaultContext;
                    else if (prefix == "self")
                        context = yyContextStack.top().first;
                    else
                        context = prefix;

                    prefix.clear();

                    if (!text.isEmpty()) {
                        TranslatorMessage message(QString::fromUtf8(context),
                                                  QString::fromUtf8(text),
                                                  QString::fromUtf8(comment),
                                                  {}, yyFileName, yyLineNo,
                                                  {}, TranslatorMessage::Unfinished, plural);
                        setMessageParameters(&message);
                        tor.extend(message, cd);
                    }
                }
                break;
            case Tok_translate: {
                bool plural{};
                if (parseTranslate(&text, &context, &comment, &utf8, &plural)
                    && !text.isEmpty()) {
                        TranslatorMessage message(QString::fromUtf8(context),
                                                  QString::fromUtf8(text),
                                                  QString::fromUtf8(comment),
                                                  {}, yyFileName, yyLineNo,
                                                  {}, TranslatorMessage::Unfinished, plural);
                        setMessageParameters(&message);
                        tor.extend(message, cd);
                    }
                }
                break;
            case Tok_Ident:
                if (!prefix.isEmpty())
                    prefix += '.';
                prefix += yyIdent;
                yyTok = getToken();
                if (yyTok != Tok_Dot)
                    prefix.clear();
                break;
            case Tok_Comment:
                comment = yyComment;
                comment = comment.simplified();
                if (comment.left(sizeof(MagicComment) - 1) == MagicComment) {
                    comment.remove(0, sizeof(MagicComment) - 1);
                    int k = comment.indexOf(' ');
                    if (k == -1) {
                        context = comment;
                    } else {
                        context = comment.left(k);
                        comment.remove( 0, k + 1);
                        TranslatorMessage message(QString::fromUtf8(context),
                                                  {}, QString::fromUtf8(comment), {},
                                                  yyFileName, yyLineNo, {});
                        tor.extend(message, cd);
                    }
                }
                yyTok = getToken();
                break;
            default:
                yyTok = getToken();
        }
    }

    if (yyParenDepth != 0) {
        qWarning("%s: Unbalanced parentheses in Python code",
                 qPrintable(yyFileName));
    }
}

bool loadPython(Translator &translator, const QString &fileName, ConversionData &cd)
{
    // Match the function aliases to our tokens
    static bool firstTime = true;
    if (firstTime) {
        firstTime = false;
        const auto &nameMap  = trFunctionAliasManager.nameToTrFunctionMap();
        for (auto it = nameMap.cbegin(), end = nameMap.cend(); it != end; ++it) {
            switch (it.value()) {
            case TrFunctionAliasManager::Function_tr:
            case TrFunctionAliasManager::Function_QT_TR_NOOP:
                tokens.insert(it.key().toUtf8(), Tok_tr);
                break;
            case TrFunctionAliasManager::Function_trUtf8:
                tokens.insert(it.key().toUtf8(), Tok_trUtf8);
                break;
            case TrFunctionAliasManager::Function_translate:
            case TrFunctionAliasManager::Function_QT_TRANSLATE_NOOP:
            // QTranslator::findMessage() has the same parameters as QApplication::translate().
            case TrFunctionAliasManager::Function_findMessage:
                tokens.insert(it.key().toUtf8(), Tok_translate);
                break;
            default:
                break;
            }
        }
    }

#ifdef Q_CC_MSVC
    const auto *fileNameC = reinterpret_cast<const wchar_t *>(fileName.utf16());
    const bool ok = _wfopen_s(&yyInFile, fileNameC, L"r") == 0;
#else
    const QByteArray fileNameC = QFile::encodeName(fileName);
    yyInFile = std::fopen( fileNameC.constData(), "r");
    const bool ok = yyInFile != nullptr;
#endif
    if (!ok) {
        cd.appendError(QStringLiteral("Cannot open %1").arg(fileName));
        return false;
    }

    startTokenizer(fileName, getCharFromFile, peekCharFromFile);
    parse(translator, cd);
    std::fclose(yyInFile);
    return true;
}

QT_END_NAMESPACE
