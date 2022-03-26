/****************************************************************************
**
** Copyright (C) 2017 Witekio.
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCoap module.
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

#include "coaphandler.h"

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QUrl>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("Qt CoAP test application");
    parser.addHelpOption();
    parser.addPositionalArgument("url",
                                 "The targeted coap resource URL, e.g. 'coap://10.10.10.10/myresource'");
    parser.addOption({"get", "Use GET method for the request. This is the default method"});
    parser.addOption({"post", "Use POST method for the request"});
    parser.addOption({"put", "Use PUT method for the request"});
    parser.addOption({"delete", "Use DELETE method for the request"});
    parser.addOption({"observe", "Observe a resource"});
    parser.addOption({"discover", "Discover available resources"});

    parser.process(a);

    if (parser.positionalArguments().isEmpty()) {
        qWarning("Please provide an url for the request");
        parser.showHelp(1);
    }

    QUrl url = parser.positionalArguments().first();
    if (!url.isValid()) {
        qWarning("The url provided is not valid.");
        parser.showHelp(1);
    }

    CoapHandler handler;
    if (parser.isSet("get"))        handler.runGet(url);
    if (parser.isSet("post"))       handler.runPost(url);
    if (parser.isSet("put"))        handler.runPut(url);
    if (parser.isSet("delete"))     handler.runDelete(url);
    if (parser.isSet("observe"))    handler.runObserve(url);
    if (parser.isSet("discover"))   handler.runDiscover(url);

    return a.exec();
}
