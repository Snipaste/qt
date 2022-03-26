/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt OPC UA module.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QOpcUaClient>

QT_BEGIN_NAMESPACE

class QOpcUaProvider;
class OpcUaModel;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(const QString &initialUrl, QWidget *parent = nullptr);
    ~MainWindow();
    Q_INVOKABLE void log(const QString &text, const QString &context, const QColor &color);
    void log(const QString &text, const QColor &color = Qt::black);

private slots:
    void connectToServer();
    void findServers();
    void findServersComplete(const QList<QOpcUaApplicationDescription> &servers, QOpcUa::UaStatusCode statusCode);
    void getEndpoints();
    void getEndpointsComplete(const QList<QOpcUaEndpointDescription> &endpoints, QOpcUa::UaStatusCode statusCode);
    void clientConnected();
    void clientDisconnected();
    void namespacesArrayUpdated(const QStringList &namespaceArray);
    void clientError(QOpcUaClient::ClientError);
    void clientState(QOpcUaClient::ClientState);
    void showErrorDialog(QOpcUaErrorState *errorState);
    void openCustomContextMenu(const QPoint &point);
    void toggleMonitoring();

private:
    void createClient();
    void updateUiState();
    void setupPkiConfiguration();
    bool createPkiFolders();
    bool createPkiPath(const QString &path);

private:
    Ui::MainWindow *ui;
    OpcUaModel *mOpcUaModel;
    QOpcUaProvider *mOpcUaProvider;
    QOpcUaClient *mOpcUaClient = nullptr;
    QList<QOpcUaEndpointDescription> mEndpointList;
    bool mClientConnected = false;
    QOpcUaApplicationIdentity m_identity;
    QOpcUaPkiConfiguration m_pkiConfig;
    QOpcUaEndpointDescription m_endpoint; // current endpoint used to connect
    QMenu *mContextMenu;
    QAction *mContextMenuAction;
};

QT_END_NAMESPACE

#endif // MAINWINDOW_H
