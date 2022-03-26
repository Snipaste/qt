/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqmlmetatype_p.h"

#include <private/qqmlmetatypedata_p.h>
#include <private/qqmltypemodule_p.h>
#include <private/qqmltype_p_p.h>
#include <private/qqmltypeloader_p.h>
#include <private/qqmlextensionplugin_p.h>
#include <private/qqmlvaluetype_p.h>
#include <private/qv4executablecompilationunit_p.h>

#if QT_CONFIG(qml_itemmodel)
#include <private/qqmlmodelindexvaluetype_p.h>
#endif

#include <QtCore/qcoreapplication.h>
#include <QtCore/qmutex.h>
#include <QtCore/qloggingcategory.h>

Q_DECLARE_LOGGING_CATEGORY(DBG_DISK_CACHE)
Q_LOGGING_CATEGORY(lcTypeRegistration, "qt.qml.typeregistration")

QT_BEGIN_NAMESPACE

CompositeMetaTypeIds CompositeMetaTypeIds::fromCompositeName(const QByteArray &name)
{
    auto ids = QQmlMetaType::registerInternalCompositeType(name);
    ids.refCount = new int;
    *ids.refCount = 1;
    return ids;
}

void CompositeMetaTypeIds::deref()
{
    Q_ASSERT(refCount);
    --*refCount;
    if (!*refCount) {
        delete refCount;
        QQmlMetaType::unregisterInternalCompositeType(*this);
        refCount = nullptr;
    }
}

CompositeMetaTypeIds::~CompositeMetaTypeIds()
{
    if (refCount)
        deref();
}


struct LockedData : private QQmlMetaTypeData
{
    friend class QQmlMetaTypeDataPtr;
};

Q_GLOBAL_STATIC(LockedData, metaTypeData)
Q_GLOBAL_STATIC(QRecursiveMutex, metaTypeDataLock)

struct ModuleUri : public QString
{
    ModuleUri(const QString &string) : QString(string) {}
    ModuleUri(const std::unique_ptr<QQmlTypeModule> &module) : QString(module->module()) {}
};

class QQmlMetaTypeDataPtr
{
    Q_DISABLE_COPY_MOVE(QQmlMetaTypeDataPtr)
public:
    QQmlMetaTypeDataPtr() : locker(metaTypeDataLock()), data(metaTypeData()) {}
    ~QQmlMetaTypeDataPtr() = default;

    QQmlMetaTypeData &operator*() { return *data; }
    QQmlMetaTypeData *operator->() { return data; }
    operator QQmlMetaTypeData *() { return data; }

    const QQmlMetaTypeData &operator*() const { return *data; }
    const QQmlMetaTypeData *operator->() const { return data; }
    operator const QQmlMetaTypeData *() const { return data; }

    bool isValid() const { return data != nullptr; }

private:
    QMutexLocker<QRecursiveMutex> locker;
    LockedData *data = nullptr;
};

static QQmlTypePrivate *createQQmlType(QQmlMetaTypeData *data,
                                       const QQmlPrivate::RegisterInterface &type)
{
    auto *d = new QQmlTypePrivate(QQmlType::InterfaceType);
    d->iid = type.iid;
    d->typeId = type.typeId;
    d->listId = type.listId;
    d->isSetup.storeRelease(true);
    d->module = QString::fromUtf8(type.uri);
    d->version = type.version;
    data->registerType(d);
    return d;
}

static QQmlTypePrivate *createQQmlType(QQmlMetaTypeData *data, const QString &elementName,
                                       const QQmlPrivate::RegisterSingletonType &type)
{
    auto *d = new QQmlTypePrivate(QQmlType::SingletonType);
    data->registerType(d);

    d->setName(QString::fromUtf8(type.uri), elementName);
    d->version = type.version;

    if (type.qObjectApi) {
        d->baseMetaObject = type.instanceMetaObject;
        d->typeId = type.typeId;
        d->revision = type.revision;
    }

    d->extraData.sd->singletonInstanceInfo = new QQmlType::SingletonInstanceInfo;
    d->extraData.sd->singletonInstanceInfo->scriptCallback = type.scriptApi;
    d->extraData.sd->singletonInstanceInfo->qobjectCallback = type.qObjectApi;
    d->extraData.sd->singletonInstanceInfo->typeName = QString::fromUtf8(type.typeName);
    d->extraData.sd->singletonInstanceInfo->instanceMetaObject
            = type.qObjectApi ? type.instanceMetaObject : nullptr;
    d->extraData.sd->extFunc = type.extensionObjectCreate;
    d->extraData.sd->extMetaObject = type.extensionMetaObject;

    return d;
}

static QQmlTypePrivate *createQQmlType(QQmlMetaTypeData *data, const QString &elementName,
                                       const QQmlPrivate::RegisterType &type)
{
    QQmlTypePrivate *d = new QQmlTypePrivate(QQmlType::CppType);
    data->registerType(d);
    d->setName(QString::fromUtf8(type.uri), elementName);

    d->version = type.version;
    d->revision = type.revision;
    d->typeId = type.typeId;
    d->listId = type.listId;
    d->extraData.cd->allocationSize = type.objectSize;
    d->extraData.cd->userdata = type.userdata;
    d->extraData.cd->newFunc = type.create;
    d->extraData.cd->noCreationReason = type.noCreationReason;
    d->extraData.cd->createValueTypeFunc = type.createValueType;
    d->baseMetaObject = type.metaObject;
    d->extraData.cd->attachedPropertiesFunc = type.attachedPropertiesFunction;
    d->extraData.cd->attachedPropertiesType = type.attachedPropertiesMetaObject;
    d->extraData.cd->parserStatusCast = type.parserStatusCast;
    d->extraData.cd->propertyValueSourceCast = type.valueSourceCast;
    d->extraData.cd->propertyValueInterceptorCast = type.valueInterceptorCast;
    d->extraData.cd->extFunc = type.extensionObjectCreate;
    d->extraData.cd->customParser = reinterpret_cast<QQmlCustomParser *>(type.customParser);
    d->extraData.cd->registerEnumClassesUnscoped = true;

    if (type.extensionMetaObject)
        d->extraData.cd->extMetaObject = type.extensionMetaObject;

    // Check if the user wants only scoped enum classes
    if (d->baseMetaObject) {
        auto indexOfClassInfo = d->baseMetaObject->indexOfClassInfo("RegisterEnumClassesUnscoped");
        if (indexOfClassInfo != -1 && QString::fromUtf8(d->baseMetaObject->classInfo(indexOfClassInfo).value()) == QLatin1String("false"))
            d->extraData.cd->registerEnumClassesUnscoped = false;
    }

    return d;
}

static QQmlTypePrivate *createQQmlType(QQmlMetaTypeData *data, const QString &elementName,
                                       const QQmlPrivate::RegisterCompositeType &type)
{
    auto *d = new QQmlTypePrivate(QQmlType::CompositeType);
    data->registerType(d);
    d->setName(QString::fromUtf8(type.uri), elementName);
    d->version = type.version;

    d->extraData.fd->url = QQmlTypeLoader::normalize(type.url);
    return d;
}

static QQmlTypePrivate *createQQmlType(QQmlMetaTypeData *data, const QString &elementName,
                                       const QQmlPrivate::RegisterCompositeSingletonType &type)
{
    auto *d = new QQmlTypePrivate(QQmlType::CompositeSingletonType);
    data->registerType(d);
    d->setName(QString::fromUtf8(type.uri), elementName);

    d->version = type.version;

    d->extraData.sd->singletonInstanceInfo = new QQmlType::SingletonInstanceInfo;
    d->extraData.sd->singletonInstanceInfo->url = QQmlTypeLoader::normalize(type.url);
    d->extraData.sd->singletonInstanceInfo->typeName = QString::fromUtf8(type.typeName);
    return d;
}

void QQmlMetaType::clone(QMetaObjectBuilder &builder, const QMetaObject *mo,
                         const QMetaObject *ignoreStart, const QMetaObject *ignoreEnd)
{
    // Set classname
    builder.setClassName(ignoreEnd->className());

    // Clone Q_CLASSINFO
    for (int ii = mo->classInfoOffset(); ii < mo->classInfoCount(); ++ii) {
        QMetaClassInfo info = mo->classInfo(ii);

        int otherIndex = ignoreEnd->indexOfClassInfo(info.name());
        if (otherIndex >= ignoreStart->classInfoOffset() + ignoreStart->classInfoCount()) {
            // Skip
        } else {
            builder.addClassInfo(info.name(), info.value());
        }
    }

    // Clone Q_PROPERTY
    for (int ii = mo->propertyOffset(); ii < mo->propertyCount(); ++ii) {
        QMetaProperty property = mo->property(ii);

        int otherIndex = ignoreEnd->indexOfProperty(property.name());
        if (otherIndex >= ignoreStart->propertyOffset() + ignoreStart->propertyCount()) {
            builder.addProperty(QByteArray("__qml_ignore__") + property.name(), QByteArray("void"));
            // Skip
        } else {
            builder.addProperty(property);
        }
    }

    // Clone Q_METHODS
    for (int ii = mo->methodOffset(); ii < mo->methodCount(); ++ii) {
        QMetaMethod method = mo->method(ii);

        // More complex - need to search name
        QByteArray name = method.name();


        bool found = false;

        for (int ii = ignoreStart->methodOffset() + ignoreStart->methodCount();
             !found && ii < ignoreEnd->methodOffset() + ignoreEnd->methodCount();
             ++ii) {

            QMetaMethod other = ignoreEnd->method(ii);

            found = name == other.name();
        }

        QMetaMethodBuilder m = builder.addMethod(method);
        if (found) // SKIP
            m.setAccess(QMetaMethod::Private);
    }

    // Clone Q_ENUMS
    for (int ii = mo->enumeratorOffset(); ii < mo->enumeratorCount(); ++ii) {
        QMetaEnum enumerator = mo->enumerator(ii);

        int otherIndex = ignoreEnd->indexOfEnumerator(enumerator.name());
        if (otherIndex >= ignoreStart->enumeratorOffset() + ignoreStart->enumeratorCount()) {
            // Skip
        } else {
            builder.addEnumerator(enumerator);
        }
    }
}

void QQmlMetaType::qmlInsertModuleRegistration(const QString &uri, void (*registerFunction)())
{
    QQmlMetaTypeDataPtr data;
    if (data->moduleTypeRegistrationFunctions.contains(uri))
        qFatal("Cannot add multiple registrations for %s", qPrintable(uri));
    else
        data->moduleTypeRegistrationFunctions.insert(uri, registerFunction);
}

void QQmlMetaType::qmlRemoveModuleRegistration(const QString &uri)
{
    QQmlMetaTypeDataPtr data;

    if (!data.isValid())
        return; // shutdown/deletion race. Not a problem.

    if (!data->moduleTypeRegistrationFunctions.contains(uri))
        qFatal("Cannot remove multiple registrations for %s", qPrintable(uri));
    else
        data->moduleTypeRegistrationFunctions.remove(uri);
}

bool QQmlMetaType::qmlRegisterModuleTypes(const QString &uri)
{
    QQmlMetaTypeDataPtr data;
    return data->registerModuleTypes(uri);
}

void QQmlMetaType::clearTypeRegistrations()
{
    //Only cleans global static, assumed no running engine
    QQmlMetaTypeDataPtr data;

    data->uriToModule.clear();
    data->types.clear();
    data->idToType.clear();
    data->nameToType.clear();
    data->urlToType.clear();
    data->typePropertyCaches.clear();
    data->urlToNonFileImportType.clear();
    data->metaObjectToType.clear();
    data->undeletableTypes.clear();
}

int QQmlMetaType::registerAutoParentFunction(const QQmlPrivate::RegisterAutoParent &function)
{
    if (function.structVersion > 0)
        qFatal("qmlRegisterType(): Cannot mix incompatible QML versions.");

    QQmlMetaTypeDataPtr data;

    data->parentFunctions.append(function.function);

    return data->parentFunctions.count() - 1;
}

void QQmlMetaType::unregisterAutoParentFunction(const QQmlPrivate::AutoParentFunction &function)
{
    QQmlMetaTypeDataPtr data;
    data->parentFunctions.removeOne(function);
}

QQmlType QQmlMetaType::registerInterface(const QQmlPrivate::RegisterInterface &type)
{
    if (type.structVersion > 0)
        qFatal("qmlRegisterType(): Cannot mix incompatible QML versions.");

    QQmlMetaTypeDataPtr data;
    QQmlTypePrivate *priv = createQQmlType(data, type);
    Q_ASSERT(priv);


    data->idToType.insert(priv->typeId.id(), priv);
    data->idToType.insert(priv->listId.id(), priv);

    data->interfaces.insert(type.typeId.id());

    return QQmlType(priv);
}

QString registrationTypeString(QQmlType::RegistrationType typeType)
{
    QString typeStr;
    if (typeType == QQmlType::CppType)
        typeStr = QStringLiteral("element");
    else if (typeType == QQmlType::SingletonType)
        typeStr = QStringLiteral("singleton type");
    else if (typeType == QQmlType::CompositeSingletonType)
        typeStr = QStringLiteral("composite singleton type");
    else if (typeType == QQmlType::SequentialContainerType)
        typeStr = QStringLiteral("sequential container type");
    else
        typeStr = QStringLiteral("type");
    return typeStr;
}

// NOTE: caller must hold a QMutexLocker on "data"
bool checkRegistration(QQmlType::RegistrationType typeType, QQmlMetaTypeData *data,
                       const char *uri, const QString &typeName, QTypeRevision version,
                       QMetaType::TypeFlags flags)
{
    if (!typeName.isEmpty()) {
        if (typeName.at(0).isLower() && (flags & QMetaType::PointerToQObject)) {
            QString failure(QCoreApplication::translate("qmlRegisterType", "Invalid QML %1 name \"%2\"; type names must begin with an uppercase letter"));
            data->recordTypeRegFailure(failure.arg(registrationTypeString(typeType)).arg(typeName));
            return false;
        }

        if (typeName.at(0).isUpper()
                && (flags & (QMetaType::IsGadget | QMetaType::PointerToGadget))) {
            qCWarning(lcTypeRegistration).noquote()
                    << QCoreApplication::translate(
                           "qmlRegisterType",
                           "Invalid QML %1 name \"%2\"; "
                           "value type names should begin with a lowercase letter")
                       .arg(registrationTypeString(typeType)).arg(typeName);
        }

        // There can also be types that aren't even gadgets, and there can be types for namespaces.
        // We cannot check those, but namespaces should be uppercase.

        int typeNameLen = typeName.length();
        for (int ii = 0; ii < typeNameLen; ++ii) {
            if (!(typeName.at(ii).isLetterOrNumber() || typeName.at(ii) == u'_')) {
                QString failure(QCoreApplication::translate("qmlRegisterType", "Invalid QML %1 name \"%2\""));
                data->recordTypeRegFailure(failure.arg(registrationTypeString(typeType)).arg(typeName));
                return false;
            }
        }
    }

    if (uri && !typeName.isEmpty()) {
        QString nameSpace = QString::fromUtf8(uri);
        QQmlTypeModule *qqtm = data->findTypeModule(nameSpace, version);
        if (qqtm && qqtm->lockLevel() != QQmlTypeModule::LockLevel::Open) {
            QString failure(QCoreApplication::translate(
                                "qmlRegisterType",
                                "Cannot install %1 '%2' into protected module '%3' version '%4'"));
            data->recordTypeRegFailure(failure.arg(registrationTypeString(typeType))
                                       .arg(typeName).arg(nameSpace)
                                       .arg(version.majorVersion()));
            return false;
        }
    }

    return true;
}

// NOTE: caller must hold a QMutexLocker on "data"
QQmlTypeModule *getTypeModule(const QHashedString &uri, QTypeRevision version, QQmlMetaTypeData *data)
{
    if (QQmlTypeModule *module = data->findTypeModule(uri, version))
        return module;
    return data->addTypeModule(std::make_unique<QQmlTypeModule>(uri, version.majorVersion()));
}

// NOTE: caller must hold a QMutexLocker on "data"
void addTypeToData(QQmlTypePrivate *type, QQmlMetaTypeData *data)
{
    Q_ASSERT(type);

    if (!type->elementName.isEmpty())
        data->nameToType.insert(type->elementName, type);

    if (type->baseMetaObject)
        data->metaObjectToType.insert(type->baseMetaObject, type);

    if (type->typeId.isValid()) {
        data->idToType.insert(type->typeId.id(), type);
    }

    if (type->listId.isValid()) {
        data->idToType.insert(type->listId.id(), type);
    }

    if (!type->module.isEmpty()) {
        const QHashedString &mod = type->module;

        QQmlTypeModule *module = getTypeModule(mod, type->version, data);
        Q_ASSERT(module);
        module->add(type);
    }
}

QQmlType QQmlMetaType::registerType(const QQmlPrivate::RegisterType &type)
{
    if (type.structVersion > 0)
        qFatal("qmlRegisterType(): Cannot mix incompatible QML versions.");

    QQmlMetaTypeDataPtr data;

    QString elementName = QString::fromUtf8(type.elementName);
    if (!checkRegistration(QQmlType::CppType, data, type.uri, elementName, type.version,
                           QMetaType(type.typeId).flags())) {
        return QQmlType();
    }

    QQmlTypePrivate *priv = createQQmlType(data, elementName, type);
    addTypeToData(priv, data);

    return QQmlType(priv);
}

QQmlType QQmlMetaType::registerSingletonType(const QQmlPrivate::RegisterSingletonType &type)
{
    if (type.structVersion > 0)
        qFatal("qmlRegisterType(): Cannot mix incompatible QML versions.");

    QQmlMetaTypeDataPtr data;

    QString typeName = QString::fromUtf8(type.typeName);
    if (!checkRegistration(QQmlType::SingletonType, data, type.uri, typeName, type.version,
                           QMetaType(type.typeId).flags())) {
        return QQmlType();
    }

    QQmlTypePrivate *priv = createQQmlType(data, typeName, type);

    addTypeToData(priv, data);

    return QQmlType(priv);
}

QQmlType QQmlMetaType::registerCompositeSingletonType(const QQmlPrivate::RegisterCompositeSingletonType &type)
{
    if (type.structVersion > 0)
        qFatal("qmlRegisterType(): Cannot mix incompatible QML versions.");

    // Assumes URL is absolute and valid. Checking of user input should happen before the URL enters type.
    QQmlMetaTypeDataPtr data;

    QString typeName = QString::fromUtf8(type.typeName);
    bool fileImport = false;
    if (*(type.uri) == '\0')
        fileImport = true;
    if (!checkRegistration(QQmlType::CompositeSingletonType, data, fileImport ? nullptr : type.uri,
                           typeName, type.version, {})) {
        return QQmlType();
    }

    QQmlTypePrivate *priv = createQQmlType(data, typeName, type);
    addTypeToData(priv, data);

    QQmlMetaTypeData::Files *files = fileImport ? &(data->urlToType) : &(data->urlToNonFileImportType);
    files->insert(QQmlTypeLoader::normalize(type.url), priv);

    return QQmlType(priv);
}

QQmlType QQmlMetaType::registerCompositeType(const QQmlPrivate::RegisterCompositeType &type)
{
    if (type.structVersion > 0)
        qFatal("qmlRegisterType(): Cannot mix incompatible QML versions.");

    // Assumes URL is absolute and valid. Checking of user input should happen before the URL enters type.
    QQmlMetaTypeDataPtr data;

    QString typeName = QString::fromUtf8(type.typeName);
    bool fileImport = false;
    if (*(type.uri) == '\0')
        fileImport = true;
    if (!checkRegistration(QQmlType::CompositeType, data, fileImport?nullptr:type.uri, typeName,
                           type.version, {})) {
        return QQmlType();
    }

    QQmlTypePrivate *priv = createQQmlType(data, typeName, type);
    addTypeToData(priv, data);

    QQmlMetaTypeData::Files *files = fileImport ? &(data->urlToType) : &(data->urlToNonFileImportType);
    files->insert(QQmlTypeLoader::normalize(type.url), priv);

    return QQmlType(priv);
}

CompositeMetaTypeIds QQmlMetaType::registerInternalCompositeType(const QByteArray &className)
{
    QByteArray ptr = className + '*';
    QByteArray lst = "QQmlListProperty<" + className + '>';

    QMetaType ptr_type(new QQmlMetaTypeInterface(ptr, (QObject **)nullptr));
    QMetaType lst_type(new QQmlListMetaTypeInterface(lst, (QQmlListProperty<QObject> *)nullptr, ptr_type.iface()));

    return {ptr_type, lst_type};
}

void QQmlMetaType::unregisterInternalCompositeType(const CompositeMetaTypeIds &typeIds)
{
    QMetaType metaType(typeIds.id);
    QMetaType listMetaType(typeIds.listId);

    // This may be called from delayed dtors on shutdown when the data is already gone.
    QQmlMetaTypeDataPtr data;
    if (data.isValid()) {
        if (QQmlValueType *vt = data->metaTypeToValueType.take(metaType.id()))
            delete vt;
        if (QQmlValueType *vt = data->metaTypeToValueType.take(listMetaType.id()))
            delete vt;
    }

    QMetaType::unregisterMetaType(metaType);
    QMetaType::unregisterMetaType(listMetaType);
    delete static_cast<const QQmlMetaTypeInterface *>(metaType.iface());
    delete static_cast<const QQmlListMetaTypeInterface *>(listMetaType.iface());
}

int QQmlMetaType::registerUnitCacheHook(
        const QQmlPrivate::RegisterQmlUnitCacheHook &hookRegistration)
{
    if (hookRegistration.structVersion > 0)
        qFatal("qmlRegisterType(): Cannot mix incompatible QML versions.");

    QQmlMetaTypeDataPtr data;
    data->lookupCachedQmlUnit << hookRegistration.lookupCachedQmlUnit;
    return 0;
}

QQmlType QQmlMetaType::registerSequentialContainer(
        const QQmlPrivate::RegisterSequentialContainer &container)
{
    if (container.structVersion > 0)
        qFatal("qmlRegisterSequenceContainer(): Cannot mix incompatible QML versions.");

    QQmlMetaTypeDataPtr data;

    const QString typeName = QString::fromUtf8(container.typeName);
    if (!checkRegistration(QQmlType::SequentialContainerType, data, container.uri, typeName,
                           container.version, {})) {
        return QQmlType();
    }

    QQmlTypePrivate *priv = new QQmlTypePrivate(QQmlType::SequentialContainerType);

    data->registerType(priv);
    priv->setName(QString::fromUtf8(container.uri), typeName);
    priv->version = container.version;
    priv->revision = container.revision;
    priv->typeId = container.typeId;
    *priv->extraData.ld = container.metaSequence;

    addTypeToData(priv, data);

    return QQmlType(priv);
}

void QQmlMetaType::unregisterSequentialContainer(int id)
{
    unregisterType(id);
}

bool QQmlMetaType::protectModule(const QString &uri, QTypeRevision version,
                                 bool weakProtectAllVersions)
{
    QQmlMetaTypeDataPtr data;
    if (version.hasMajorVersion()) {
        if (QQmlTypeModule *module = data->findTypeModule(uri, version)) {
            if (!weakProtectAllVersions) {
                module->setLockLevel(QQmlTypeModule::LockLevel::Strong);
                return true;
            }
        } else {
            return false;
        }
    }

    const auto range = std::equal_range(
                data->uriToModule.begin(), data->uriToModule.end(), uri,
                std::less<ModuleUri>());

    for (auto it = range.first; it != range.second; ++it)
        (*it)->setLockLevel(QQmlTypeModule::LockLevel::Weak);

    return range.first != range.second;
}

void QQmlMetaType::registerModuleImport(const QString &uri, QTypeRevision moduleVersion,
                                        const QQmlDirParser::Import &import)
{
    QQmlMetaTypeDataPtr data;

    data->moduleImports.insert(QQmlMetaTypeData::VersionedUri(uri, moduleVersion), import);
}

static bool operator==(const QQmlDirParser::Import &a, const QQmlDirParser::Import &b)
{
    return a.module == b.module && a.version == b.version && a.flags == b.flags;
}

void QQmlMetaType::unregisterModuleImport(const QString &uri, QTypeRevision moduleVersion,
                                          const QQmlDirParser::Import &import)
{
    QQmlMetaTypeDataPtr data;
    data->moduleImports.remove(QQmlMetaTypeData::VersionedUri(uri, moduleVersion), import);
}

QList<QQmlDirParser::Import> QQmlMetaType::moduleImports(
        const QString &uri, QTypeRevision version)
{
    QQmlMetaTypeDataPtr data;

    const auto unrevisioned = data->moduleImports.equal_range(
                QQmlMetaTypeData::VersionedUri(uri, QTypeRevision()));

    QList<QQmlDirParser::Import> result(unrevisioned.first, unrevisioned.second);
    if (version.hasMajorVersion())
        return result + data->moduleImports.values(QQmlMetaTypeData::VersionedUri(uri, version));

    // Use latest module available with that URI.
    const auto begin = data->moduleImports.begin();
    auto it = unrevisioned.first;
    if (it == begin)
        return result;

    const QQmlMetaTypeData::VersionedUri latestVersion = (--it).key();
    if (latestVersion.uri != uri)
        return result;

    do {
        result += *it;
    } while (it != begin && (--it).key() == latestVersion);

    return result;
}

void QQmlMetaType::registerModule(const char *uri, QTypeRevision version)
{
    QQmlMetaTypeDataPtr data;

    QQmlTypeModule *module = getTypeModule(QString::fromUtf8(uri), version, data);
    Q_ASSERT(module);

    if (version.hasMinorVersion())
        module->addMinorVersion(version.minorVersion());
}

int QQmlMetaType::typeId(const char *uri, QTypeRevision version, const char *qmlName)
{
    QQmlMetaTypeDataPtr data;

    QQmlTypeModule *module = getTypeModule(QString::fromUtf8(uri), version, data);
    if (!module)
        return -1;

    QQmlType type = module->type(QHashedStringRef(QString::fromUtf8(qmlName)), version);
    if (!type.isValid())
        return -1;

    return type.index();
}

void QQmlMetaType::registerUndeletableType(const QQmlType &dtype)
{
    QQmlMetaTypeDataPtr data;
    data->undeletableTypes.insert(dtype);
}

static bool namespaceContainsRegistrations(const QQmlMetaTypeData *data, const QString &uri,
                                           QTypeRevision version)
{
    // Has any type previously been installed to this namespace?
    QHashedString nameSpace(uri);
    for (const QQmlType &type : data->types) {
        if (type.module() == nameSpace && type.version().majorVersion() == version.majorVersion())
            return true;
    }

    return false;
}

class QQmlMetaTypeRegistrationFailureRecorder
{
    Q_DISABLE_COPY_MOVE(QQmlMetaTypeRegistrationFailureRecorder)
public:
    QQmlMetaTypeRegistrationFailureRecorder(QQmlMetaTypeData *data, QStringList *failures)
        : data(data)
    {
        data->setTypeRegistrationFailures(failures);
    }

    ~QQmlMetaTypeRegistrationFailureRecorder()
    {
        data->setTypeRegistrationFailures(nullptr);
    }

    QQmlMetaTypeData *data = nullptr;
};


QQmlMetaType::RegistrationResult QQmlMetaType::registerPluginTypes(
        QObject *instance, const QString &basePath, const QString &uri,
        const QString &typeNamespace, QTypeRevision version, QList<QQmlError> *errors)
{
    if (!typeNamespace.isEmpty() && typeNamespace != uri) {
        // This is an 'identified' module
        // The namespace for type registrations must match the URI for locating the module
        if (errors) {
            QQmlError error;
            error.setDescription(
                    QStringLiteral("Module namespace '%1' does not match import URI '%2'")
                            .arg(typeNamespace).arg(uri));
            errors->prepend(error);
        }
        return RegistrationResult::Failure;
    }

    QStringList failures;
    QQmlMetaTypeDataPtr data;
    {
        QQmlMetaTypeRegistrationFailureRecorder failureRecorder(data, &failures);
        if (!typeNamespace.isEmpty()) {
            // This is an 'identified' module
            if (namespaceContainsRegistrations(data, typeNamespace, version)) {
                // Other modules have already installed to this namespace
                if (errors) {
                    QQmlError error;
                    error.setDescription(QStringLiteral("Namespace '%1' has already been used "
                                                        "for type registration")
                                                 .arg(typeNamespace));
                    errors->prepend(error);
                }
                return RegistrationResult::Failure;
            }
        } else {
            // This is not an identified module - provide a warning
            qWarning().nospace() << qPrintable(
                    QStringLiteral("Module '%1' does not contain a module identifier directive - "
                                   "it cannot be protected from external registrations.").arg(uri));
        }

        if (instance && !qobject_cast<QQmlEngineExtensionInterface *>(instance)) {
            QQmlTypesExtensionInterface *iface = qobject_cast<QQmlTypesExtensionInterface *>(instance);
            if (!iface) {
                if (errors) {
                    QQmlError error;
                    // Also does not implement QQmlTypesExtensionInterface, but we want to discourage that.
                    error.setDescription(QStringLiteral("Module loaded for URI '%1' does not implement "
                                                        "QQmlEngineExtensionInterface").arg(typeNamespace));
                    errors->prepend(error);
                }
                return RegistrationResult::Failure;
            }

            if (auto *plugin = qobject_cast<QQmlExtensionPlugin *>(instance)) {
                // basepath should point to the directory of the module, not the plugin file itself:
                QQmlExtensionPluginPrivate::get(plugin)->baseUrl
                        = QQmlImports::urlFromLocalFileOrQrcOrUrl(basePath);
            }

            const QByteArray bytes = uri.toUtf8();
            const char *moduleId = bytes.constData();
            iface->registerTypes(moduleId);
        }

        if (failures.isEmpty() && !data->registerModuleTypes(uri))
            return RegistrationResult::NoRegistrationFunction;

        if (!failures.isEmpty()) {
            if (errors) {
                for (const QString &failure : qAsConst(failures)) {
                    QQmlError error;
                    error.setDescription(failure);
                    errors->prepend(error);
                }
            }
            return RegistrationResult::Failure;
        }
    }

    return RegistrationResult::Success;
}

/*
    \internal

    Fetches the QQmlType instance registered for \a urlString, creating a
    registration for it if it is not already registered, using the associated
    \a typeName, \a isCompositeSingleton, \a majorVersion and \a minorVersion
    details.

    Errors (if there are any) are placed into \a errors, if it is nonzero.
    Otherwise errors are printed as warnings.
*/
QQmlType QQmlMetaType::typeForUrl(const QString &urlString,
                                  const QHashedStringRef &qualifiedType,
                                  bool isCompositeSingleton, QList<QQmlError> *errors,
                                  QTypeRevision version)
{
    // ### unfortunate (costly) conversion
    const QUrl url = QQmlTypeLoader::normalize(QUrl(urlString));

    QQmlMetaTypeDataPtr data;
    {
        QQmlType ret(data->urlToType.value(url));
        if (ret.isValid() && ret.sourceUrl() == url)
            return ret;
    }
    {
        QQmlType ret(data->urlToNonFileImportType.value(url));
        if (ret.isValid() && ret.sourceUrl() == url)
            return ret;
    }

    const int dot = qualifiedType.indexOf(QLatin1Char('.'));
    const QString typeName = dot < 0
            ? qualifiedType.toString()
            : QString(qualifiedType.constData() + dot + 1, qualifiedType.length() - dot - 1);

    QStringList failures;
    QQmlMetaTypeRegistrationFailureRecorder failureRecorder(data, &failures);

    // Register the type. Note that the URI parameters here are empty; for
    // file type imports, we do not place them in a URI as we don't
    // necessarily have a good and unique one (picture a library import,
    // which may be found in multiple plugin locations on disk), but there
    // are other reasons for this too.
    //
    // By not putting them in a URI, we prevent the types from being
    // registered on a QQmlTypeModule; this is important, as once types are
    // placed on there, they cannot be easily removed, meaning if the
    // developer subsequently loads a different import (meaning different
    // types) with the same URI (using, say, a different plugin path), it is
    // very undesirable that we continue to associate the types from the
    // "old" URI with that new module.
    //
    // Not having URIs also means that the types cannot be found by name
    // etc, the only way to look them up is through QQmlImports -- for
    // better or worse.
    const QQmlType::RegistrationType registrationType = isCompositeSingleton
            ? QQmlType::CompositeSingletonType
            : QQmlType::CompositeType;
    if (checkRegistration(registrationType, data, nullptr, typeName, version, {})) {
        auto *priv = new QQmlTypePrivate(registrationType);
        priv->setName(QString(), typeName);
        priv->version = version;

        if (isCompositeSingleton) {
            priv->extraData.sd->singletonInstanceInfo = new QQmlType::SingletonInstanceInfo;
            priv->extraData.sd->singletonInstanceInfo->url = url;
            priv->extraData.sd->singletonInstanceInfo->typeName = typeName;
        } else {
            priv->extraData.fd->url = url;
        }

        data->registerType(priv);
        addTypeToData(priv, data);
        data->urlToType.insert(url, priv);
        return QQmlType(priv);
    }

    // This means that the type couldn't be found by URL, but could not be
    // registered either, meaning we most likely were passed some kind of bad
    // data.
    if (errors) {
        QQmlError error;
        error.setDescription(failures.join(u'\n'));
        errors->prepend(error);
    } else {
        qWarning("%s", failures.join(u'\n').toLatin1().constData());
    }
    return QQmlType();
}

QRecursiveMutex *QQmlMetaType::typeRegistrationLock()
{
    return metaTypeDataLock();
}

/*
    Returns the latest version of \a uri installed, or an in valid QTypeRevision().
*/
QTypeRevision QQmlMetaType::latestModuleVersion(const QString &uri)
{
    QQmlMetaTypeDataPtr data;
    auto upper = std::upper_bound(data->uriToModule.begin(), data->uriToModule.end(), uri,
                                  std::less<ModuleUri>());
    if (upper == data->uriToModule.begin())
        return QTypeRevision();

    const auto module = (--upper)->get();
    return (module->module() == uri)
            ? QTypeRevision::fromVersion(module->majorVersion(), module->maximumMinorVersion())
            : QTypeRevision();
}

/*
    Returns true if a module \a uri of this version is installed and locked;
*/
bool QQmlMetaType::isStronglyLockedModule(const QString &uri, QTypeRevision version)
{
    QQmlMetaTypeDataPtr data;

    if (QQmlTypeModule* qqtm = data->findTypeModule(uri, version))
        return qqtm->lockLevel() == QQmlTypeModule::LockLevel::Strong;
    return false;
}

/*
    Returns the best matching registered version for the given \a module. If \a version is
    the does not have a major version, returns the latest registered version. Otherwise
    chooses the same major version and checks if the minor version is within the range
    of registered minor versions. If so, retuens the original version, otherwise returns
    an invalid version. If \a version does not have a minor version, chooses the latest one.
*/
QTypeRevision QQmlMetaType::matchingModuleVersion(const QString &module, QTypeRevision version)
{
    if (!version.hasMajorVersion())
        return latestModuleVersion(module);

    QQmlMetaTypeDataPtr data;

    // first, check Types
    if (QQmlTypeModule *tm = data->findTypeModule(module, version)) {
        if (!version.hasMinorVersion())
            return QTypeRevision::fromVersion(version.majorVersion(), tm->maximumMinorVersion());

        if (tm->minimumMinorVersion() <= version.minorVersion()
                && tm->maximumMinorVersion() >= version.minorVersion()) {
            return version;
        }
    }

    return QTypeRevision();
}

QQmlTypeModule *QQmlMetaType::typeModule(const QString &uri, QTypeRevision version)
{
    QQmlMetaTypeDataPtr data;

    if (version.hasMajorVersion())
        return data->findTypeModule(uri, version);

    auto range = std::equal_range(data->uriToModule.begin(), data->uriToModule.end(),
                                  uri, std::less<ModuleUri>());

    return range.first == range.second ? nullptr : (--range.second)->get();
}

QList<QQmlPrivate::AutoParentFunction> QQmlMetaType::parentFunctions()
{
    QQmlMetaTypeDataPtr data;
    return data->parentFunctions;
}

QObject *QQmlMetaType::toQObject(const QVariant &v, bool *ok)
{
    if (!v.metaType().flags().testFlag(QMetaType::PointerToQObject)) {
        if (ok) *ok = false;
        return nullptr;
    }

    if (ok) *ok = true;

    return *(QObject *const *)v.constData();
}

/*
    Returns the item type for a list of type \a id.
 */
QMetaType QQmlMetaType::listType(QMetaType metaType)
{
    if (!isList(metaType))
        return QMetaType {};
    const auto iface = metaType.iface();
    if (iface->metaObjectFn == &dynamicQmlListMarker)
        return QMetaType(static_cast<const QQmlListMetaTypeInterface *>(iface)->valueType);
    auto id = metaType.id();
    QQmlMetaTypeDataPtr data;
    QQmlTypePrivate *type = data->idToType.value(id);
    if (type && type->listId.id() == id)
        return type->typeId;
    else
        return QMetaType {};
}

QQmlAttachedPropertiesFunc QQmlMetaType::attachedPropertiesFunc(QQmlEnginePrivate *engine,
                                                                const QMetaObject *mo)
{
    QQmlMetaTypeDataPtr data;

    QQmlType type(data->metaObjectToType.value(mo));
    return type.attachedPropertiesFunction(engine);
}

QMetaProperty QQmlMetaType::defaultProperty(const QMetaObject *metaObject)
{
    int idx = metaObject->indexOfClassInfo("DefaultProperty");
    if (-1 == idx)
        return QMetaProperty();

    QMetaClassInfo info = metaObject->classInfo(idx);
    if (!info.value())
        return QMetaProperty();

    idx = metaObject->indexOfProperty(info.value());
    if (-1 == idx)
        return QMetaProperty();

    return metaObject->property(idx);
}

QMetaProperty QQmlMetaType::defaultProperty(QObject *obj)
{
    if (!obj)
        return QMetaProperty();

    const QMetaObject *metaObject = obj->metaObject();
    return defaultProperty(metaObject);
}

QMetaMethod QQmlMetaType::defaultMethod(const QMetaObject *metaObject)
{
    int idx = metaObject->indexOfClassInfo("DefaultMethod");
    if (-1 == idx)
        return QMetaMethod();

    QMetaClassInfo info = metaObject->classInfo(idx);
    if (!info.value())
        return QMetaMethod();

    idx = metaObject->indexOfMethod(info.value());
    if (-1 == idx)
        return QMetaMethod();

    return metaObject->method(idx);
}

QMetaMethod QQmlMetaType::defaultMethod(QObject *obj)
{
    if (!obj)
        return QMetaMethod();

    const QMetaObject *metaObject = obj->metaObject();
    return defaultMethod(metaObject);
}

/*!
    See qmlRegisterInterface() for information about when this will return true.
*/
bool QQmlMetaType::isInterface(int userType)
{
    const QQmlMetaTypeDataPtr data;
    return data->interfaces.contains(userType);
}

const char *QQmlMetaType::interfaceIId(int userType)
{

    QQmlTypePrivate *typePrivate = nullptr;
    {
        QQmlMetaTypeDataPtr data;
        typePrivate = data->idToType.value(userType);
    }

    QQmlType type(typePrivate);
    if (type.isInterface() && type.typeId().id() == userType)
        return type.interfaceIId();
    else
        return nullptr;
}

bool QQmlMetaType::isList(QMetaType type)
{
    if (type.flags().testFlag(QMetaType::IsQmlList))
        return true;
    else
        return false;
}

/*!
    Returns the type (if any) of URI-qualified named \a qualifiedName and version specified
    by \a version_major and \a version_minor.
*/
QQmlType QQmlMetaType::qmlType(const QString &qualifiedName, QTypeRevision version)
{
    int slash = qualifiedName.indexOf(QLatin1Char('/'));
    if (slash <= 0)
        return QQmlType();

    QHashedStringRef module(qualifiedName.constData(), slash);
    QHashedStringRef name(qualifiedName.constData() + slash + 1, qualifiedName.length() - slash - 1);

    return qmlType(name, module, version);
}

/*!
    Returns the type (if any) of \a name in \a module and version specified
    by \a version_major and \a version_minor.
*/
QQmlType QQmlMetaType::qmlType(const QHashedStringRef &name, const QHashedStringRef &module,
                               QTypeRevision version)
{
    const QQmlMetaTypeDataPtr data;

    const QHashedString key(QString::fromRawData(name.constData(), name.length()), name.hash());
    QQmlMetaTypeData::Names::ConstIterator it = data->nameToType.constFind(key);
    while (it != data->nameToType.cend() && it.key() == name) {
        QQmlType t(*it);
        if (module.isEmpty() || t.availableInVersion(module, version))
            return t;
        ++it;
    }

    return QQmlType();
}

/*!
    Returns the type (if any) that corresponds to the \a metaObject.  Returns null if no
    type is registered.
*/
QQmlType QQmlMetaType::qmlType(const QMetaObject *metaObject)
{
    const QQmlMetaTypeDataPtr data;
    return QQmlType(data->metaObjectToType.value(metaObject));
}

/*!
    Returns the type (if any) that corresponds to the \a metaObject in version specified
    by \a version_major and \a version_minor in module specified by \a uri.  Returns null if no
    type is registered.
*/
QQmlType QQmlMetaType::qmlType(const QMetaObject *metaObject, const QHashedStringRef &module,
                               QTypeRevision version)
{
    const QQmlMetaTypeDataPtr data;

    QQmlMetaTypeData::MetaObjects::const_iterator it = data->metaObjectToType.constFind(metaObject);
    while (it != data->metaObjectToType.cend() && it.key() == metaObject) {
        QQmlType t(*it);
        if (module.isEmpty() || t.availableInVersion(module, version))
            return t;
        ++it;
    }

    return QQmlType();
}

/*!
    Returns the type (if any) that corresponds to \a typeId.  Depending on \a category, the
    \a typeId is interpreted either as QVariant::Type or as QML type id returned by one of the
    qml type registration functions.  Returns null if no type is registered.
*/
QQmlType QQmlMetaType::qmlType(int typeId, TypeIdCategory category)
{
    const QQmlMetaTypeDataPtr data;

    if (category == TypeIdCategory::MetaType) {
        QQmlTypePrivate *type = data->idToType.value(typeId);
        if (type && type->typeId.id() == typeId)
            return QQmlType(type);
    } else if (category == TypeIdCategory::QmlType) {
        QQmlType type = data->types.value(typeId);
        if (type.isValid())
            return type;
    }
    return QQmlType();
}

/*!
    Returns the type (if any) that corresponds to the given \a url in the set of
    composite types added through file imports.

    Returns null if no such type is registered.
*/
QQmlType QQmlMetaType::qmlType(const QUrl &unNormalizedUrl, bool includeNonFileImports /* = false */)
{
    const QUrl url = QQmlTypeLoader::normalize(unNormalizedUrl);
    const QQmlMetaTypeDataPtr data;

    QQmlType type(data->urlToType.value(url));
    if (!type.isValid() && includeNonFileImports)
        type = QQmlType(data->urlToNonFileImportType.value(url));

    if (type.sourceUrl() == url)
        return type;
    else
        return QQmlType();
}

QQmlPropertyCache *QQmlMetaType::propertyCache(const QMetaObject *metaObject, QTypeRevision version, bool doRef)
{
    QQmlMetaTypeDataPtr data; // not const: the cache is created on demand
    auto ret =  data->propertyCache(metaObject, version);
    if (doRef)
        return ret.take();
    else
        return ret.data();
}

QQmlPropertyCache *QQmlMetaType::propertyCache(const QQmlType &type, QTypeRevision version)
{
    QQmlMetaTypeDataPtr data; // not const: the cache is created on demand
    return data->propertyCache(type, version);
}

void QQmlMetaType::unregisterType(int typeIndex)
{
    QQmlMetaTypeDataPtr data;
    const QQmlType type = data->types.value(typeIndex);
    if (const QQmlTypePrivate *d = type.priv()) {
        removeQQmlTypePrivate(data->idToType, d);
        removeQQmlTypePrivate(data->nameToType, d);
        removeQQmlTypePrivate(data->urlToType, d);
        removeQQmlTypePrivate(data->urlToNonFileImportType, d);
        removeQQmlTypePrivate(data->metaObjectToType, d);
        for (auto & module : data->uriToModule)
            module->remove(d);
        data->clearPropertyCachesForVersion(typeIndex);
        data->types[typeIndex] = QQmlType();
        data->undeletableTypes.remove(type);
    }
}

static bool hasActiveInlineComponents(const QQmlTypePrivate *d)
{
    for (const QQmlType &ic : qAsConst(d->objectIdToICType)) {
        const QQmlTypePrivate *icPriv = ic.priv();
        if (icPriv && icPriv->count() > 1)
            return true;
    }
    return false;
}

void QQmlMetaType::freeUnusedTypesAndCaches()
{
    QQmlMetaTypeDataPtr data;

    // in case this is being called during program exit, `data` might be destructed already
    if (!data.isValid())
        return;

    bool deletedAtLeastOneType;
    do {
        deletedAtLeastOneType = false;
        QList<QQmlType>::Iterator it = data->types.begin();
        while (it != data->types.end()) {
            const QQmlTypePrivate *d = (*it).priv();
            if (d && d->count() == 1 && !hasActiveInlineComponents(d)) {
                deletedAtLeastOneType = true;

                removeQQmlTypePrivate(data->idToType, d);
                removeQQmlTypePrivate(data->nameToType, d);
                removeQQmlTypePrivate(data->urlToType, d);
                removeQQmlTypePrivate(data->urlToNonFileImportType, d);
                removeQQmlTypePrivate(data->metaObjectToType, d);

                for (auto &module : data->uriToModule)
                    module->remove(d);

                data->clearPropertyCachesForVersion(d->index);
                *it = QQmlType();
            } else {
                ++it;
            }
        }
    } while (deletedAtLeastOneType);

    bool deletedAtLeastOneCache;
    do {
        deletedAtLeastOneCache = false;
        QHash<const QMetaObject *, QQmlPropertyCache *>::Iterator it = data->propertyCaches.begin();
        while (it != data->propertyCaches.end()) {

            if ((*it)->count() == 1) {
                QQmlPropertyCache *pc = nullptr;
                qSwap(pc, *it);
                it = data->propertyCaches.erase(it);
                pc->release();
                deletedAtLeastOneCache = true;
            } else {
                ++it;
            }
        }
    } while (deletedAtLeastOneCache);
}

/*!
    Returns the list of registered QML type names.
*/
QList<QString> QQmlMetaType::qmlTypeNames()
{
    const QQmlMetaTypeDataPtr data;

    QList<QString> names;
    names.reserve(data->nameToType.count());
    QQmlMetaTypeData::Names::ConstIterator it = data->nameToType.cbegin();
    while (it != data->nameToType.cend()) {
        QQmlType t(*it);
        names += t.qmlTypeName();
        ++it;
    }

    return names;
}

/*!
    Returns the list of registered QML types.
*/
QList<QQmlType> QQmlMetaType::qmlTypes()
{
    const QQmlMetaTypeDataPtr data;

    QList<QQmlType> types;
    for (QQmlTypePrivate *t : data->nameToType)
        types.append(QQmlType(t));

    return types;
}

/*!
    Returns the list of all registered types.
*/
QList<QQmlType> QQmlMetaType::qmlAllTypes()
{
    const QQmlMetaTypeDataPtr data;
    return data->types;
}

/*!
    Returns the list of registered QML singleton types.
*/
QList<QQmlType> QQmlMetaType::qmlSingletonTypes()
{
    const QQmlMetaTypeDataPtr data;

    QList<QQmlType> retn;
    for (const auto t : qAsConst(data->nameToType)) {
        QQmlType type(t);
        if (type.isSingleton())
            retn.append(type);
    }
    return retn;
}

const QQmlPrivate::CachedQmlUnit *QQmlMetaType::findCachedCompilationUnit(const QUrl &uri, CachedUnitLookupError *status)
{
    const QQmlMetaTypeDataPtr data;

    for (const auto lookup : qAsConst(data->lookupCachedQmlUnit)) {
        if (const QQmlPrivate::CachedQmlUnit *unit = lookup(uri)) {
            QString error;
            if (!QV4::ExecutableCompilationUnit::verifyHeader(unit->qmlData, QDateTime(), &error)) {
                qCDebug(DBG_DISK_CACHE) << "Error loading pre-compiled file " << uri << ":" << error;
                if (status)
                    *status = CachedUnitLookupError::VersionMismatch;
                return nullptr;
            }
            if (status)
                *status = CachedUnitLookupError::NoError;
            return unit;
        }
    }

    if (status)
        *status = CachedUnitLookupError::NoUnitFound;

    return nullptr;
}

void QQmlMetaType::prependCachedUnitLookupFunction(QQmlPrivate::QmlUnitCacheLookupFunction handler)
{
    QQmlMetaTypeDataPtr data;
    data->lookupCachedQmlUnit.prepend(handler);
}

void QQmlMetaType::removeCachedUnitLookupFunction(QQmlPrivate::QmlUnitCacheLookupFunction handler)
{
    QQmlMetaTypeDataPtr data;
    data->lookupCachedQmlUnit.removeAll(handler);
}

/*!
    Returns the pretty QML type name (e.g. 'Item' instead of 'QtQuickItem') for the given object.
 */
QString QQmlMetaType::prettyTypeName(const QObject *object)
{
    QString typeName;

    if (!object)
        return typeName;

    QQmlType type = QQmlMetaType::qmlType(object->metaObject());
    if (type.isValid()) {
        typeName = type.qmlTypeName();
        const int lastSlash = typeName.lastIndexOf(QLatin1Char('/'));
        if (lastSlash != -1)
            typeName = typeName.mid(lastSlash + 1);
    }

    if (typeName.isEmpty()) {
        typeName = QString::fromUtf8(object->metaObject()->className());
        int marker = typeName.indexOf(QLatin1String("_QMLTYPE_"));
        if (marker != -1)
            typeName = typeName.left(marker);

        marker = typeName.indexOf(QLatin1String("_QML_"));
        if (marker != -1) {
            typeName = QStringView{typeName}.left(marker) + QLatin1Char('*');
            type = QQmlMetaType::qmlType(QMetaType::fromName(typeName.toLatin1()).id());
            if (type.isValid()) {
                QString qmlTypeName = type.qmlTypeName();
                const int lastSlash = qmlTypeName.lastIndexOf(QLatin1Char('/'));
                if (lastSlash != -1)
                    qmlTypeName = qmlTypeName.mid(lastSlash + 1);
                if (!qmlTypeName.isEmpty())
                    typeName = qmlTypeName;
            }
        }
    }

    return typeName;
}

QList<QQmlProxyMetaObject::ProxyData> QQmlMetaType::proxyData(const QMetaObject *mo,
                                                              const QMetaObject *baseMetaObject,
                                                              QMetaObject *lastMetaObject)
{
    QList<QQmlProxyMetaObject::ProxyData> metaObjects;
    mo = mo->d.superdata;

    const QQmlMetaTypeDataPtr data;

    auto createProxyMetaObject = [&](const QMetaObject *superdataBaseMetaObject,
                                     const QMetaObject *extMetaObject,
                                     QObject *(*extFunc)(QObject *)) {
        if (!extMetaObject)
            return;

        QMetaObjectBuilder builder;
        clone(builder, extMetaObject, superdataBaseMetaObject, baseMetaObject);
        builder.setFlags(MetaObjectFlag::DynamicMetaObject);
        QMetaObject *mmo = builder.toMetaObject();
        mmo->d.superdata = baseMetaObject;
        if (!metaObjects.isEmpty())
            metaObjects.constLast().metaObject->d.superdata = mmo;
        else if (lastMetaObject)
            lastMetaObject->d.superdata = mmo;
        QQmlProxyMetaObject::ProxyData data = { mmo, extFunc, 0, 0 };
        metaObjects << data;
    };

    while (mo) {
        QQmlTypePrivate *t = data->metaObjectToType.value(mo);
        if (t) {
            if (t->regType == QQmlType::CppType) {
                createProxyMetaObject(t->baseMetaObject, t->extraData.cd->extMetaObject,
                                      t->extraData.cd->extFunc);
            } else if (t->regType == QQmlType::SingletonType) {
                createProxyMetaObject(t->baseMetaObject, t->extraData.sd->extMetaObject,
                                      t->extraData.sd->extFunc);
            }
        }
        mo = mo->d.superdata;
    }

    return metaObjects;
}

bool isInternalType(int idx)
{
    // Qt internal types
    switch (idx) {
    case QMetaType::UnknownType:
    case QMetaType::QStringList:
    case QMetaType::QObjectStar:
    case QMetaType::VoidStar:
    case QMetaType::Nullptr:
    case QMetaType::QVariant:
    case QMetaType::QLocale:
    case QMetaType::QImage:  // scarce type, keep as QVariant
    case QMetaType::QPixmap: // scarce type, keep as QVariant
        return true;
    default:
        return false;
    }
}

bool QQmlMetaType::isValueType(QMetaType type)
{
    if (!type.isValid() || isInternalType(type.id()))
        return false;

    return valueType(type) != nullptr;
}

const QMetaObject *QQmlMetaType::metaObjectForValueType(QMetaType metaType)
{
    const int t = metaType.id();
    switch (t) {
    case QMetaType::QPoint:
        return &QQmlPointValueType::staticMetaObject;
    case QMetaType::QPointF:
        return &QQmlPointFValueType::staticMetaObject;
    case QMetaType::QSize:
        return &QQmlSizeValueType::staticMetaObject;
    case QMetaType::QSizeF:
        return &QQmlSizeFValueType::staticMetaObject;
    case QMetaType::QRect:
        return &QQmlRectValueType::staticMetaObject;
    case QMetaType::QRectF:
        return &QQmlRectFValueType::staticMetaObject;
#if QT_CONFIG(easingcurve)
    case QMetaType::QEasingCurve:
        return &QQmlEasingValueType::staticMetaObject;
#endif
#if QT_CONFIG(qml_itemmodel)
    case QMetaType::QModelIndex:
        return &QQmlModelIndexValueType::staticMetaObject;
    case QMetaType::QPersistentModelIndex:
        return &QQmlPersistentModelIndexValueType::staticMetaObject;
#endif
    default:
#if QT_CONFIG(qml_itemmodel)
        if (metaType == QMetaType::fromType<QItemSelectionRange>())
            return &QQmlItemSelectionRangeValueType::staticMetaObject;
#endif
        break;
    }

    // It doesn't have to be a gadget for a QML type to exist, but we don't want to
    // call QObject pointers value types. Explicitly registered types also override
    // the implicit use of gadgets.
    if (!(metaType.flags() & QMetaType::PointerToQObject)) {
        const QQmlType qmlType = QQmlMetaType::qmlType(t, QQmlMetaType::TypeIdCategory::MetaType);

        // Prefer the extension meta object.
        // Extensions allow registration of non-gadget value types.
        if (const QMetaObject *extensionMetaObject = qmlType.extensionMetaObject())
            return extensionMetaObject;

        if (const QMetaObject *qmlTypeMetaObject = qmlType.metaObject())
            return qmlTypeMetaObject;
    }

    // If it _is_ a gadget, we can just use it.
    if (metaType.flags() & QMetaType::IsGadget)
        return metaType.metaObject();

    return nullptr;
}

QQmlValueType *QQmlMetaType::valueType(QMetaType type)
{
    const int idx = type.id();
    QQmlMetaTypeDataPtr data;

    const auto it = data->metaTypeToValueType.constFind(idx);
    if (it != data->metaTypeToValueType.constEnd())
        return *it;

    if (const QMetaObject *mo = metaObjectForValueType(type))
        return *data->metaTypeToValueType.insert(idx, new QQmlValueType(idx, mo));
    return *data->metaTypeToValueType.insert(idx, nullptr);
}

QT_END_NAMESPACE
