/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the QtCoap module.
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

#include "mainwindow.h"
#include "optiondialog.h"
#include "ui_mainwindow.h"

#include <QCoapResourceDiscoveryReply>
#include <QCoapReply>
#include <QDateTime>
#include <QFileDialog>
#include <QHostInfo>
#include <QMessageBox>
#include <QMetaEnum>
#include <QNetworkInterface>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    m_client = new QCoapClient(QtCoap::SecurityMode::NoSecurity, this);
    connect(m_client, &QCoapClient::finished, this, &MainWindow::onFinished);
    connect(m_client, &QCoapClient::error, this, &MainWindow::onError);

    ui->setupUi(this);

    ui->methodComboBox->addItem("Get", QVariant::fromValue(QtCoap::Method::Get));
    ui->methodComboBox->addItem("Put", QVariant::fromValue(QtCoap::Method::Put));
    ui->methodComboBox->addItem("Post", QVariant::fromValue(QtCoap::Method::Post));
    ui->methodComboBox->addItem("Delete", QVariant::fromValue(QtCoap::Method::Delete));

    fillHostSelector();
    ui->hostComboBox->setFocus();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::fillHostSelector()
{
    const auto networkInterfaces = QNetworkInterface::allInterfaces();
    for (const auto &interface : networkInterfaces)
        for (const auto &address : interface.addressEntries())
            ui->hostComboBox->addItem(address.ip().toString());
}

void MainWindow::addMessage(const QString &message, bool isError)
{
    const QString content = "--------------- "
            + QDateTime::currentDateTime().toString()
            + " ---------------\n"
            + message + "\n\n";
    ui->textEdit->setTextColor(isError ? Qt::red : Qt::black);
    ui->textEdit->insertPlainText(content);
    ui->textEdit->ensureCursorVisible();
}

void MainWindow::onFinished(QCoapReply *reply)
{
    if (reply->errorReceived() == QtCoap::Error::Ok)
        addMessage(reply->message().payload());
}

static QString errorMessage(QtCoap::Error errorCode)
{
    const auto error = QMetaEnum::fromType<QtCoap::Error>().valueToKey(static_cast<int>(errorCode));
    return QString("Request failed with error: %1\n").arg(error);
}

void MainWindow::onError(QCoapReply *reply, QtCoap::Error error)
{
    const auto errorCode = reply ? reply->errorReceived() : error;
    addMessage(errorMessage(errorCode), true);
}

void MainWindow::onDiscovered(QCoapResourceDiscoveryReply *reply, QList<QCoapResource> resources)
{
    if (reply->errorReceived() != QtCoap::Error::Ok)
        return;

    QString message;
    for (const auto &resource : resources) {
        ui->resourceComboBox->addItem(resource.path());
        message += "Discovered resource: \"" + resource.title() + "\" on path "
                + resource.path() + "\n";
    }
    addMessage(message);
}

void MainWindow::onNotified(QCoapReply *reply, const QCoapMessage &message)
{
    if (reply->errorReceived() == QtCoap::Error::Ok)
        addMessage("Received observe notification with payload: " + message.payload());
}


static QString tryToResolveHostName(const QString hostName)
{
    const auto hostInfo = QHostInfo::fromName(hostName);
    if (!hostInfo.addresses().empty())
        return hostInfo.addresses().first().toString();

    return hostName;
}

void MainWindow::on_runButton_clicked()
{
    const auto msgType = ui->msgTypeCheckBox->isChecked() ? QCoapMessage::Type::Confirmable
                                                          : QCoapMessage::Type::NonConfirmable;
    QUrl url;
    url.setHost(tryToResolveHostName(ui->hostComboBox->currentText()));
    url.setPort(ui->portSpinBox->value());
    url.setPath(ui->resourceComboBox->currentText());

    QCoapRequest request(url, msgType);
    for (const auto &option : m_options)
        request.addOption(option);
    m_options.clear();

    const auto method = ui->methodComboBox->currentData(Qt::UserRole).value<QtCoap::Method>();
    switch (method) {
    case QtCoap::Method::Get:
        m_client->get(request);
        break;
    case QtCoap::Method::Put:
        m_client->put(request, m_currentData);
        break;
    case QtCoap::Method::Post:
        m_client->post(request, m_currentData);
        break;
    case QtCoap::Method::Delete:
        m_client->deleteResource(request);
        break;
    default:
        break;
    }
    m_currentData.clear();
}

void MainWindow::on_discoverButton_clicked()
{
    QUrl url;
    url.setHost(tryToResolveHostName(ui->hostComboBox->currentText()));
    url.setPort(ui->portSpinBox->value());

    QCoapResourceDiscoveryReply *discoverReply = m_client->discover(url, ui->discoveryPathEdit->text());
    if (discoverReply)
        connect(discoverReply, &QCoapResourceDiscoveryReply::discovered, this, &MainWindow::onDiscovered);
    else
        QMessageBox::critical(this, "Error", "Something went wrong, discovery request failed.");
}

void MainWindow::on_observeButton_clicked()
{
    QUrl url;
    url.setHost(tryToResolveHostName(ui->hostComboBox->currentText()));
    url.setPort(ui->portSpinBox->value());
    url.setPath(ui->resourceComboBox->currentText());

    QCoapReply *observeReply = m_client->observe(url);
    if (!observeReply) {
        QMessageBox::critical(this, "Error", "Something went wrong, observe request failed.");
        return;
    }

    connect(observeReply, &QCoapReply::notified, this, &MainWindow::onNotified);

    ui->cancelObserveButton->setEnabled(true);
    connect(ui->cancelObserveButton, &QPushButton::clicked, this, [=]() {
        m_client->cancelObserve(url);
        ui->cancelObserveButton->setEnabled(false);
    });
}

void MainWindow::on_addOptionsButton_clicked()
{
    OptionDialog dialog;
    if (dialog.exec() == QDialog::Accepted)
        m_options = dialog.options();
}

void MainWindow::on_contentButton_clicked()
{
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::AnyFile);
    if (!dialog.exec())
        return;

    const auto fileName = dialog.selectedFiles().back();
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "Error", QString("Failed to read from file %1").arg(fileName));
        return;
    }

    m_currentData = file.readAll();
}

void MainWindow::on_resourceComboBox_editTextChanged(const QString &text)
{
    ui->observeButton->setEnabled(!text.isEmpty());
}

void MainWindow::on_methodComboBox_currentIndexChanged(int index)
{
    Q_UNUSED(index);

    const auto method = ui->methodComboBox->currentData(Qt::UserRole).value<QtCoap::Method>();
    ui->contentButton->setEnabled(method == QtCoap::Method::Put || method == QtCoap::Method::Post);
}
