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

#ifndef FUNCTIONNODE_H
#define FUNCTIONNODE_H

#include "aggregate.h"
#include "node.h"
#include "parameters.h"

#include <QtCore/qglobal.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

class FunctionNode : public Node
{
public:
    enum Virtualness { NonVirtual, NormalVirtual, PureVirtual };

    enum Metaness {
        Plain,
        Signal,
        Slot,
        Ctor,
        Dtor,
        CCtor, // copy constructor
        MCtor, // move-copy constructor
        MacroWithParams,
        MacroWithoutParams,
        Native,
        CAssign, // copy-assignment operator
        MAssign, // move-assignment operator
        QmlSignal,
        QmlSignalHandler,
        QmlMethod,
        JsSignal,
        JsSignalHandler,
        JsMethod
    };

    FunctionNode(Aggregate *parent, const QString &name); // C++ function (Plain)
    FunctionNode(Metaness type, Aggregate *parent, const QString &name, bool attached = false);

    Node *clone(Aggregate *parent) override;
    [[nodiscard]] Metaness metaness() const { return m_metaness; }
    [[nodiscard]] QString metanessString() const;
    bool changeMetaness(Metaness from, Metaness to);
    void setMetaness(Metaness metaness) { m_metaness = metaness; }
    [[nodiscard]] QString kindString() const;
    static Metaness getMetaness(const QString &value);
    static Metaness getMetanessFromTopic(const QString &topic);
    static Genus getGenus(Metaness metaness);

    void setReturnType(const QString &type) { m_returnType = type; }
    void setParentPath(const QStringList &path) { m_parentPath = path; }
    void setVirtualness(const QString &value);
    void setVirtualness(Virtualness virtualness) { m_virtualness = virtualness; }
    void setVirtual() { m_virtualness = NormalVirtual; }
    void setConst(bool b) { m_const = b; }
    void setDefault(bool b) { m_default = b; }
    void setStatic(bool b) { m_static = b; }
    void setReimpFlag() { m_reimpFlag = true; }
    void setOverridesThis(const QString &path) { m_overridesThis = path; }

    [[nodiscard]] const QString &returnType() const { return m_returnType; }
    [[nodiscard]] QString virtualness() const;
    [[nodiscard]] bool isConst() const { return m_const; }
    [[nodiscard]] bool isDefault() const override { return m_default; }
    [[nodiscard]] bool isStatic() const override { return m_static; }
    [[nodiscard]] bool isOverload() const { return m_overloadFlag; }
    [[nodiscard]] bool isMarkedReimp() const override { return m_reimpFlag; }
    [[nodiscard]] bool isSomeCtor() const { return isCtor() || isCCtor() || isMCtor(); }
    [[nodiscard]] bool isMacroWithParams() const { return (m_metaness == MacroWithParams); }
    [[nodiscard]] bool isMacroWithoutParams() const { return (m_metaness == MacroWithoutParams); }
    [[nodiscard]] bool isMacro() const override
    {
        return (isMacroWithParams() || isMacroWithoutParams());
    }
    [[nodiscard]] bool isDeprecated() const override;

    [[nodiscard]] bool isCppFunction() const { return m_metaness == Plain; } // Is this correct?
    [[nodiscard]] bool isSignal() const { return (m_metaness == Signal); }
    [[nodiscard]] bool isSlot() const { return (m_metaness == Slot); }
    [[nodiscard]] bool isCtor() const { return (m_metaness == Ctor); }
    [[nodiscard]] bool isDtor() const { return (m_metaness == Dtor); }
    [[nodiscard]] bool isCCtor() const { return (m_metaness == CCtor); }
    [[nodiscard]] bool isMCtor() const { return (m_metaness == MCtor); }
    [[nodiscard]] bool isCAssign() const { return (m_metaness == CAssign); }
    [[nodiscard]] bool isMAssign() const { return (m_metaness == MAssign); }

    [[nodiscard]] bool isJsMethod() const { return (m_metaness == JsMethod); }
    [[nodiscard]] bool isJsSignal() const { return (m_metaness == JsSignal); }
    [[nodiscard]] bool isJsSignalHandler() const { return (m_metaness == JsSignalHandler); }

    [[nodiscard]] bool isQmlMethod() const { return (m_metaness == QmlMethod); }
    [[nodiscard]] bool isQmlSignal() const { return (m_metaness == QmlSignal); }
    [[nodiscard]] bool isQmlSignalHandler() const { return (m_metaness == QmlSignalHandler); }

    [[nodiscard]] bool isSpecialMemberFunction() const
    {
        return (isCtor() || isDtor() || isCCtor() || isMCtor() || isCAssign() || isMAssign());
    }
    [[nodiscard]] bool isNonvirtual() const { return (m_virtualness == NonVirtual); }
    [[nodiscard]] bool isVirtual() const { return (m_virtualness == NormalVirtual); }
    [[nodiscard]] bool isPureVirtual() const { return (m_virtualness == PureVirtual); }
    [[nodiscard]] bool returnsBool() const { return (m_returnType == QLatin1String("bool")); }

    Parameters &parameters() { return m_parameters; }
    [[nodiscard]] const Parameters &parameters() const { return m_parameters; }
    [[nodiscard]] bool isPrivateSignal() const { return m_parameters.isPrivateSignal(); }
    void setParameters(const QString &signature) { m_parameters.set(signature); }
    [[nodiscard]] QString signature(bool values, bool noReturnType,
                                    bool templateParams = false) const override;

    [[nodiscard]] const QString &overridesThis() const { return m_overridesThis; }
    [[nodiscard]] const NodeList &associatedProperties() const { return m_associatedProperties; }
    [[nodiscard]] bool hasAssociatedProperties() const { return !m_associatedProperties.isEmpty(); }
    [[nodiscard]] bool hasOneAssociatedProperty() const
    {
        return (m_associatedProperties.size() == 1);
    }
    [[nodiscard]] Node *firstAssociatedProperty() const { return m_associatedProperties[0]; }

    [[nodiscard]] QString element() const override { return parent()->name(); }
    [[nodiscard]] bool isAttached() const override { return m_attached; }
    [[nodiscard]] bool isQtQuickNode() const override { return parent()->isQtQuickNode(); }
    [[nodiscard]] QString qmlTypeName() const override { return parent()->qmlTypeName(); }
    [[nodiscard]] QString logicalModuleName() const override
    {
        return parent()->logicalModuleName();
    }
    [[nodiscard]] QString logicalModuleVersion() const override
    {
        return parent()->logicalModuleVersion();
    }
    [[nodiscard]] QString logicalModuleIdentifier() const override
    {
        return parent()->logicalModuleIdentifier();
    }

    void debug() const;

    void setFinal(bool b) { m_isFinal = b; }
    [[nodiscard]] bool isFinal() const { return m_isFinal; }

    void setOverride(bool b) { m_isOverride = b; }
    [[nodiscard]] bool isOverride() const { return m_isOverride; }

    void setRef(bool b) { m_isRef = b; }
    [[nodiscard]] bool isRef() const { return m_isRef; }

    void setRefRef(bool b) { m_isRefRef = b; }
    [[nodiscard]] bool isRefRef() const { return m_isRefRef; }

    void setInvokable(bool b) { m_isInvokable = b; }
    [[nodiscard]] bool isInvokable() const { return m_isInvokable; }

    [[nodiscard]] bool hasTag(const QString &tag) const override { return (m_tag == tag); }
    void setTag(const QString &tag) { m_tag = tag; }
    [[nodiscard]] const QString &tag() const { return m_tag; }
    bool compare(const Node *node, bool sameParent = true) const;
    [[nodiscard]] bool isIgnored() const;
    [[nodiscard]] bool hasOverloads() const;
    void setOverloadFlag() { m_overloadFlag = true; }
    void setOverloadNumber(signed short number);
    void appendOverload(FunctionNode *functionNode);
    void removeOverload(FunctionNode *functionNode);
    [[nodiscard]] signed short overloadNumber() const { return m_overloadNumber; }
    FunctionNode *nextOverload() { return m_nextOverload; }
    void setNextOverload(FunctionNode *functionNode) { m_nextOverload = functionNode; }
    FunctionNode *findPrimaryFunction();

private:
    void addAssociatedProperty(PropertyNode *property);

    friend class Aggregate;
    friend class PropertyNode;

    bool m_const : 1;
    bool m_default : 1;
    bool m_static : 1;
    bool m_reimpFlag : 1;
    bool m_attached : 1;
    bool m_overloadFlag : 1;
    bool m_isFinal : 1;
    bool m_isOverride : 1;
    bool m_isRef : 1;
    bool m_isRefRef : 1;
    bool m_isInvokable : 1;
    Metaness m_metaness {};
    Virtualness m_virtualness {};
    signed short m_overloadNumber {};
    FunctionNode *m_nextOverload { nullptr };
    QString m_returnType {};
    QStringList m_parentPath {};
    QString m_overridesThis {};
    QString m_tag {};
    NodeList m_associatedProperties {};
    Parameters m_parameters {};
};

QT_END_NAMESPACE

#endif // FUNCTIONNODE_H
