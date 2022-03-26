/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Charts module of the Qt Toolkit.
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

#include "utilities.h"
#include <QBarSet>
#include <QApplication>
#include <QBarSeries>
#include <QList>
#include <QChart>
#include <QBarCategoryAxis>
#include <QValueAxis>
#include <QChartView>
#include <QWindow>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QMainWindow mainWindow;
    mainWindow.setWindowTitle(QCoreApplication::tr("Meat consumption series"));

    //![1]
    QBarSet *setChicken = Utilities::createChickenSet();
    QBarSet *setPork = Utilities::createPorkSet();
    QBarSet *setTurkey = Utilities::createTurkeySet();
    QBarSet *setHam = Utilities::createHamSet();
    qreal totalSum = setChicken->sum() + setPork->sum() + setTurkey->sum() + setHam->sum();
    QList<QBarSet *> setList = QList<QBarSet *>{setChicken, setPork, setTurkey, setHam};

    QBarSeries *series = new QBarSeries();
    series->append(setList);
    //![1]

    //![2]
    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle(QCoreApplication::tr("Meat consumption"));
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);
    //![2]

    //![3]
    for (QBarSet *barSet : series->barSets())
        barSet->setSelectedColor(barSet->brush().color().darker());
    //![3]

    //![4]
    QStringList categories = Utilities::createYearCategories();
    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->setCategories(categories);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setRange(0, 20);
    axisY->setTitleText(QCoreApplication::tr("Tons"));
    axisY->setLabelsAngle(-90);
    axisY->setTitleVisible(true);
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);
    //![4]

    //![5]
    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    //![5]

    //![6]
    QWidget *labelWidget = new QWidget(&mainWindow);
    QHBoxLayout *labelLayout = new QHBoxLayout(labelWidget);
    labelLayout->setAlignment(Qt::AlignCenter);

    QLabel *totalSumLabel = new QLabel(QCoreApplication::tr("Total sum: %1 T").arg(totalSum));
    labelLayout->addWidget(totalSumLabel);
    totalSumLabel->setContentsMargins(0, 0, 54, 0);

    QLabel *selectedSumLabel = new QLabel(QCoreApplication::tr("Selected sum: 0 T"));
    labelLayout->addWidget(selectedSumLabel);

    QLabel *unselectedSumLabel = new QLabel(QCoreApplication::tr("Unselected sum: %1 T").arg(totalSum));
    labelLayout->addWidget(unselectedSumLabel);
    unselectedSumLabel->setContentsMargins(54, 0, 0, 0);
    //![6]

    //![7]
    QObject::connect(series, &QAbstractBarSeries::clicked, series, [=](int index, QBarSet *set) {
        set->toggleSelection({index});
        qreal selectedSum = 0.;
        for (int i = 0; i < setList.size(); ++i) {
            auto selectedIndices = setList.at(i)->selectedBars();
            for (int k = 0; k < selectedIndices.size(); ++k)
                selectedSum += setList.at(i)->at(selectedIndices.at(k));
        }
        selectedSumLabel->setText(QCoreApplication::tr("Selected sum: %1 T").arg(selectedSum));
        // Because of rounding errors, selectedSum can result in being bigger than total sum
        qreal unselectedSum = totalSum - selectedSum < 0 ? 0. : totalSum - selectedSum;
        unselectedSumLabel->setText(
                                    QCoreApplication::tr("Unselected sum: %1 T")
                                    .arg(unselectedSum)
                                   );
    });
    //![7]

    //![8]
    QWidget *mainWidget = new QWidget(&mainWindow);
    QVBoxLayout *mainLayout = new QVBoxLayout(mainWidget);

    mainLayout->addWidget(chartView);
    mainLayout->addWidget(labelWidget);

    mainWindow.setCentralWidget(mainWidget);
    mainWindow.resize(800, 600);

    mainWindow.show();
    return a.exec();
    //![8]
}
