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

#include "optiondialog.h"
#include "ui_optiondialog.h"

OptionDialog::OptionDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OptionDialog)
{
    ui->setupUi(this);

    fillOptions();

    auto header = ui->tableWidget->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::Stretch);
}

OptionDialog::~OptionDialog()
{
    delete ui;
}

QList<QCoapOption> OptionDialog::options() const
{
    return m_options;
}

void OptionDialog::on_addButton_clicked()
{
    const auto option =
            ui->optionComboBox->currentData(Qt::UserRole).value<QCoapOption::OptionName>();
    m_options.push_back(QCoapOption(option, ui->optionValueEdit->text()));

    const auto rowCount = ui->tableWidget->rowCount();
    ui->tableWidget->insertRow(rowCount);

    QTableWidgetItem *optionItem = new QTableWidgetItem(ui->optionComboBox->currentText());
    optionItem->setFlags(optionItem->flags() ^ Qt::ItemIsEditable);
    ui->tableWidget->setItem(rowCount, 0, optionItem);

    QTableWidgetItem *valueItem = new QTableWidgetItem(ui->optionValueEdit->text());
    valueItem->setFlags(valueItem->flags() ^ Qt::ItemIsEditable);
    ui->tableWidget->setItem(rowCount, 1, valueItem);
}

void OptionDialog::on_clearButton_clicked()
{
    m_options.clear();
    ui->tableWidget->setRowCount(0);
}

void OptionDialog::fillOptions()
{
    ui->tableWidget->setHorizontalHeaderLabels({"Name", "Value"});
    ui->optionComboBox->addItem("None", QCoapOption::Invalid);
    ui->optionComboBox->addItem("Block1", QCoapOption::Block1);
    ui->optionComboBox->addItem("Block2", QCoapOption::Block2);
    ui->optionComboBox->addItem("Content-Format", QCoapOption::ContentFormat);
    ui->optionComboBox->addItem("If-Match", QCoapOption::IfMatch);
    ui->optionComboBox->addItem("If-None-Match", QCoapOption::IfNoneMatch);
    ui->optionComboBox->addItem("Location-Path", QCoapOption::LocationPath);
    ui->optionComboBox->addItem("Location-Query", QCoapOption::LocationQuery);
    ui->optionComboBox->addItem("Max-Age", QCoapOption::MaxAge);
    ui->optionComboBox->addItem("Observe", QCoapOption::Observe);
    ui->optionComboBox->addItem("Proxy-Scheme", QCoapOption::ProxyScheme);
    ui->optionComboBox->addItem("Proxy-Uri", QCoapOption::ProxyUri);
    ui->optionComboBox->addItem("Size1", QCoapOption::Size1);
    ui->optionComboBox->addItem("Size2", QCoapOption::Size2);
    ui->optionComboBox->addItem("Uri-Host", QCoapOption::UriHost);
    ui->optionComboBox->addItem("Uri-Path", QCoapOption::UriPath);
    ui->optionComboBox->addItem("Uri-Port", QCoapOption::UriPort);
    ui->optionComboBox->addItem("Uri-Query", QCoapOption::UriQuery);
}
