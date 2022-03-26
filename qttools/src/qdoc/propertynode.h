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

#ifndef PROPERTYNODE_H
#define PROPERTYNODE_H

#include "functionnode.h"
#include "node.h"

#include <QtCore/qglobal.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

class Aggregate;

class PropertyNode : public Node
{
public:
    enum PropertyType { Standard, Bindable };
    enum FunctionRole { Getter, Setter, Resetter, Notifier };
    enum { NumFunctionRoles = Notifier + 1 };

    PropertyNode(Aggregate *parent, const QString &name);

    void setDataType(const QString &dataType) override { m_type = dataType; }
    void addFunction(FunctionNode *function, FunctionRole role);
    void addSignal(FunctionNode *function, FunctionRole role);
    void setStored(bool stored) { m_stored = toFlagValue(stored); }
    void setDesignable(bool designable) { m_designable = toFlagValue(designable); }
    void setScriptable(bool scriptable) { m_scriptable = toFlagValue(scriptable); }
    void setWritable(bool writable) { m_writable = toFlagValue(writable); }
    void setOverriddenFrom(const PropertyNode *baseProperty);
    void setConstant() { m_const = true; }
    void setRequired() { m_required = true; }
    void setPropertyType(PropertyType type) { m_propertyType = type; }

    [[nodiscard]] const QString &dataType() const { return m_type; }
    [[nodiscard]] QString qualifiedDataType() const;
    [[nodiscard]] NodeList functions() const;
    [[nodiscard]] const NodeList &functions(FunctionRole role) const
    {
        return m_functions[(int)role];
    }
    [[nodiscard]] const NodeList &getters() const { return functions(Getter); }
    [[nodiscard]] const NodeList &setters() const { return functions(Setter); }
    [[nodiscard]] const NodeList &resetters() const { return functions(Resetter); }
    [[nodiscard]] const NodeList &notifiers() const { return functions(Notifier); }
    [[nodiscard]] bool hasAccessFunction(const QString &name) const;
    FunctionRole role(const FunctionNode *functionNode) const;
    [[nodiscard]] bool isStored() const { return fromFlagValue(m_stored, storedDefault()); }
    [[nodiscard]] bool isWritable() const { return fromFlagValue(m_writable, writableDefault()); }
    [[nodiscard]] bool isConstant() const { return m_const; }
    [[nodiscard]] bool isRequired() const { return m_required; }
    [[nodiscard]] PropertyType propertyType() const { return m_propertyType; }
    [[nodiscard]] const PropertyNode *overriddenFrom() const { return m_overrides; }

    [[nodiscard]] bool storedDefault() const { return true; }
    [[nodiscard]] bool designableDefault() const { return !setters().isEmpty(); }
    [[nodiscard]] bool writableDefault() const { return !setters().isEmpty(); }

private:
    QString m_type {};
    PropertyType m_propertyType { Standard };
    NodeList m_functions[NumFunctionRoles] {};
    FlagValue m_stored { FlagValueDefault };
    FlagValue m_designable { FlagValueDefault };
    FlagValue m_scriptable { FlagValueDefault };
    FlagValue m_writable { FlagValueDefault };
    FlagValue m_user { FlagValueDefault };
    bool m_const { false };
    bool m_required { false };
    const PropertyNode *m_overrides { nullptr };
};

inline void PropertyNode::addFunction(FunctionNode *function, FunctionRole role)
{
    m_functions[(int)role].append(function);
    function->addAssociatedProperty(this);
}

inline void PropertyNode::addSignal(FunctionNode *function, FunctionRole role)
{
    m_functions[(int)role].append(function);
    function->addAssociatedProperty(this);
}

inline NodeList PropertyNode::functions() const
{
    NodeList list;
    for (int i = 0; i < NumFunctionRoles; ++i)
        list += m_functions[i];
    return list;
}

QT_END_NAMESPACE

#endif // PROPERTYNODE_H
