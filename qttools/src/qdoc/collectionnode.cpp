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

#include "collectionnode.h"

#include <QtCore/qstringlist.h>

QT_BEGIN_NAMESPACE

/*!
  \class CollectionNode
  \brief A class for holding the members of a collection of doc pages.
 */

/*!
  Appends \a node to the collection node's member list, if
  and only if it isn't already in the member list.
 */
void CollectionNode::addMember(Node *node)
{
    if (!m_members.contains(node))
        m_members.append(node);
}

/*!
  Returns \c true if this collection node contains at least
  one namespace node.
 */
bool CollectionNode::hasNamespaces() const
{
    return std::any_of(m_members.cbegin(), m_members.cend(), [](const Node *member) {
        return member->isClassNode() && member->isInAPI();
    });
}

/*!
  Returns \c true if this collection node contains at least
  one class node.
 */
bool CollectionNode::hasClasses() const
{
    return std::any_of(m_members.cbegin(), m_members.cend(), [](const Node *member) {
        return member->isClassNode() && member->isInAPI();
    });
}

/*!
  Loads \a out with all this collection node's members that
  are namespace nodes.
 */
void CollectionNode::getMemberNamespaces(NodeMap &out)
{
    out.clear();
    for (const auto &member : qAsConst(m_members)) {
        if (member->isNamespace() && member->isInAPI())
            out.insert(member->name(), member);
    }
}

/*!
  Loads \a out with all this collection node's members that
  are class nodes.
 */
void CollectionNode::getMemberClasses(NodeMap &out) const
{
    out.clear();
    for (const auto &member : qAsConst(m_members)) {
        if (member->isClassNode() && member->isInAPI())
            out.insert(member->name(), member);
    }
}

/*!
  Returns the logical module version.
*/
QString CollectionNode::logicalModuleVersion() const
{
    QStringList version;
    version << m_logicalModuleVersionMajor << m_logicalModuleVersionMinor;
    version.removeAll(QString());
    return version.join(".");
}

/*!
  This function splits \a arg on the blank character to get a
  logical module name and version number. If the version number
  is present, it splits the version number on the '.' character
  to get a major version number and a minor version number. If
  the version number is present, both the major and minor version
  numbers should be there, but the minor version number is not
  absolutely necessary.
 */
void CollectionNode::setLogicalModuleInfo(const QString &arg)
{
    QStringList blankSplit = arg.split(QLatin1Char(' '));
    m_logicalModuleName = blankSplit[0];
    if (blankSplit.size() > 1) {
        QStringList dotSplit = blankSplit[1].split(QLatin1Char('.'));
        m_logicalModuleVersionMajor = dotSplit[0];
        if (dotSplit.size() > 1)
            m_logicalModuleVersionMinor = dotSplit[1];
        else
            m_logicalModuleVersionMinor = "0";
    }
}

/*!
  This function accepts the logical module \a info as a string
  list. If the logical module info contains the version number,
  it splits the version number on the '.' character to get the
  major and minor version numbers. Both major and minor version
  numbers should be provided, but the minor version number is
  not strictly necessary.
 */
void CollectionNode::setLogicalModuleInfo(const QStringList &info)
{
    m_logicalModuleName = info[0];
    if (info.size() > 1) {
        QStringList dotSplit = info[1].split(QLatin1Char('.'));
        m_logicalModuleVersionMajor = dotSplit[0];
        if (dotSplit.size() > 1)
            m_logicalModuleVersionMinor = dotSplit[1];
        else
            m_logicalModuleVersionMinor = "0";
    }
}

QT_END_NAMESPACE
