/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Labs Platform module of the Qt Toolkit.
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

#include "qquicklabsplatformmenuitem_p.h"
#include "qquicklabsplatformmenu_p.h"
#include "qquicklabsplatformmenuitemgroup_p.h"
#include "qquicklabsplatformiconloader_p.h"

#include <QtGui/qicon.h>
#if QT_CONFIG(shortcut)
#include <QtGui/qkeysequence.h>
#endif
#include <QtGui/qpa/qplatformtheme.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtQuickTemplates2/private/qquickshortcutcontext_p_p.h>

#include "widgets/qwidgetplatform_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype MenuItem
    \inherits QtObject
//!     \instantiates QQuickLabsPlatformMenuItem
    \inqmlmodule Qt.labs.platform
    \since 5.8
    \brief A native menu item.

    The MenuItem type provides a QML API for native platform menu items.

    \image qtlabsplatform-menu.png

    A menu item consists of an \l icon, \l text, and \l shortcut.

    \code
    Menu {
        id: zoomMenu

        MenuItem {
            text: qsTr("Zoom In")
            shortcut: StandardKey.ZoomIn
            onTriggered: zoomIn()
        }

        MenuItem {
            text: qsTr("Zoom Out")
            shortcut: StandardKey.ZoomOut
            onTriggered: zoomOut()
        }
    }
    \endcode

    \labs

    \sa Menu, MenuItemGroup
*/

/*!
    \qmlsignal Qt.labs.platform::MenuItem::triggered()

    This signal is emitted when the menu item is triggered by the user.
*/

/*!
    \qmlsignal Qt.labs.platform::MenuItem::hovered()

    This signal is emitted when the menu item is hovered by the user.
*/

QQuickLabsPlatformMenuItem::QQuickLabsPlatformMenuItem(QObject *parent)
    : QObject(parent),
      m_complete(false),
      m_enabled(true),
      m_visible(true),
      m_separator(false),
      m_checkable(false),
      m_checked(false),
      m_role(QPlatformMenuItem::TextHeuristicRole),
      m_menu(nullptr),
      m_subMenu(nullptr),
      m_group(nullptr),
      m_iconLoader(nullptr),
      m_handle(nullptr)
{
}

QQuickLabsPlatformMenuItem::~QQuickLabsPlatformMenuItem()
{
    if (m_menu)
        m_menu->removeItem(this);
    if (m_group)
        m_group->removeItem(this);
#if QT_CONFIG(shortcut)
    if (m_shortcutId != -1) {
        QKeySequence sequence;
        if (m_shortcut.metaType().id() == QMetaType::Int)
            sequence = QKeySequence(static_cast<QKeySequence::StandardKey>(m_shortcut.toInt()));
        else
            sequence = QKeySequence::fromString(m_shortcut.toString());
        QGuiApplicationPrivate::instance()->shortcutMap.removeShortcut(m_shortcutId, this, sequence);
    }
#endif
    delete m_iconLoader;
    m_iconLoader = nullptr;
    delete m_handle;
    m_handle = nullptr;
}

QPlatformMenuItem *QQuickLabsPlatformMenuItem::handle() const
{
    return m_handle;
}

QPlatformMenuItem *QQuickLabsPlatformMenuItem::create()
{
    if (!m_handle && m_menu && m_menu->handle()) {
        m_handle = m_menu->handle()->createMenuItem();

        // TODO: implement QCocoaMenu::createMenuItem()
        if (!m_handle)
            m_handle = QGuiApplicationPrivate::platformTheme()->createPlatformMenuItem();

        if (!m_handle)
            m_handle = QWidgetPlatform::createMenuItem();

        if (m_handle) {
            connect(m_handle, &QPlatformMenuItem::activated, this, &QQuickLabsPlatformMenuItem::activate);
            connect(m_handle, &QPlatformMenuItem::hovered, this, &QQuickLabsPlatformMenuItem::hovered);
        }
    }
    return m_handle;
}

void QQuickLabsPlatformMenuItem::sync()
{
    if (!m_complete || !create())
        return;

    m_handle->setEnabled(isEnabled());
    m_handle->setVisible(isVisible());
    m_handle->setIsSeparator(m_separator);
    m_handle->setCheckable(m_checkable);
    m_handle->setChecked(m_checked);
    m_handle->setRole(m_role);
    m_handle->setText(m_text);
    m_handle->setFont(m_font);
    m_handle->setHasExclusiveGroup(m_group && m_group->isExclusive());
    if (m_subMenu) {
        // Sync first as dynamically created menus may need to get the
        // handle recreated
        m_subMenu->sync();
        if (m_subMenu->handle())
            m_handle->setMenu(m_subMenu->handle());
    }

#if QT_CONFIG(shortcut)
    QKeySequence sequence;
    if (m_shortcut.metaType().id() == QMetaType::Int)
        sequence = QKeySequence(static_cast<QKeySequence::StandardKey>(m_shortcut.toInt()));
    else
        sequence = QKeySequence::fromString(m_shortcut.toString());
    m_handle->setShortcut(sequence.toString());
#endif

    if (m_menu && m_menu->handle())
        m_menu->handle()->syncMenuItem(m_handle);
}

/*!
    \readonly
    \qmlproperty Menu Qt.labs.platform::MenuItem::menu

    This property holds the menu that the item belongs to, or \c null if the
    item is not in a menu.
*/
QQuickLabsPlatformMenu *QQuickLabsPlatformMenuItem::menu() const
{
    return m_menu;
}

void QQuickLabsPlatformMenuItem::setMenu(QQuickLabsPlatformMenu *menu)
{
    if (m_menu == menu)
        return;

    m_menu = menu;
    emit menuChanged();
}

/*!
    \readonly
    \qmlproperty Menu Qt.labs.platform::MenuItem::subMenu

    This property holds the sub-menu that the item contains, or \c null if
    the item is not a sub-menu item.
*/
QQuickLabsPlatformMenu *QQuickLabsPlatformMenuItem::subMenu() const
{
    return m_subMenu;
}

void QQuickLabsPlatformMenuItem::setSubMenu(QQuickLabsPlatformMenu *menu)
{
    if (m_subMenu == menu)
        return;

    m_subMenu = menu;
    sync();
    emit subMenuChanged();
}

/*!
    \qmlproperty MenuItemGroup Qt.labs.platform::MenuItem::group

    This property holds the group that the item belongs to, or \c null if the
    item is not in a group.
*/
QQuickLabsPlatformMenuItemGroup *QQuickLabsPlatformMenuItem::group() const
{
    return m_group;
}

void QQuickLabsPlatformMenuItem::setGroup(QQuickLabsPlatformMenuItemGroup *group)
{
    if (m_group == group)
        return;

    bool wasEnabled = isEnabled();
    bool wasVisible = isVisible();

    if (group)
        group->addItem(this);

    m_group = group;
    sync();
    emit groupChanged();

    if (isEnabled() != wasEnabled)
        emit enabledChanged();
    if (isVisible() != wasVisible)
        emit visibleChanged();
}

/*!
    \qmlproperty bool Qt.labs.platform::MenuItem::enabled

    This property holds whether the item is enabled. The default value is \c true.

    Disabled items cannot be triggered by the user. They do not disappear from menus,
    but they are displayed in a way which indicates that they are unavailable. For
    example, they might be displayed using only shades of gray.

    When an item is disabled, it is not possible to trigger it through its \l shortcut.
*/
bool QQuickLabsPlatformMenuItem::isEnabled() const
{
    return m_enabled && (!m_group || m_group->isEnabled());
}

void QQuickLabsPlatformMenuItem::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;

    bool wasEnabled = isEnabled();
    m_enabled = enabled;
    sync();
    if (isEnabled() != wasEnabled)
        emit enabledChanged();
}

/*!
    \qmlproperty bool Qt.labs.platform::MenuItem::visible

    This property holds whether the item is visible. The default value is \c true.
*/
bool QQuickLabsPlatformMenuItem::isVisible() const
{
    return m_visible && (!m_group || m_group->isVisible());
}

void QQuickLabsPlatformMenuItem::setVisible(bool visible)
{
    if (m_visible == visible)
        return;

    bool wasVisible = isVisible();
    m_visible = visible;
    sync();
    if (isVisible() != wasVisible)
        emit visibleChanged();
}

/*!
    \qmlproperty bool Qt.labs.platform::MenuItem::separator

    This property holds whether the item is a separator line. The default value
    is \c false.

    \sa MenuSeparator
*/
bool QQuickLabsPlatformMenuItem::isSeparator() const
{
    return m_separator;
}

void QQuickLabsPlatformMenuItem::setSeparator(bool separator)
{
    if (m_separator == separator)
        return;

    m_separator = separator;
    sync();
    emit separatorChanged();
}

/*!
    \qmlproperty bool Qt.labs.platform::MenuItem::checkable

    This property holds whether the item is checkable.

    A checkable menu item has an on/off state. For example, in a word processor,
    a "Bold" menu item may be either on or off. A menu item that is not checkable
    is a command item that is simply executed, e.g. file save.

    The default value is \c false.

    \sa checked, MenuItemGroup
*/
bool QQuickLabsPlatformMenuItem::isCheckable() const
{
    return m_checkable;
}

void QQuickLabsPlatformMenuItem::setCheckable(bool checkable)
{
    if (m_checkable == checkable)
        return;

    m_checkable = checkable;
    sync();
    emit checkableChanged();
}

/*!
    \qmlproperty bool Qt.labs.platform::MenuItem::checked

    This property holds whether the item is checked (on) or unchecked (off).
    The default value is \c false.

    \sa checkable, MenuItemGroup
*/
bool QQuickLabsPlatformMenuItem::isChecked() const
{
    return m_checked;
}

void QQuickLabsPlatformMenuItem::setChecked(bool checked)
{
    if (m_checked == checked)
        return;

    if (checked && !m_checkable)
        setCheckable(true);

    m_checked = checked;
    sync();
    emit checkedChanged();
}

/*!
    \qmlproperty enumeration Qt.labs.platform::MenuItem::role

    This property holds the role of the item. The role determines whether
    the item should be placed into the application menu on macOS.

    Available values:
    \value MenuItem.NoRole The item should not be put into the application menu
    \value MenuItem.TextHeuristicRole The item should be put in the application menu based on the action's text (default)
    \value MenuItem.ApplicationSpecificRole The item should be put in the application menu with an application-specific role
    \value MenuItem.AboutQtRole The item handles the "About Qt" menu item.
    \value MenuItem.AboutRole The item should be placed where the "About" menu item is in the application menu. The text of
           the menu item will be set to "About <application name>". The application name is fetched from the
           \c{Info.plist} file in the application's bundle (See \l{Qt for macOS - Deployment}).
    \value MenuItem.PreferencesRole The item should be placed where the "Preferences..." menu item is in the application menu.
    \value MenuItem.QuitRole The item should be placed where the Quit menu item is in the application menu.

    Specifying the role only has effect on items that are in the immediate
    menus of a menubar, not in the submenus of those menus. For example, if
    you have a "File" menu in your menubar and the "File" menu has a submenu,
    specifying a role for the items in that submenu has no effect. They will
    never be moved to the application menu.
*/
QPlatformMenuItem::MenuRole QQuickLabsPlatformMenuItem::role() const
{
    return m_role;
}

void QQuickLabsPlatformMenuItem::setRole(QPlatformMenuItem::MenuRole role)
{
    if (m_role == role)
        return;

    m_role = role;
    sync();
    emit roleChanged();
}

/*!
    \qmlproperty string Qt.labs.platform::MenuItem::text

    This property holds the menu item's text.
*/
QString QQuickLabsPlatformMenuItem::text() const
{
    return m_text;
}

void QQuickLabsPlatformMenuItem::setText(const QString &text)
{
    if (m_text == text)
        return;

    m_text = text;
    sync();
    emit textChanged();
}

/*!
    \qmlproperty keysequence Qt.labs.platform::MenuItem::shortcut

    This property holds the menu item's shortcut.

    The shortcut key sequence can be set to one of the
    \l{QKeySequence::StandardKey}{standard keyboard shortcuts}, or it can be
    specified by a string containing a sequence of up to four key presses
    that are needed to \l{triggered}{trigger} the shortcut.

    The default value is an empty key sequence.

    \code
    MenuItem {
        shortcut: "Ctrl+E,Ctrl+W"
        onTriggered: edit.wrapMode = TextEdit.Wrap
    }
    \endcode
*/
QVariant QQuickLabsPlatformMenuItem::shortcut() const
{
    return m_shortcut;
}

bool QQuickLabsPlatformMenuItem::event(QEvent *e)
{
#if QT_CONFIG(shortcut)
    if (e->type() == QEvent::Shortcut) {
        QShortcutEvent *se = static_cast<QShortcutEvent *>(e);
        if (se->shortcutId() == m_shortcutId) {
            activate();
            return true;
        }
    }
#endif
    return QObject::event(e);
}

void QQuickLabsPlatformMenuItem::setShortcut(const QVariant& shortcut)
{
    if (m_shortcut == shortcut)
        return;

#if QT_CONFIG(shortcut)
    if (m_shortcutId != -1) {
        QKeySequence sequence;
        if (m_shortcut.metaType().id() == QMetaType::Int)
            sequence = QKeySequence(static_cast<QKeySequence::StandardKey>(m_shortcut.toInt()));
        else
            sequence = QKeySequence::fromString(m_shortcut.toString());
        QGuiApplicationPrivate::instance()->shortcutMap.removeShortcut(m_shortcutId, this, sequence);
    }
#endif
    m_shortcut = shortcut;
    sync();
#if QT_CONFIG(shortcut)
    QKeySequence sequence;
    if (m_shortcut.metaType().id() == QMetaType::Int)
        sequence = QKeySequence(static_cast<QKeySequence::StandardKey>(m_shortcut.toInt()));
    else
        sequence = QKeySequence::fromString(m_shortcut.toString());
    if (!sequence.isEmpty()) {
        m_shortcutId = QGuiApplicationPrivate::instance()->shortcutMap.addShortcut(this, sequence,
            Qt::WindowShortcut, QQuickShortcutContext::matcher);
    } else {
        m_shortcutId = -1;
    }
#endif
    emit shortcutChanged();
}

/*!
    \qmlproperty font Qt.labs.platform::MenuItem::font

    This property holds the menu item's font.

    \sa text
*/
QFont QQuickLabsPlatformMenuItem::font() const
{
    return m_font;
}

void QQuickLabsPlatformMenuItem::setFont(const QFont& font)
{
    if (m_font == font)
        return;

    m_font = font;
    sync();
    emit fontChanged();
}

/*!
    \since Qt.labs.platform 1.1 (Qt 5.12)
    \qmlproperty url Qt.labs.platform::MenuItem::icon.source
    \qmlproperty string Qt.labs.platform::MenuItem::icon.name
    \qmlproperty bool Qt.labs.platform::MenuItem::icon.mask

    This property holds the menu item's icon.

    \code
    MenuItem {
        icon.mask: true
        icon.name: "edit-undo"
        icon.source: "qrc:/images/undo.png"
    }
    \endcode

    \sa QIcon::fromTheme()
*/
QQuickLabsPlatformIcon QQuickLabsPlatformMenuItem::icon() const
{
    if (!m_iconLoader)
        return QQuickLabsPlatformIcon();

    return m_iconLoader->icon();
}

void QQuickLabsPlatformMenuItem::setIcon(const QQuickLabsPlatformIcon &icon)
{
    if (iconLoader()->icon() == icon)
        return;

    iconLoader()->setIcon(icon);
    emit iconChanged();
}

/*!
    \qmlmethod void Qt.labs.platform::MenuItem::toggle()

    Toggles the \l checked state to its opposite state.
*/
void QQuickLabsPlatformMenuItem::toggle()
{
    if (m_checkable)
        setChecked(!m_checked);
}

void QQuickLabsPlatformMenuItem::classBegin()
{
}

void QQuickLabsPlatformMenuItem::componentComplete()
{
    if (m_handle && m_iconLoader)
        m_iconLoader->setEnabled(true);
    m_complete = true;
    sync();
}

QQuickLabsPlatformIconLoader *QQuickLabsPlatformMenuItem::iconLoader() const
{
    if (!m_iconLoader) {
        QQuickLabsPlatformMenuItem *that = const_cast<QQuickLabsPlatformMenuItem *>(this);
        static int slot = staticMetaObject.indexOfSlot("updateIcon()");
        m_iconLoader = new QQuickLabsPlatformIconLoader(slot, that);
        m_iconLoader->setEnabled(m_complete);
    }
    return m_iconLoader;
}

void QQuickLabsPlatformMenuItem::activate()
{
    toggle();
    emit triggered();
}

void QQuickLabsPlatformMenuItem::updateIcon()
{
    if (!m_handle || !m_iconLoader)
        return;

    m_handle->setIcon(m_iconLoader->toQIcon());
    sync();
}

QT_END_NAMESPACE

#include "moc_qquicklabsplatformmenuitem_p.cpp"
