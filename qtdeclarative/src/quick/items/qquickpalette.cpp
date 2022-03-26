/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickpalette_p.h"

#include <QtQuick/private/qquickpalettecolorprovider_p.h>

QT_BEGIN_NAMESPACE

/*!
    \internal

    \class QQuickPalette
    \brief The QQuickPalette class contains color groups for each QML item state.
    \inmodule QtQuick
    \since 6.0

    This class is the wrapper around QPalette.

    \sa QQuickColorGroup, QQuickAbstractPaletteProvider, QPalette
 */

/*!
    \qmltype Palette
    \instantiates QQuickPalette
    \inherits QQuickColorGroup
    \inqmlmodule QtQuick
    \ingroup qtquick-visual
    \brief The QQuickPalette class contains color groups for each QML item state.

    A palette consists of three color groups: Active, Disabled, and Inactive.
    Active color group is the default group, its colors are used for other groups
    if colors of these groups aren't explicitly specified.

    In the following example, color is applied for all color groups:
    \code
    ApplicationWindow {
        palette.buttonText: "salmon"

        ColumnLayout {
            Button {
                text: qsTr("Disabled button")
                enabled: false
            }

            Button {
                text: qsTr("Enabled button")
            }
        }
    }
    \endcode
    It means that text color will be the same for both buttons.

    In the following example, colors will be different for enabled and disabled states:
    \code
    ApplicationWindow {
        palette.buttonText: "salmon"
        palette.disabled.buttonText: "lavender"

        ColumnLayout {
            Button {
                text: qsTr("Disabled button")
                enabled: false
            }

            Button {
                text: qsTr("Enabled button")
            }
        }
    }
    \endcode

    It is also possible to specify colors like this:
    \code
    palette {
        buttonText: "azure"
        button: "khaki"

        disabled {
            buttonText: "lavender"
            button: "coral"
        }
    }
    \endcode
    This approach is convenient when you need to specify a whole palette with all color groups.
*/

/*!
    \qmlproperty QQuickColorGroup QtQuick::Palette::active

    The Active group is used for windows that are in focus.

    \sa QPalette::Active
*/

/*!
    \qmlproperty QQuickColorGroup QtQuick::Palette::inactive

    The Inactive group is used for windows that have no keyboard focus.

    \sa QPalette::Inactive
*/

/*!
    \qmlproperty QQuickColorGroup QtQuick::Palette::disabled

    The Disabled group is used for elements that are disabled for some reason.

    \sa QPalette::Disabled
*/

QQuickPalette::QQuickPalette(QObject *parent)
    : QQuickColorGroup(parent)
    , m_currentGroup(defaultCurrentGroup())
{
}

QQuickColorGroup *QQuickPalette::active() const
{
    return colorGroup(QPalette::Active);
}

QQuickColorGroup *QQuickPalette::inactive() const
{
    return colorGroup(QPalette::Inactive);
}

QQuickColorGroup *QQuickPalette::disabled() const
{
    return colorGroup(QPalette::Disabled);
}

/*!
    \internal

    Returns the palette's current color group.
    The default value is Active.
 */
QPalette::ColorGroup QQuickPalette::currentColorGroup() const
{
    return m_currentGroup;
}

/*!
    \internal

    Sets \a currentGroup for this palette.

    The current color group is used when accessing colors of this palette.
    For example, if color group is Disabled, color accessors will be
    returning colors form the respective group.
    \code
    QQuickPalette palette;

    palette.setAlternateBase(Qt::green);
    palette.disabled()->setAlternateBase(Qt::red);

    auto color = palette.alternateBase(); // Qt::green

    palette.setCurrentGroup(QPalette::Disabled);
    color = palette.alternateBase(); // Qt::red
    \endcode

    Emits QColorGroup::changed().
 */
void QQuickPalette::setCurrentGroup(QPalette::ColorGroup currentGroup)
{
    if (m_currentGroup != currentGroup) {
        m_currentGroup = currentGroup;
        Q_EMIT changed();
    }
}

void QQuickPalette::fromQPalette(QPalette palette)
{
    if (colorProvider().fromQPalette(std::move(palette))) {
        Q_EMIT changed();
    }
}

QPalette QQuickPalette::toQPalette() const
{
    return colorProvider().palette();
}

const QQuickAbstractPaletteProvider *QQuickPalette::paletteProvider() const
{
    return colorProvider().paletteProvider();
}

void QQuickPalette::setPaletteProvider(const QQuickAbstractPaletteProvider *paletteProvider)
{
    colorProvider().setPaletteProvider(paletteProvider);
}

void QQuickPalette::reset()
{
    if (colorProvider().reset()) {
        Q_EMIT changed();
    }
}

void QQuickPalette::inheritPalette(const QPalette &palette)
{
    if (colorProvider().inheritPalette(palette)) {
        Q_EMIT changed();
    }
}

void QQuickPalette::setActive(QQuickColorGroup *active)
{
    setColorGroup(QPalette::Active, active, &QQuickPalette::activeChanged);
}

void QQuickPalette::setInactive(QQuickColorGroup *inactive)
{
    setColorGroup(QPalette::Inactive, inactive, &QQuickPalette::inactiveChanged);
}

void QQuickPalette::setDisabled(QQuickColorGroup *disabled)
{
    setColorGroup(QPalette::Disabled, disabled, &QQuickPalette::disabledChanged);
}


void QQuickPalette::setColorGroup(QPalette::ColorGroup groupTag,
                                  const QQuickColorGroup::GroupPtr &group,
                                  void (QQuickPalette::*notifier)())
{
    if (isValidColorGroup(groupTag, group)) {
        if (colorProvider().copyColorGroup(groupTag, group->colorProvider())) {
            Q_EMIT (this->*notifier)();
            Q_EMIT changed();
        }
    }
}

QQuickColorGroup::GroupPtr QQuickPalette::colorGroup(QPalette::ColorGroup groupTag) const
{
    if (auto group = findColorGroup(groupTag)) {
        return group;
    }

    auto group = QQuickColorGroup::createWithParent(*const_cast<QQuickPalette*>(this));
    const_cast<QQuickPalette*>(this)->registerColorGroup(group, groupTag);
    return group;
}

QQuickColorGroup::GroupPtr QQuickPalette::findColorGroup(QPalette::ColorGroup groupTag) const
{
    if (auto it = m_colorGroups.find(groupTag); it != m_colorGroups.end()) {
        return it->second;
    }

    return nullptr;
}

void QQuickPalette::registerColorGroup(QQuickColorGroup *group, QPalette::ColorGroup groupTag)
{
    if (auto it = m_colorGroups.find(groupTag); it != m_colorGroups.end() && it->second) {
        it->second->deleteLater();
    }

    m_colorGroups[groupTag] = group;

    group->setGroupTag(groupTag);

    QQuickColorGroup::connect(group, &QQuickColorGroup::changed, this, &QQuickPalette::changed);
}

bool QQuickPalette::isValidColorGroup(QPalette::ColorGroup groupTag,
                                      const QQuickColorGroup::GroupPtr &colorGroup) const
{
    if (!colorGroup) {
        qWarning("Color group cannot be null.");
        return false;
    }

    if (!colorGroup->parent()) {
        qWarning("Color group should have a parent.");
        return false;
    }

    if (colorGroup->parent() && !qobject_cast<QQuickPalette*>(colorGroup->parent())) {
        qWarning("Color group should be a part of QQuickPalette.");
        return false;
    }

    if (groupTag == defaultGroupTag()) {
        qWarning("Register %i color group is not allowed."
                 " QQuickPalette is %i color group itself.", groupTag, groupTag);
        return false;
    }

    if (findColorGroup(groupTag) == colorGroup) {
        qWarning("The color group is already a part of the current palette.");
        return false;
    }

    return true;
}

QT_END_NAMESPACE
