/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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
**/
#include "qqmldomtop_p.h"
#include "qqmldomexternalitems_p.h"
#include "qqmldommock_p.h"
#include "qqmldomelements_p.h"
#include "qqmldomastcreator_p.h"
#include "qqmldommoduleindex_p.h"

#include <QtQml/private/qqmljslexer_p.h>
#include <QtQml/private/qqmljsparser_p.h>
#include <QtQml/private/qqmljsengine_p.h>
#include <QtQml/private/qqmljsastvisitor_p.h>
#include <QtQml/private/qqmljsast_p.h>

#include <QtCore/QAtomicInt>
#include <QtCore/QBasicMutex>
#include <QtCore/QCborArray>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QPair>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#if QT_FEATURE_thread
#    include <QtCore/QThread>
#endif

#include <memory>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace Dom {

using std::shared_ptr;


/*!
 \internal
 \brief QQml::Dom::DomTop::loadFile
 \param filePath
 the file path to load
 \param logicalPath
 the path from the
 \param callback
 a callback called with an canonical path, the old value, and the current value.
  \param loadOptions are
 if force is true the file is always read
 */

Path DomTop::canonicalPath(DomItem &) const
{
    return canonicalPath();
}

DomItem DomTop::containingObject(DomItem &) const
{
    return DomItem();
}

bool DomTop::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    static QHash<QString, QString> knownFields;
    static QBasicMutex m;
    auto toField = [](QString f) mutable -> QStringView {
        QMutexLocker l(&m);
        if (!knownFields.contains(f))
            knownFields[f] = f;
        return knownFields[f];
    };
    bool cont = true;
    auto objs = m_extraOwningItems;
    auto itO = objs.cbegin();
    auto endO = objs.cend();
    while (itO != endO) {
        cont = cont && self.dvItemField(visitor, toField(itO.key()), [&self, &itO]() {
            return std::visit([&self](auto &&el) { return self.copy(el); }, *itO);
        });
        ++itO;
    }
    return cont;
}

void DomTop::clearExtraOwningItems()
{
    QMutexLocker l(mutex());
    m_extraOwningItems.clear();
}

QMap<QString, OwnerT> DomTop::extraOwningItems() const
{
    QMutexLocker l(mutex());
    QMap<QString, OwnerT> res = m_extraOwningItems;
    return res;
}

/*!
\class QQmlJS::Dom::DomUniverse

\brief Represents a set of parsed/loaded modules libraries and a plugins

This can be used to share parsing and updates between several Dom models, and kickstart a model
without reparsing everything.

The universe is peculiar, because stepping into it from an environment looses the connection with
the environment.

This implementation is a placeholder, a later patch will introduce it.
 */

ErrorGroups DomUniverse::myErrors()
{
    static ErrorGroups groups = {{ DomItem::domErrorGroup, NewErrorGroup("Universe") }};
    return groups;
}

DomUniverse::DomUniverse(QString universeName, Options options):
    m_name(universeName), m_options(options)
{}

std::shared_ptr<DomUniverse> DomUniverse::guaranteeUniverse(std::shared_ptr<DomUniverse> univ)
{
    static QAtomicInt counter(0);
    if (univ)
        return univ;
    return std::shared_ptr<DomUniverse>(
            new DomUniverse(QLatin1String("universe") + QString::number(++counter)));
}

DomItem DomUniverse::create(QString universeName, Options options)
{
    std::shared_ptr<DomUniverse> res(new DomUniverse(universeName, options));
    return DomItem(res);
}

Path DomUniverse::canonicalPath() const
{
    return Path::Root(u"universe");
}

bool DomUniverse::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    bool cont = true;
    cont = cont && DomTop::iterateDirectSubpaths(self, visitor);
    cont = cont && self.dvValueField(visitor, Fields::name, name());
    cont = cont && self.dvValueField(visitor, Fields::options, int(options()));
    cont = cont && self.dvItemField(visitor, Fields::globalScopeWithName, [this, &self]() {
        return self.subMapItem(Map(
                Path::Field(Fields::globalScopeWithName),
                [this](DomItem &map, QString key) { return map.copy(globalScopeWithName(key)); },
                [this](DomItem &) { return globalScopeNames(); }, QLatin1String("GlobalScope")));
    });
    cont = cont && self.dvItemField(visitor, Fields::qmlDirectoryWithPath, [this, &self]() {
        return self.subMapItem(Map(
                Path::Field(Fields::qmlDirectoryWithPath),
                [this](DomItem &map, QString key) { return map.copy(qmlDirectoryWithPath(key)); },
                [this](DomItem &) { return qmlDirectoryPaths(); }, QLatin1String("QmlDirectory")));
    });
    cont = cont && self.dvItemField(visitor, Fields::qmldirFileWithPath, [this, &self]() {
        return self.subMapItem(Map(
                Path::Field(Fields::qmldirFileWithPath),
                [this](DomItem &map, QString key) { return map.copy(qmldirFileWithPath(key)); },
                [this](DomItem &) { return qmldirFilePaths(); }, QLatin1String("QmldirFile")));
    });
    cont = cont && self.dvItemField(visitor, Fields::qmlFileWithPath, [this, &self]() {
        return self.subMapItem(Map(
                Path::Field(Fields::qmlFileWithPath),
                [this](DomItem &map, QString key) { return map.copy(qmlFileWithPath(key)); },
                [this](DomItem &) { return qmlFilePaths(); }, QLatin1String("QmlFile")));
    });
    cont = cont && self.dvItemField(visitor, Fields::jsFileWithPath, [this, &self]() {
        return self.subMapItem(Map(
                Path::Field(Fields::jsFileWithPath),
                [this](DomItem &map, QString key) { return map.copy(jsFileWithPath(key)); },
                [this](DomItem &) { return jsFilePaths(); }, QLatin1String("JsFile")));
    });
    cont = cont && self.dvItemField(visitor, Fields::jsFileWithPath, [this, &self]() {
        return self.subMapItem(Map(
                Path::Field(Fields::qmltypesFileWithPath),
                [this](DomItem &map, QString key) { return map.copy(qmltypesFileWithPath(key)); },
                [this](DomItem &) { return qmltypesFilePaths(); }, QLatin1String("QmltypesFile")));
    });
    cont = cont && self.dvItemField(visitor, Fields::queue, [this, &self]() {
        QQueue<ParsingTask> q = queue();
        return self.subListItem(List(
                Path::Field(Fields::queue),
                [q](DomItem &list, index_type i) {
                    if (i >= 0 && i < q.length())
                        return list.subDataItem(PathEls::Index(i), q.at(i).toCbor(),
                                                ConstantData::Options::FirstMapIsFields);
                    else
                        return DomItem();
                },
                [q](DomItem &) { return index_type(q.length()); }, nullptr,
                QLatin1String("ParsingTask")));
    });
    return cont;
}

std::shared_ptr<OwningItem> DomUniverse::doCopy(DomItem &) const
{
    QRegularExpression r(QRegularExpression::anchoredPattern(QLatin1String(R"(.*Copy([0-9]*)$)")));
    auto m = r.match(m_name);
    QString newName;
    if (m.hasMatch())
        newName = QStringLiteral(u"%1Copy%2").arg(m_name).arg(m.captured(1).toInt() + 1);
    else
        newName = m_name + QLatin1String("Copy");
    auto res = std::shared_ptr<DomUniverse>(new DomUniverse(newName));
    return res;
}

void DomUniverse::loadFile(DomItem &self, QString filePath, QString logicalPath, Callback callback,
                           LoadOptions loadOptions, std::optional<DomType> fileType)
{
    loadFile(self, filePath, logicalPath, QString(), QDateTime::fromMSecsSinceEpoch(0), callback,
             loadOptions, fileType);
}

static DomType fileTypeForPath(DomItem &self, QString canonicalFilePath)
{
    if (canonicalFilePath.endsWith(u".qml", Qt::CaseInsensitive)
        || canonicalFilePath.endsWith(u".qmlannotation", Qt::CaseInsensitive)) {
        return DomType::QmlFile;
    } else if (canonicalFilePath.endsWith(u".qmltypes")) {
        return DomType::QmltypesFile;
    } else if (QStringView(u"qmldir").compare(QFileInfo(canonicalFilePath).fileName(),
                                              Qt::CaseInsensitive)
               == 0) {
        return DomType::QmltypesFile;
    } else if (QFileInfo(canonicalFilePath).isDir()) {
        return DomType::QmlDirectory;
    } else {
        self.addError(DomUniverse::myErrors()
                              .error(QCoreApplication::translate("Dom::filteTypeForPath",
                                                                 "Could not detect type of file %1")
                                             .arg(canonicalFilePath))
                              .handle());
    }
    return DomType::Empty;
}

void DomUniverse::loadFile(DomItem &self, QString canonicalFilePath, QString logicalPath,
                           QString code, QDateTime codeDate, Callback callback,
                           LoadOptions loadOptions, std::optional<DomType> fileType)
{
    DomType fType = (bool(fileType) ? (*fileType) : fileTypeForPath(self, canonicalFilePath));
    switch (fType) {
    case DomType::QmlFile:
    case DomType::QmltypesFile:
    case DomType::QmldirFile:
    case DomType::QmlDirectory:
        m_queue.enqueue(ParsingTask { QDateTime::currentDateTime(), loadOptions, fType,
                                      canonicalFilePath, logicalPath, code, codeDate,
                                      self.ownerAs<DomUniverse>(), callback });
        break;
    default:
        self.addError(myErrors()
                              .error(tr("Ignoring request to load file %1 of unexpected type %2, "
                                        "calling callback immediately")
                                             .arg(canonicalFilePath, domTypeToString(fType)))
                              .handle());
        Q_ASSERT(false && "loading non supported file type");
        callback(Path(), DomItem::empty, DomItem::empty);
        return;
    }
    if (m_options & Option::SingleThreaded)
        execQueue(); // immediate execution in the same thread
}

template<typename T>
QPair<std::shared_ptr<ExternalItemPair<T>>, std::shared_ptr<ExternalItemPair<T>>>
updateEntry(DomItem &univ, std::shared_ptr<T> newItem,
            QMap<QString, std::shared_ptr<ExternalItemPair<T>>> &map, QBasicMutex *mutex)
{
    std::shared_ptr<ExternalItemPair<T>> oldValue;
    std::shared_ptr<ExternalItemPair<T>> newValue;
    QString canonicalPath = newItem->canonicalFilePath();
    QDateTime now = QDateTime::currentDateTime();
    {
        QMutexLocker l(mutex);
        auto it = map.find(canonicalPath);
        if (it != map.cend() && (*it) && (*it)->current) {
            oldValue = *it;
            QString oldCode = oldValue->current->code();
            QString newCode = newItem->code();
            if (!oldCode.isNull() && !newCode.isNull() && oldCode == newCode) {
                newValue = oldValue;
                if (newValue->current->lastDataUpdateAt() < newItem->lastDataUpdateAt())
                    newValue->current->refreshedDataAt(newItem->lastDataUpdateAt());
            } else if (oldValue->current->lastDataUpdateAt() > newItem->lastDataUpdateAt()) {
                newValue = oldValue;
            } else {
                DomItem oldValueObj = univ.copy(oldValue);
                newValue = oldValue->makeCopy(oldValueObj);
                newValue->current = newItem;
                newValue->currentExposedAt = now;
                if (newItem->isValid()) {
                    newValue->valid = newItem;
                    newValue->validExposedAt = now;
                }
                it = map.insert(it, canonicalPath, newValue);
            }
        } else {
            newValue = std::shared_ptr<ExternalItemPair<T>>(new ExternalItemPair<T>(
                    (newItem->isValid() ? newItem : std::shared_ptr<T>()), newItem, now, now));
            map.insert(canonicalPath, newValue);
        }
    }
    return qMakePair(oldValue, newValue);
}

void DomUniverse::execQueue()
{
    ParsingTask t = m_queue.dequeue();
    shared_ptr<DomUniverse> topPtr = t.requestingUniverse.lock();
    if (!topPtr) {
        myErrors().error(tr("Ignoring callback for loading of %1: universe is not valid anymore").arg(t.canonicalPath)).handle();
    }
    QString canonicalPath = t.canonicalPath;
    QString code = t.contents;
    QDateTime contentDate = t.contentsDate;
    bool skipParse = false;
    DomItem oldValue; // old ExternalItemPair (might be empty, or equal to newValue)
    DomItem newValue; // current ExternalItemPair
    DomItem univ = DomItem(topPtr);
    QFileInfo path(canonicalPath);
    QList<ErrorMessage> messages;

    if (t.kind == DomType::QmlFile || t.kind == DomType::QmltypesFile
        || t.kind == DomType::QmldirFile || t.kind == DomType::QmlDirectory) {
        auto getValue = [&t, this, &canonicalPath]() -> std::shared_ptr<ExternalItemPairBase> {
            if (t.kind == DomType::QmlFile)
                return m_qmlFileWithPath.value(canonicalPath);
            else if (t.kind == DomType::QmltypesFile)
                return m_qmlFileWithPath.value(canonicalPath);
            else if (t.kind == DomType::QmldirFile)
                return m_qmlFileWithPath.value(canonicalPath);
            else if (t.kind == DomType::QmlDirectory)
                return m_qmlDirectoryWithPath.value(canonicalPath);
            else
                Q_ASSERT(false);
            return {};
        };
        if (code.isEmpty()) {
            QFile file(canonicalPath);
            canonicalPath = path.canonicalFilePath();
            if (canonicalPath.isEmpty()) {
                messages.append(myErrors().error(tr("Non existing path %1").arg(t.canonicalPath)));
                canonicalPath = t.canonicalPath;
            }
            {
                QMutexLocker l(mutex());
                auto value = getValue();
                if (!(t.loadOptions & LoadOption::ForceLoad) && value) {
                    if (value && value->currentItem()
                        && path.lastModified() < value->currentItem()->lastDataUpdateAt()) {
                        oldValue = newValue = univ.copy(value);
                        skipParse = true;
                    }
                }
            }
            if (!skipParse) {
                contentDate = QDateTime::currentDateTime();
                if (QFileInfo(canonicalPath).isDir()) {
                    code = QDir(canonicalPath)
                                   .entryList(QDir::NoDotAndDotDot | QDir::Files, QDir::Name)
                                   .join(QLatin1Char('\n'));
                } else if (!file.open(QIODevice::ReadOnly)) {
                    code = QStringLiteral(u"");
                    messages.append(myErrors().error(tr("Error opening path %1: %2 %3")
                                                             .arg(canonicalPath,
                                                                  QString::number(file.error()),
                                                                  file.errorString())));
                } else {
                    code = QString::fromUtf8(file.readAll());
                    file.close();
                }
            }
        }
        if (!skipParse) {
            QMutexLocker l(mutex());
            if (auto value = getValue()) {
                QString oldCode = value->currentItem()->code();
                if (value && value->currentItem() && !oldCode.isNull() && oldCode == code) {
                    skipParse = true;
                    newValue = oldValue = univ.copy(value);
                    if (value->currentItem()->lastDataUpdateAt() < contentDate)
                        value->currentItem()->refreshedDataAt(contentDate);
                }
            }
        }
        if (!skipParse) {
            QDateTime now(QDateTime::currentDateTime());
            if (t.kind == DomType::QmlFile) {
                shared_ptr<QmlFile> qmlFile(new QmlFile(canonicalPath, code, contentDate));
                shared_ptr<DomEnvironment> envPtr(new DomEnvironment(
                        QStringList(), DomEnvironment::Option::NoDependencies, topPtr));
                envPtr->addQmlFile(qmlFile);
                DomItem env(envPtr);
                if (qmlFile->isValid()) {
                    MutableDomItem qmlFileObj(env.copy(qmlFile));
                    createDom(qmlFileObj);
                } else {
                    QString errs;
                    DomItem qmlFileObj = env.copy(qmlFile);
                    qmlFile->iterateErrors(qmlFileObj, [&errs](DomItem, ErrorMessage m) {
                        errs += m.toString();
                        errs += u"\n";
                        return true;
                    });
                    qCWarning(domLog).noquote().nospace()
                            << "Parsed invalid file " << canonicalPath << errs;
                }
                auto change = updateEntry<QmlFile>(univ, qmlFile, m_qmlFileWithPath, mutex());
                oldValue = univ.copy(change.first);
                newValue = univ.copy(change.second);
            } else if (t.kind == DomType::QmltypesFile) {
                shared_ptr<QmltypesFile> qmltypesFile(
                        new QmltypesFile(canonicalPath, code, contentDate));
                auto change = updateEntry<QmltypesFile>(univ, qmltypesFile, m_qmltypesFileWithPath,
                                                        mutex());
                oldValue = univ.copy(change.first);
                newValue = univ.copy(change.second);
            } else if (t.kind == DomType::QmldirFile) {
                shared_ptr<QmldirFile> qmldirFile =
                        QmldirFile::fromPathAndCode(canonicalPath, code);
                auto change =
                        updateEntry<QmldirFile>(univ, qmldirFile, m_qmldirFileWithPath, mutex());
                oldValue = univ.copy(change.first);
                newValue = univ.copy(change.second);
            } else if (t.kind == DomType::QmlDirectory) {
                shared_ptr<QmlDirectory> qmlDirectory(new QmlDirectory(
                        canonicalPath, code.split(QLatin1Char('\n')), contentDate));
                auto change = updateEntry<QmlDirectory>(univ, qmlDirectory, m_qmlDirectoryWithPath,
                                                        mutex());
                oldValue = univ.copy(change.first);
                newValue = univ.copy(change.second);
            } else {
                Q_ASSERT(false);
            }
        }
        for (const ErrorMessage &m : messages)
            newValue.addError(m);
        // to do: tell observers?
        // execute callback
        if (t.callback) {
            Path p;
            if (t.kind == DomType::QmlFile)
                p = Paths::qmlFileInfoPath(canonicalPath);
            else if (t.kind == DomType::QmltypesFile)
                p = Paths::qmltypesFileInfoPath(canonicalPath);
            else if (t.kind == DomType::QmldirFile)
                p = Paths::qmldirFileInfoPath(canonicalPath);
            else if (t.kind == DomType::QmlDirectory)
                p = Paths::qmlDirectoryInfoPath(canonicalPath);
            else
                Q_ASSERT(false);
            t.callback(p, oldValue, newValue);
        }
    } else {
        Q_ASSERT(false && "Unhandled kind in queue");
    }
}

std::shared_ptr<OwningItem> LoadInfo::doCopy(DomItem &self) const
{
    std::shared_ptr<LoadInfo> res(new LoadInfo(*this));
    if (res->status() != Status::Done) {
        res->addErrorLocal(DomEnvironment::myErrors().warning(
                u"This is a copy of a LoadInfo still in progress, artificially ending it, if you "
                u"use this you will *not* resume loading"));
        DomEnvironment::myErrors()
                .warning([&self](Sink sink) {
                    sink(u"Copying an in progress LoadInfo, which is most likely an error (");
                    self.dump(sink);
                    sink(u")");
                })
                .handle();
        QMutexLocker l(res->mutex());
        res->m_status = Status::Done;
        res->m_toDo.clear();
        res->m_inProgress.clear();
        res->m_endCallbacks.clear();
    }
    return res;
}

Path LoadInfo::canonicalPath(DomItem &) const
{
    return Path::Root(PathRoot::Env).field(Fields::loadInfo).key(elementCanonicalPath().toString());
}

bool LoadInfo::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    bool cont = OwningItem::iterateDirectSubpaths(self, visitor);
    cont = cont && self.dvValueField(visitor, Fields::status, int(status()));
    cont = cont && self.dvValueField(visitor, Fields::nLoaded, nLoaded());
    cont = cont
            && self.dvValueField(visitor, Fields::elementCanonicalPath,
                                 elementCanonicalPath().toString());
    cont = cont && self.dvValueField(visitor, Fields::nNotdone, nNotDone());
    cont = cont && self.dvValueField(visitor, Fields::nCallbacks, nCallbacks());
    return cont;
}

void LoadInfo::addEndCallback(DomItem &self,
                              std::function<void(Path, DomItem &, DomItem &)> callback)
{
    if (!callback)
        return;
    {
        QMutexLocker l(mutex());
        switch (m_status) {
        case Status::NotStarted:
        case Status::Starting:
        case Status::InProgress:
        case Status::CallingCallbacks:
            m_endCallbacks.append(callback);
            return;
        case Status::Done:
            break;
        }
    }
    Path p = elementCanonicalPath();
    DomItem el = self.path(p);
    callback(p, el, el);
}

void LoadInfo::advanceLoad(DomItem &self)
{
    Status myStatus;
    Dependency dep;
    bool depValid = false;
    {
        QMutexLocker l(mutex());
        myStatus = m_status;
        switch (myStatus) {
        case Status::NotStarted:
            m_status = Status::Starting;
            break;
        case Status::Starting:
        case Status::InProgress:
            if (!m_toDo.isEmpty()) {
                dep = m_toDo.dequeue();
                m_inProgress.append(dep);
                depValid = true;
            }
            break;
        case Status::CallingCallbacks:
        case Status::Done:
            break;
        }
    }
    switch (myStatus) {
    case Status::NotStarted:
        refreshedDataAt(QDateTime::currentDateTime());
        doAddDependencies(self);
        refreshedDataAt(QDateTime::currentDateTime());
        {
            QMutexLocker l(mutex());
            Q_ASSERT(m_status == Status::Starting);
            if (m_toDo.isEmpty() && m_inProgress.isEmpty())
                myStatus = m_status = Status::CallingCallbacks;
            else
                myStatus = m_status = Status::InProgress;
        }
        if (myStatus == Status::CallingCallbacks)
            execEnd(self);
        break;
    case Status::Starting:
    case Status::InProgress:
        if (depValid) {
            refreshedDataAt(QDateTime::currentDateTime());
            if (!dep.uri.isEmpty()) {
                self.loadModuleDependency(
                        dep.uri, dep.version,
                        [this, self, dep](Path, DomItem &, DomItem &) mutable {
                            finishedLoadingDep(self, dep);
                        },
                        self.errorHandler());
                Q_ASSERT(dep.filePath.isEmpty() && "dependency with both uri and file");
            } else if (!dep.filePath.isEmpty()) {
                DomItem env = self.environment();
                if (std::shared_ptr<DomEnvironment> envPtr = env.ownerAs<DomEnvironment>())
                    envPtr->loadFile(
                            env, dep.filePath, QString(),
                            [this, self, dep](Path, DomItem &, DomItem &) mutable {
                                finishedLoadingDep(self, dep);
                            },
                            nullptr, nullptr, LoadOption::DefaultLoad, dep.fileType,
                            self.errorHandler());
                else
                    Q_ASSERT(false && "missing environment");
            } else {
                Q_ASSERT(false && "dependency without uri and filePath");
            }
        } else {
            addErrorLocal(DomEnvironment::myErrors().error(
                    tr("advanceLoad called but found no work, which should never happen")));
        }
        break;
    case Status::CallingCallbacks:
    case Status::Done:
        addErrorLocal(DomEnvironment::myErrors().error(tr(
                "advanceLoad called after work should have been done, which should never happen")));
        break;
    }
}

void LoadInfo::finishedLoadingDep(DomItem &self, const Dependency &d)
{
    bool didRemove = false;
    bool unexpectedState = false;
    bool doEnd = false;
    {
        QMutexLocker l(mutex());
        didRemove = m_inProgress.removeOne(d);
        switch (m_status) {
        case Status::NotStarted:
        case Status::CallingCallbacks:
        case Status::Done:
            unexpectedState = true;
            break;
        case Status::Starting:
            break;
        case Status::InProgress:
            if (m_toDo.isEmpty() && m_inProgress.isEmpty()) {
                m_status = Status::CallingCallbacks;
                doEnd = true;
            }
            break;
        }
    }
    if (!didRemove) {
        addErrorLocal(DomEnvironment::myErrors().error([&self](Sink sink) {
            sink(u"LoadInfo::finishedLoadingDep did not find its dependency in those inProgress "
                 u"()");
            self.dump(sink);
            sink(u")");
        }));
        Q_ASSERT(false
                 && "LoadInfo::finishedLoadingDep did not find its dependency in those inProgress");
    }
    if (unexpectedState) {
        addErrorLocal(DomEnvironment::myErrors().error([&self](Sink sink) {
            sink(u"LoadInfo::finishedLoadingDep found an unexpected state (");
            self.dump(sink);
            sink(u")");
        }));
        Q_ASSERT(false && "LoadInfo::finishedLoadingDep did find an unexpected state");
    }
    if (doEnd)
        execEnd(self);
}

void LoadInfo::execEnd(DomItem &self)
{
    QList<std::function<void(Path, DomItem &, DomItem &)>> endCallbacks;
    bool unexpectedState = false;
    {
        QMutexLocker l(mutex());
        unexpectedState = m_status != Status::CallingCallbacks;
        endCallbacks = m_endCallbacks;
        m_endCallbacks.clear();
    }
    Q_ASSERT(!unexpectedState && "LoadInfo::execEnd found an unexpected state");
    Path p = elementCanonicalPath();
    DomItem el = self.path(p);
    {
        auto cleanup = qScopeGuard([this, p, &el] {
            QList<std::function<void(Path, DomItem &, DomItem &)>> otherCallbacks;
            bool unexpectedState2 = false;
            {
                QMutexLocker l(mutex());
                unexpectedState2 = m_status != Status::CallingCallbacks;
                m_status = Status::Done;
                otherCallbacks = m_endCallbacks;
                m_endCallbacks.clear();
            }
            Q_ASSERT(!unexpectedState2 && "LoadInfo::execEnd found an unexpected state");
            for (auto const &cb : otherCallbacks) {
                if (cb)
                    cb(p, el, el);
            }
        });
        for (auto const &cb : endCallbacks) {
            if (cb)
                cb(p, el, el);
        }
    }
}

void LoadInfo::doAddDependencies(DomItem &self)
{
    if (!elementCanonicalPath()) {
        DomEnvironment::myErrors()
                .error(tr("Uninitialized LoadInfo %1").arg(self.canonicalPath().toString()))
                .handle(nullptr);
        Q_ASSERT(false);
        return;
    }
    // sychronous add of all dependencies
    DomItem el = self.path(elementCanonicalPath());
    if (el.internalKind() == DomType::ExternalItemInfo) {
        DomItem currentImports = el.field(Fields::currentItem).field(Fields::imports);
        int iEnd = currentImports.indexes();
        for (int i = 0; i < iEnd; ++i) {
            DomItem import = currentImports.index(i);
            if (const Import *importPtr = import.as<Import>()) {
                if (!importPtr->filePath().isEmpty()) {
                    addDependency(self,
                                  Dependency { QString(), importPtr->version, importPtr->filePath(),
                                               DomType::Empty });
                } else {
                    addDependency(self,
                                  Dependency { importPtr->uri, importPtr->version, QString(),
                                               DomType::ModuleIndex });
                }
            }
        }
        DomItem currentQmltypesFiles = el.field(Fields::currentItem).field(Fields::qmltypesFiles);
        int qEnd = currentQmltypesFiles.indexes();
        for (int i = 0; i < qEnd; ++i) {
            DomItem qmltypesRef = currentQmltypesFiles.index(i);
            if (const Reference *ref = qmltypesRef.as<Reference>()) {
                Path canonicalPath = ref->referredObjectPath[2];
                if (canonicalPath && !canonicalPath.headName().isEmpty())
                    addDependency(self,
                                  Dependency { QString(), Version(), canonicalPath.headName(),
                                               DomType::QmltypesFile });
            }
        }
        DomItem currentQmlFiles = el.field(Fields::currentItem).field(Fields::qmlFiles);
        currentQmlFiles.visitKeys([this, &self](QString, DomItem &els) {
            return els.visitIndexes([this, &self](DomItem &el) {
                if (const Reference *ref = el.as<Reference>()) {
                    Path canonicalPath = ref->referredObjectPath[2];
                    if (canonicalPath && !canonicalPath.headName().isEmpty())
                        addDependency(self,
                                      Dependency { QString(), Version(), canonicalPath.headName(),
                                                   DomType::QmlFile });
                }
                return true;
            });
        });
    } else if (shared_ptr<ModuleIndex> elPtr = el.ownerAs<ModuleIndex>()) {
        for (Path qmldirPath : elPtr->qmldirsToLoad(el)) {
            Path canonicalPath = qmldirPath[2];
            if (canonicalPath && !canonicalPath.headName().isEmpty())
                addDependency(self,
                              Dependency { QString(), Version(), canonicalPath.headName(),
                                           DomType::QmldirFile });
        }
    } else if (!el) {
        self.addError(DomEnvironment::myErrors().error(
                tr("Ignoring dependencies for empty (invalid) type")
                        .arg(domTypeToString(el.internalKind()))));
    } else {
        self.addError(
                DomEnvironment::myErrors().error(tr("dependencies of %1 (%2) not yet implemented")
                                                         .arg(domTypeToString(el.internalKind()),
                                                              elementCanonicalPath().toString())));
    }
}

void LoadInfo::addDependency(DomItem &self, const Dependency &dep)
{
    bool unexpectedState = false;
    {
        QMutexLocker l(mutex());
        unexpectedState = m_status != Status::Starting;
        m_toDo.enqueue(dep);
    }
    Q_ASSERT(!unexpectedState && "LoadInfo::addDependency found an unexpected state");
    DomItem env = self.environment();
    env.ownerAs<DomEnvironment>()->addWorkForLoadInfo(elementCanonicalPath());
}

/*!
\class QQmlJS::Dom::DomEnvironment

\brief Represents a consistent set of types organized in modules, it is the top level of the DOM
 */

template<typename T>
DomTop::Callback envCallbackForFile(
        DomItem &self, QMap<QString, std::shared_ptr<ExternalItemInfo<T>>> DomEnvironment::*map,
        std::shared_ptr<ExternalItemInfo<T>> (DomEnvironment::*lookupF)(DomItem &, QString,
                                                                        EnvLookup) const,
        DomTop::Callback loadCallback, DomTop::Callback allDirectDepsCallback,
        DomTop::Callback endCallback)
{
    std::shared_ptr<DomEnvironment> ePtr = self.ownerAs<DomEnvironment>();
    std::weak_ptr<DomEnvironment> selfPtr = ePtr;
    std::shared_ptr<DomEnvironment> basePtr = ePtr->base();
    return [selfPtr, basePtr, map, lookupF, loadCallback, allDirectDepsCallback,
            endCallback](Path, DomItem &, DomItem &newItem) {
        shared_ptr<DomEnvironment> envPtr = selfPtr.lock();
        if (!envPtr)
            return;
        DomItem env = DomItem(envPtr);
        shared_ptr<ExternalItemInfo<T>> oldValue;
        shared_ptr<ExternalItemInfo<T>> newValue;
        shared_ptr<T> newItemPtr;
        if (envPtr->options() & DomEnvironment::Option::KeepValid)
            newItemPtr = newItem.field(Fields::validItem).ownerAs<T>();
        if (!newItemPtr)
            newItemPtr = newItem.field(Fields::currentItem).ownerAs<T>();
        Q_ASSERT(newItemPtr && "callbackForQmlFile reached without current qmlFile");
        {
            QMutexLocker l(envPtr->mutex());
            oldValue = ((*envPtr).*map).value(newItem.canonicalFilePath());
        }
        if (oldValue) {
            // we do not change locally loaded files (avoid loading a file more than once)
            newValue = oldValue;
        } else {
            if (basePtr) {
                DomItem baseObj(basePtr);
                oldValue = ((*basePtr).*lookupF)(baseObj, newItem.canonicalFilePath(),
                                                 EnvLookup::BaseOnly);
            }
            if (oldValue) {
                DomItem oldValueObj = env.copy(oldValue);
                newValue = oldValue->makeCopy(oldValueObj);
                if (newValue->current != newItemPtr) {
                    newValue->current = newItemPtr;
                    newValue->setCurrentExposedAt(QDateTime::currentDateTime());
                }
            } else {
                newValue = std::shared_ptr<ExternalItemInfo<T>>(
                        new ExternalItemInfo<T>(newItemPtr, QDateTime::currentDateTime()));
            }
            {
                QMutexLocker l(envPtr->mutex());
                auto value = ((*envPtr).*map).value(newItem.canonicalFilePath());
                if (value) {
                    oldValue = newValue = value;
                } else {
                    ((*envPtr).*map).insert(newItem.canonicalFilePath(), newValue);
                }
            }
        }
        Path p = env.copy(newValue).canonicalPath();
        {
            auto depLoad = qScopeGuard([p, &env, envPtr, allDirectDepsCallback, endCallback] {
                if (!(envPtr->options() & DomEnvironment::Option::NoDependencies)) {
                    std::shared_ptr<LoadInfo> loadInfo(new LoadInfo(p));
                    if (!p)
                        Q_ASSERT(false);
                    DomItem loadInfoObj = env.copy(loadInfo);
                    loadInfo->addEndCallback(loadInfoObj, allDirectDepsCallback);
                    envPtr->addLoadInfo(env, loadInfo);
                }
                if (endCallback)
                    envPtr->addAllLoadedCallback(env,
                                                 [p, endCallback](Path, DomItem &, DomItem &env) {
                                                     DomItem el = env.path(p);
                                                     endCallback(p, el, el);
                                                 });
            });
            if (loadCallback) {
                DomItem oldValueObj = env.copy(oldValue);
                DomItem newValueObj = env.copy(newValue);
                loadCallback(p, oldValueObj, newValueObj);
            }
            if ((envPtr->options() & DomEnvironment::Option::NoDependencies)
                && allDirectDepsCallback) {
                DomItem oldValueObj = env.copy(oldValue);
                DomItem newValueObj = env.copy(newValue);
                env.addError(DomEnvironment::myErrors().warning(
                        QLatin1String("calling allDirectDepsCallback immediately for load with "
                                      "NoDependencies of %1")
                                .arg(newItem.canonicalFilePath())));
                allDirectDepsCallback(p, oldValueObj, newValueObj);
            }
        }
    };
}

ErrorGroups DomEnvironment::myErrors() {
    static ErrorGroups res = {{NewErrorGroup("Dom")}};
    return res;
}

DomType DomEnvironment::kind() const
{
    return kindValue;
}

Path DomEnvironment::canonicalPath() const
{
    return Path::Root(u"env");
}

bool DomEnvironment::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    bool cont = true;
    cont = cont && DomTop::iterateDirectSubpaths(self, visitor);
    DomItem univ = universe();
    cont = cont && self.dvItemField(visitor, Fields::universe, [this]() { return universe(); });
    cont = cont && self.dvValueField(visitor, Fields::options, int(options()));
    cont = cont && self.dvItemField(visitor, Fields::base, [this]() { return base(); });
    cont = cont
            && self.dvValueLazyField(visitor, Fields::loadPaths, [this]() { return loadPaths(); });
    cont = cont && self.dvValueField(visitor, Fields::globalScopeName, globalScopeName());
    cont = cont && self.dvItemField(visitor, Fields::globalScopeWithName, [this, &self]() {
        return self.subMapItem(Map(
                Path::Field(Fields::globalScopeWithName),
                [&self, this](DomItem &map, QString key) {
                    return map.copy(globalScopeWithName(self, key));
                },
                [&self, this](DomItem &) { return globalScopeNames(self); },
                QLatin1String("GlobalScope")));
    });
    cont = cont && self.dvItemField(visitor, Fields::qmlDirectoryWithPath, [this, &self]() {
        return self.subMapItem(Map(
                Path::Field(Fields::qmlDirectoryWithPath),
                [&self, this](DomItem &map, QString key) {
                    return map.copy(qmlDirectoryWithPath(self, key));
                },
                [&self, this](DomItem &) { return qmlDirectoryPaths(self); },
                QLatin1String("QmlDirectory")));
    });
    cont = cont && self.dvItemField(visitor, Fields::qmldirFileWithPath, [this, &self]() {
        return self.subMapItem(Map(
                Path::Field(Fields::qmldirFileWithPath),
                [&self, this](DomItem &map, QString key) {
                    return map.copy(qmldirFileWithPath(self, key));
                },
                [&self, this](DomItem &) { return qmldirFilePaths(self); },
                QLatin1String("QmldirFile")));
    });
    cont = cont && self.dvItemField(visitor, Fields::qmldirWithPath, [this, &self]() {
        return self.subMapItem(Map(
                Path::Field(Fields::qmldirWithPath),
                [&self, this](DomItem &map, QString key) {
                    return map.copy(qmlDirWithPath(self, key));
                },
                [&self, this](DomItem &) { return qmlDirPaths(self); }, QLatin1String("Qmldir")));
    });
    cont = cont && self.dvItemField(visitor, Fields::qmlFileWithPath, [this, &self]() {
        return self.subMapItem(Map(
                Path::Field(Fields::qmlFileWithPath),
                [&self, this](DomItem &map, QString key) {
                    return map.copy(qmlFileWithPath(self, key));
                },
                [&self, this](DomItem &) { return qmlFilePaths(self); }, QLatin1String("QmlFile")));
    });
    cont = cont && self.dvItemField(visitor, Fields::jsFileWithPath, [this, &self]() {
        return self.subMapItem(Map(
                Path::Field(Fields::jsFileWithPath),
                [this](DomItem &map, QString key) {
                    DomItem mapOw(map.owner());
                    return map.copy(jsFileWithPath(mapOw, key));
                },
                [this](DomItem &map) {
                    DomItem mapOw = map.owner();
                    return jsFilePaths(mapOw);
                },
                QLatin1String("JsFile")));
    });
    cont = cont && self.dvItemField(visitor, Fields::qmltypesFileWithPath, [this, &self]() {
        return self.subMapItem(Map(
                Path::Field(Fields::qmltypesFileWithPath),
                [this](DomItem &map, QString key) {
                    DomItem mapOw = map.owner();
                    return map.copy(qmltypesFileWithPath(mapOw, key));
                },
                [this](DomItem &map) {
                    DomItem mapOw = map.owner();
                    return qmltypesFilePaths(mapOw);
                },
                QLatin1String("QmltypesFile")));
    });
    cont = cont && self.dvItemField(visitor, Fields::moduleIndexWithUri, [this, &self]() {
        return self.subMapItem(Map(
                Path::Field(Fields::moduleIndexWithUri),
                [this](DomItem &map, QString key) {
                    return map.subMapItem(Map(
                            map.pathFromOwner().key(key),
                            [this, key](DomItem &submap, QString subKey) {
                                bool ok;
                                int i = subKey.toInt(&ok);
                                if (!ok) {
                                    if (subKey.isEmpty())
                                        i = Version::Undefined;
                                    else if (subKey.compare(u"Latest", Qt::CaseInsensitive) == 0)
                                        i = Version::Latest;
                                    else
                                        return DomItem();
                                }
                                DomItem subMapOw = submap.owner();
                                std::shared_ptr<ModuleIndex> mIndex =
                                        moduleIndexWithUri(subMapOw, key, i);
                                return submap.copy(mIndex);
                            },
                            [this, key](DomItem &subMap) {
                                QSet<QString> res;
                                DomItem subMapOw = subMap.owner();
                                for (int mVersion :
                                     moduleIndexMajorVersions(subMapOw, key, EnvLookup::Normal))
                                    if (mVersion == Version::Undefined)
                                        res.insert(QString());
                                    else
                                        res.insert(QString::number(mVersion));
                                if (!res.isEmpty())
                                    res.insert(QLatin1String("Latest"));
                                return res;
                            },
                            QLatin1String("ModuleIndex")));
                },
                [this](DomItem &map) {
                    DomItem mapOw = map.owner();
                    return moduleIndexUris(mapOw);
                },
                QLatin1String("Map<ModuleIndex>")));
    });
    bool loadedLoadInfo = false;
    QQueue<Path> loadsWithWork;
    QQueue<Path> inProgress;
    int nAllLoadedCallbacks;
    auto ensureInfo = [&]() {
        if (!loadedLoadInfo) {
            QMutexLocker l(mutex());
            loadedLoadInfo = true;
            loadsWithWork = m_loadsWithWork;
            inProgress = m_inProgress;
            nAllLoadedCallbacks = m_allLoadedCallback.length();
        }
    };
    cont = cont
            && self.dvItemField(
                    visitor, Fields::loadsWithWork, [&ensureInfo, &self, &loadsWithWork]() {
                        ensureInfo();
                        return self.subListItem(List(
                                Path::Field(Fields::loadsWithWork),
                                [loadsWithWork](DomItem &list, index_type i) {
                                    if (i >= 0 && i < loadsWithWork.length())
                                        return list.subDataItem(PathEls::Index(i),
                                                                loadsWithWork.at(i).toString());
                                    else
                                        return DomItem();
                                },
                                [loadsWithWork](DomItem &) {
                                    return index_type(loadsWithWork.length());
                                },
                                nullptr, QLatin1String("Path")));
                    });
    cont = cont
            && self.dvItemField(visitor, Fields::inProgress, [&self, &ensureInfo, &inProgress]() {
                   ensureInfo();
                   return self.subListItem(List(
                           Path::Field(Fields::inProgress),
                           [inProgress](DomItem &list, index_type i) {
                               if (i >= 0 && i < inProgress.length())
                                   return list.subDataItem(PathEls::Index(i),
                                                           inProgress.at(i).toString());
                               else
                                   return DomItem();
                           },
                           [inProgress](DomItem &) { return index_type(inProgress.length()); },
                           nullptr, QLatin1String("Path")));
               });
    cont = cont && self.dvItemField(visitor, Fields::loadInfo, [&self, this]() {
        return self.subMapItem(Map(
                Path::Field(Fields::loadInfo),
                [this](DomItem &map, QString pStr) {
                    bool hasErrors = false;
                    Path p = Path::fromString(pStr, [&hasErrors](ErrorMessage m) {
                        switch (m.level) {
                        case ErrorLevel::Debug:
                        case ErrorLevel::Info:
                            break;
                        case ErrorLevel::Warning:
                        case ErrorLevel::Error:
                        case ErrorLevel::Fatal:
                            hasErrors = true;
                            break;
                        }
                    });
                    if (!hasErrors)
                        return map.copy(loadInfo(p));
                    return DomItem();
                },
                [this](DomItem &) {
                    QSet<QString> res;
                    for (const Path &p : loadInfoPaths())
                        res.insert(p.toString());
                    return res;
                },
                QLatin1String("LoadInfo")));
    });
    cont = cont && self.dvWrapField(visitor, Fields::imports, m_implicitImports);
    cont = cont
            && self.dvValueLazyField(visitor, Fields::nAllLoadedCallbacks,
                                     [&nAllLoadedCallbacks, &ensureInfo]() {
                                         ensureInfo();
                                         return nAllLoadedCallbacks;
                                     });
    return cont;
}

DomItem DomEnvironment::field(DomItem &self, QStringView name) const
{
    return DomTop::field(self, name);
}

std::shared_ptr<DomEnvironment> DomEnvironment::makeCopy(DomItem &self) const
{
    return std::static_pointer_cast<DomEnvironment>(doCopy(self));
}

void DomEnvironment::loadFile(DomItem &self, QString filePath, QString logicalPath,
                              DomTop::Callback loadCallback, DomTop::Callback directDepsCallback,
                              DomTop::Callback endCallback, LoadOptions loadOptions,
                              std::optional<DomType> fileType, ErrorHandler h)
{
    loadFile(self, filePath, logicalPath, QString(), QDateTime::fromMSecsSinceEpoch(0),
             loadCallback, directDepsCallback, endCallback, loadOptions, fileType, h);
}

std::shared_ptr<OwningItem> DomEnvironment::doCopy(DomItem &) const
{
    shared_ptr<DomEnvironment> res;
    if (m_base)
        res = std::shared_ptr<DomEnvironment>(new DomEnvironment(m_base, m_loadPaths, m_options));
    else
        res = std::shared_ptr<DomEnvironment>(
                new DomEnvironment(m_loadPaths, m_options, m_universe));
    return res;
}

void DomEnvironment::loadFile(DomItem &self, QString filePath, QString logicalPath, QString code,
                              QDateTime codeDate, Callback loadCallback,
                              Callback directDepsCallback, Callback endCallback,
                              LoadOptions loadOptions, std::optional<DomType> fileType,
                              ErrorHandler h)
{
    QFileInfo fileInfo(filePath);
    QString canonicalFilePath = fileInfo.canonicalFilePath();
    if (canonicalFilePath.isEmpty()) {
        if (code.isNull()) {
            myErrors().error(tr("Non existing path to load: '%1'").arg(filePath)).handle(h);
            if (loadCallback)
                loadCallback(Path(), DomItem::empty, DomItem::empty);
            if (directDepsCallback)
                directDepsCallback(Path(), DomItem::empty, DomItem::empty);
            if (endCallback)
                addAllLoadedCallback(self, [endCallback](Path, DomItem &, DomItem &) {
                    endCallback(Path(), DomItem::empty, DomItem::empty);
                });
            return;
        } else {
            canonicalFilePath = filePath;
        }
    }
    shared_ptr<ExternalItemInfoBase> oldValue, newValue;
    DomType fType = (bool(fileType) ? (*fileType) : fileTypeForPath(self, canonicalFilePath));
    switch (fType) {
    case DomType::QmlDirectory: {
        {
            QMutexLocker l(mutex());
            auto it = m_qmlDirectoryWithPath.find(canonicalFilePath);
            if (it != m_qmlDirectoryWithPath.end())
                oldValue = newValue = *it;
        }
        if (!newValue && (options() & Option::NoReload) && m_base) {
            if (auto v = m_base->qmlDirectoryWithPath(self, canonicalFilePath, EnvLookup::Normal)) {
                oldValue = v;
                QDateTime now = QDateTime::currentDateTime();
                std::shared_ptr<ExternalItemInfo<QmlDirectory>> newV(
                        new ExternalItemInfo<QmlDirectory>(v->current, now, v->revision(),
                                                           v->lastDataUpdateAt()));
                newValue = newV;
                QMutexLocker l(mutex());
                auto it = m_qmlDirectoryWithPath.find(canonicalFilePath);
                if (it != m_qmlDirectoryWithPath.end())
                    oldValue = newValue = *it;
                else
                    m_qmlDirectoryWithPath.insert(canonicalFilePath, newV);
            }
        }
        if (!newValue) {
            self.universe().loadFile(
                    canonicalFilePath, logicalPath, code, codeDate,
                    callbackForQmlDirectory(self, loadCallback, directDepsCallback, endCallback),
                    loadOptions, fType);
            return;
        }
    } break;
    case DomType::QmlFile: {
        {
            QMutexLocker l(mutex());
            auto it = m_qmlFileWithPath.find(canonicalFilePath);
            if (it != m_qmlFileWithPath.end())
                oldValue = newValue = *it;
        }
        if (!newValue && (options() & Option::NoReload) && m_base) {
            if (auto v = m_base->qmlFileWithPath(self, canonicalFilePath, EnvLookup::Normal)) {
                oldValue = v;
                QDateTime now = QDateTime::currentDateTime();
                std::shared_ptr<ExternalItemInfo<QmlFile>> newV(new ExternalItemInfo<QmlFile>(
                        v->current, now, v->revision(), v->lastDataUpdateAt()));
                newValue = newV;
                QMutexLocker l(mutex());
                auto it = m_qmlFileWithPath.find(canonicalFilePath);
                if (it != m_qmlFileWithPath.end())
                    oldValue = newValue = *it;
                else
                    m_qmlFileWithPath.insert(canonicalFilePath, newV);
            }
        }
        if (!newValue) {
            self.universe().loadFile(
                    canonicalFilePath, logicalPath, code, codeDate,
                    callbackForQmlFile(self, loadCallback, directDepsCallback, endCallback),
                    loadOptions, fType);
            return;
        }
    } break;
    case DomType::QmltypesFile: {
        {
            QMutexLocker l(mutex());
            auto it = m_qmltypesFileWithPath.find(canonicalFilePath);
            if (it != m_qmltypesFileWithPath.end())
                oldValue = newValue = *it;
        }
        if (!newValue && (options() & Option::NoReload) && m_base) {
            if (auto v = m_base->qmltypesFileWithPath(self, canonicalFilePath, EnvLookup::Normal)) {
                oldValue = v;
                QDateTime now = QDateTime::currentDateTime();
                std::shared_ptr<ExternalItemInfo<QmltypesFile>> newV(
                        new ExternalItemInfo<QmltypesFile>(v->current, now, v->revision(),
                                                           v->lastDataUpdateAt()));
                newValue = newV;
                QMutexLocker l(mutex());
                auto it = m_qmltypesFileWithPath.find(canonicalFilePath);
                if (it != m_qmltypesFileWithPath.end())
                    oldValue = newValue = *it;
                else
                    m_qmltypesFileWithPath.insert(canonicalFilePath, newV);
            }
        }
        if (!newValue) {
            self.universe().loadFile(
                    canonicalFilePath, logicalPath, code, codeDate,
                    callbackForQmltypesFile(self, loadCallback, directDepsCallback, endCallback),
                    loadOptions, fType);
            return;
        }
    } break;
    case DomType::QmldirFile: {
        {
            QMutexLocker l(mutex());
            auto it = m_qmldirFileWithPath.find(canonicalFilePath);
            if (it != m_qmldirFileWithPath.end())
                oldValue = newValue = *it;
        }
        if (!newValue && (options() & Option::NoReload) && m_base) {
            if (auto v = m_base->qmldirFileWithPath(self, canonicalFilePath, EnvLookup::Normal)) {
                oldValue = v;
                QDateTime now = QDateTime::currentDateTime();
                std::shared_ptr<ExternalItemInfo<QmldirFile>> newV(new ExternalItemInfo<QmldirFile>(
                        v->current, now, v->revision(), v->lastDataUpdateAt()));
                newValue = newV;
                QMutexLocker l(mutex());
                auto it = m_qmldirFileWithPath.find(canonicalFilePath);
                if (it != m_qmldirFileWithPath.end())
                    oldValue = newValue = *it;
                else
                    m_qmldirFileWithPath.insert(canonicalFilePath, newV);
            }
        }
        if (!newValue) {
            self.universe().loadFile(
                    canonicalFilePath, logicalPath, code, codeDate,
                    callbackForQmldirFile(self, loadCallback, directDepsCallback, endCallback),
                    loadOptions, fType);
            return;
        }
    } break;
    default: {
        myErrors().error(tr("Unexpected file to load: '%1'").arg(filePath)).handle(h);
        if (loadCallback)
            loadCallback(self.canonicalPath(), DomItem::empty, DomItem::empty);
        if (directDepsCallback)
            directDepsCallback(self.canonicalPath(), DomItem::empty, DomItem::empty);
        if (endCallback)
            endCallback(self.canonicalPath(), DomItem::empty, DomItem::empty);
        return;
    } break;
    }
    Path p = self.copy(newValue).canonicalPath();
    std::shared_ptr<LoadInfo> lInfo = loadInfo(p);
    if (lInfo) {
        if (loadCallback) {
            DomItem oldValueObj = self.copy(oldValue);
            DomItem newValueObj = self.copy(newValue);
            loadCallback(p, oldValueObj, newValueObj);
        }
        if (directDepsCallback) {
            DomItem lInfoObj = self.copy(lInfo);
            lInfo->addEndCallback(lInfoObj, directDepsCallback);
        }
    } else {
        self.addError(myErrors().error(tr("missing load info in ")));
        if (loadCallback)
            loadCallback(self.canonicalPath(), DomItem::empty, DomItem::empty);
        if (directDepsCallback)
            directDepsCallback(self.canonicalPath(), DomItem::empty, DomItem::empty);
    }
    if (endCallback)
        addAllLoadedCallback(self, [p, endCallback](Path, DomItem &, DomItem &env) {
            DomItem el = env.path(p);
            endCallback(p, el, el);
        });
}

void DomEnvironment::loadModuleDependency(DomItem &self, QString uri, Version v,
                                          Callback loadCallback, Callback endCallback,
                                          ErrorHandler errorHandler)
{
    if (uri.startsWith(u"file://") || uri.startsWith(u"http://") || uri.startsWith(u"https://")) {
        self.addError(myErrors().error(tr("directory import not yet handled (%1)").arg(uri)));
        return;
    }
    Path p = Paths::moduleIndexPath(uri, v.majorVersion);
    if (v.majorVersion == Version::Latest) {
        // load both the latest .<version> directory, and the common one
        QStringList subPathComponents = uri.split(QLatin1Char('.'));
        int maxV = -1;
        bool commonV = false;
        QString lastComponent = subPathComponents.last();
        subPathComponents.removeLast();
        QString subPathV = subPathComponents.join(u'/');
        QRegularExpression vRe(QRegularExpression::anchoredPattern(
                QRegularExpression::escape(lastComponent) + QStringLiteral(u"\\.([0-9]*)")));
        for (QString path : loadPaths()) {
            QDir dir(path + (subPathV.isEmpty() ? QStringLiteral(u"") : QStringLiteral(u"/"))
                     + subPathV);
            for (QString dirNow : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
                auto m = vRe.match(dirNow);
                if (m.hasMatch()) {
                    int majorV = m.captured(1).toInt();
                    if (majorV > maxV) {
                        QFileInfo fInfo(dir.canonicalPath() + QChar(u'/') + dirNow
                                        + QStringLiteral(u"/qmldir"));
                        if (fInfo.isFile())
                            maxV = majorV;
                    }
                }
                if (!commonV && dirNow == lastComponent) {
                    QFileInfo fInfo(dir.canonicalPath() + QChar(u'/') + dirNow
                                    + QStringLiteral(u"/qmldir"));
                    if (fInfo.isFile())
                        commonV = true;
                }
            }
        }
        QAtomicInt toLoad((commonV ? 1 : 0) + ((maxV >= 0) ? 1 : 0));
        auto loadCallback2 = (loadCallback ? [p, loadCallback, toLoad](Path, DomItem &, DomItem &elV) mutable {
            if (--toLoad == 0) {
                DomItem el = elV.path(p);
                loadCallback(p, el, el);
            }
        }: Callback());
        if (maxV >= 0)
            loadModuleDependency(self, uri, Version(maxV, v.minorVersion), loadCallback2, nullptr);
        if (commonV)
            loadModuleDependency(self, uri, Version(Version::Undefined, v.minorVersion),
                                 loadCallback2, nullptr);
        else if (maxV < 0) {
            if (uri != u"QML") {
                addErrorLocal(myErrors()
                                      .warning(tr("Failed to find main qmldir file for %1 %2")
                                                       .arg(uri, v.stringValue()))
                                      .handle());
            }
            if (loadCallback)
                loadCallback(p, DomItem::empty, DomItem::empty);
        }
    } else {
        std::shared_ptr<ModuleIndex> mIndex = moduleIndexWithUri(
                self, uri, v.majorVersion, EnvLookup::Normal, Changeable::Writable, errorHandler);
        std::shared_ptr<LoadInfo> lInfo = loadInfo(p);
        if (lInfo) {
            DomItem lInfoObj = self.copy(lInfo);
            lInfo->addEndCallback(lInfoObj, loadCallback);
        } else {
            addErrorLocal(
                    myErrors().warning(tr("Missing loadInfo for %1").arg(p.toString())).handle());
            if (loadCallback)
                loadCallback(p, DomItem::empty, DomItem::empty);
        }
    }
    if (endCallback)
        addAllLoadedCallback(self, [p, endCallback](Path, DomItem &, DomItem &env) {
            DomItem el = env.path(p);
            endCallback(p, el, el);
        });
}

void DomEnvironment::loadBuiltins(DomItem &self, Callback callback, ErrorHandler h)
{
    QString builtinsName = QLatin1String("builtins.qmltypes");
    for (QString path : loadPaths()) {
        QDir dir(path);
        QFileInfo fInfo(dir.filePath(builtinsName));
        if (fInfo.isFile()) {
            self.loadFile(fInfo.canonicalFilePath(), QString(), callback, LoadOption::DefaultLoad);
            return;
        }
    }
    myErrors().error(tr("Could not find builtins.qmltypes file")).handle(h);
}

shared_ptr<DomUniverse> DomEnvironment::universe() const {
    if (m_universe)
        return m_universe;
    else if (m_base)
        return m_base->universe();
    else
        return {};
}

template<typename T>
QSet<QString> DomEnvironment::getStrings(function_ref<QSet<QString>()> getBase,
                                         const QMap<QString, T> &selfMap, EnvLookup options) const
{
    QSet<QString> res;
    if (options != EnvLookup::NoBase && m_base) {
        if (m_base)
            res = getBase();
    }
    if (options != EnvLookup::BaseOnly) {
        QMap<QString, T> map;
        {
            QMutexLocker l(mutex());
            map = selfMap;
        }
        auto it = map.keyBegin();
        auto end = map.keyEnd();
        while (it != end) {
            res += *it;
            ++it;
        }
    }
    return res;
}

QSet<QString> DomEnvironment::moduleIndexUris(DomItem &, EnvLookup lookup) const
{
    DomItem baseObj = DomItem(m_base);
    return this->getStrings<QMap<int, std::shared_ptr<ModuleIndex>>>(
            [this, &baseObj] { return m_base->moduleIndexUris(baseObj, EnvLookup::Normal); },
            m_moduleIndexWithUri, lookup);
}

QSet<int> DomEnvironment::moduleIndexMajorVersions(DomItem &, QString uri, EnvLookup lookup) const
{
    QSet<int> res;
    if (lookup != EnvLookup::NoBase && m_base) {
        DomItem baseObj(m_base);
        res = m_base->moduleIndexMajorVersions(baseObj, uri, EnvLookup::Normal);
    }
    if (lookup != EnvLookup::BaseOnly) {
        QMap<int, std::shared_ptr<ModuleIndex>> map;
        {
            QMutexLocker l(mutex());
            map = m_moduleIndexWithUri.value(uri);
        }
        auto it = map.keyBegin();
        auto end = map.keyEnd();
        while (it != end) {
            res += *it;
            ++it;
        }
    }
    return res;
}

std::shared_ptr<ModuleIndex> DomEnvironment::moduleIndexWithUri(DomItem &self, QString uri,
                                                                int majorVersion, EnvLookup options,
                                                                Changeable changeable,
                                                                ErrorHandler errorHandler)
{
    Q_ASSERT((changeable == Changeable::ReadOnly
              || (majorVersion >= 0 || majorVersion == Version::Undefined))
             && "A writeable moduleIndexWithUri call should have a version (not with "
                "Version::Latest)");
    std::shared_ptr<ModuleIndex> res;
    if (changeable == Changeable::Writable && (m_options & Option::Exported))
        myErrors().error(tr("A mutable module was requested in a multithreaded environment")).handle(errorHandler);
    if (options != EnvLookup::BaseOnly) {
        QMutexLocker l(mutex());
        auto it = m_moduleIndexWithUri.find(uri);
        if (it != m_moduleIndexWithUri.end()) {
            if (majorVersion == Version::Latest) {
                auto begin = it->begin();
                auto end = it->end();
                if (begin != end)
                    res = *--end;
            } else {
                auto it2 = it->find(majorVersion);
                if (it2 != it->end())
                    return *it2;
            }
        }
    }
    std::shared_ptr<ModuleIndex> newModulePtr;
    if (options != EnvLookup::NoBase && m_base) {
        std::shared_ptr<ModuleIndex> existingMod =
                m_base->moduleIndexWithUri(self, uri, majorVersion, options, Changeable::ReadOnly);
        if (res && majorVersion == Version::Latest
            && (!existingMod || res->majorVersion() >= existingMod->majorVersion()))
            return res;
        if (changeable == Changeable::Writable) {
            DomItem existingModObj = self.copy(existingMod);
            newModulePtr = existingMod->makeCopy(existingModObj);
        } else {
            return existingMod;
        }
    }
    if (!newModulePtr && res)
        return res;
    if (!newModulePtr && changeable == Changeable::Writable)
        newModulePtr = std::shared_ptr<ModuleIndex>(new ModuleIndex(uri, majorVersion));
    if (newModulePtr) {
        DomItem newModule = self.copy(newModulePtr);
        Path p = newModule.canonicalPath();
        {
            QMutexLocker l(mutex());
            auto &modsNow = m_moduleIndexWithUri[uri];
            if (modsNow.contains(majorVersion))
                return modsNow.value(majorVersion);
            modsNow.insert(majorVersion, newModulePtr);
        }
        if (p) {
            std::shared_ptr<LoadInfo> lInfo(new LoadInfo(p));
            addLoadInfo(self, lInfo);
        } else {
            myErrors()
                    .error(tr("Could not get path for newly created ModuleIndex %1 %2")
                                   .arg(uri)
                                   .arg(majorVersion))
                    .handle(errorHandler);
        }
    }
    return newModulePtr;
}

std::shared_ptr<ModuleIndex> DomEnvironment::moduleIndexWithUri(DomItem &self, QString uri,
                                                                int majorVersion,
                                                                EnvLookup options) const
{
    std::shared_ptr<ModuleIndex> res;
    if (options != EnvLookup::BaseOnly) {
        QMutexLocker l(mutex());
        auto it = m_moduleIndexWithUri.find(uri);
        if (it != m_moduleIndexWithUri.end()) {
            if (majorVersion == Version::Latest) {
                auto begin = it->begin();
                auto end = it->end();
                if (begin != end)
                    res = *--end;
            } else {
                auto it2 = it->find(majorVersion);
                if (it2 != it->end())
                    return *it2;
            }
        }
    }
    if (options != EnvLookup::NoBase && m_base) {
        std::shared_ptr existingMod =
                m_base->moduleIndexWithUri(self, uri, majorVersion, options, Changeable::ReadOnly);
        if (res && majorVersion == Version::Latest
            && (!existingMod || res->majorVersion() >= existingMod->majorVersion())) {
            return res;
        }
        return existingMod;
    }
    return res;
}

std::shared_ptr<ExternalItemInfo<QmlDirectory>>
DomEnvironment::qmlDirectoryWithPath(DomItem &self, QString path, EnvLookup options) const
{
    if (options != EnvLookup::BaseOnly) {
        QMutexLocker l(mutex());
        if (m_qmlDirectoryWithPath.contains(path))
            return m_qmlDirectoryWithPath.value(path);
    }
    if (options != EnvLookup::NoBase && m_base) {
        return m_base->qmlDirectoryWithPath(self, path, options);
    }
    return {};
}

QSet<QString> DomEnvironment::qmlDirectoryPaths(DomItem &, EnvLookup options) const
{
    return getStrings<std::shared_ptr<ExternalItemInfo<QmlDirectory>>>(
            [this] {
                DomItem baseObj(m_base);
                return m_base->qmlDirectoryPaths(baseObj, EnvLookup::Normal);
            },
            m_qmlDirectoryWithPath, options);
}

std::shared_ptr<ExternalItemInfo<QmldirFile>>
DomEnvironment::qmldirFileWithPath(DomItem &self, QString path, EnvLookup options) const
{
    if (options != EnvLookup::BaseOnly) {
        QMutexLocker l(mutex());
        auto it = m_qmldirFileWithPath.find(path);
        if (it != m_qmldirFileWithPath.end())
            return *it;
    }
    if (options != EnvLookup::NoBase && m_base)
        return m_base->qmldirFileWithPath(self, path, options);
    return {};
}

QSet<QString> DomEnvironment::qmldirFilePaths(DomItem &, EnvLookup lOptions) const
{
    return getStrings<std::shared_ptr<ExternalItemInfo<QmldirFile>>>(
            [this] {
                DomItem baseObj(m_base);
                return m_base->qmldirFilePaths(baseObj, EnvLookup::Normal);
            },
            m_qmldirFileWithPath, lOptions);
}

std::shared_ptr<ExternalItemInfoBase> DomEnvironment::qmlDirWithPath(DomItem &self, QString path,
                                                                     EnvLookup options) const
{
    if (auto qmldirFile = qmldirFileWithPath(self, path + QLatin1String("/qmldir"), options))
        return qmldirFile;
    return qmlDirectoryWithPath(self, path, options);
}

QSet<QString> DomEnvironment::qmlDirPaths(DomItem &self, EnvLookup options) const
{
    QSet<QString> res = qmlDirectoryPaths(self, options);
    for (QString p : qmldirFilePaths(self, options)) {
        if (p.endsWith(u"/qmldir")) {
            res.insert(p.left(p.length() - 7));
        } else {
            myErrors()
                    .warning(tr("Unexpected path not ending with qmldir in qmldirFilePaths: %1")
                                     .arg(p))
                    .handle();
        }
    }
    return res;
}

std::shared_ptr<ExternalItemInfo<QmlFile>>
DomEnvironment::qmlFileWithPath(DomItem &self, QString path, EnvLookup options) const
{
    if (options != EnvLookup::BaseOnly) {
        QMutexLocker l(mutex());
        auto it = m_qmlFileWithPath.find(path);
        if (it != m_qmlFileWithPath.end())
            return *it;
    }
    if (options != EnvLookup::NoBase && m_base)
        return m_base->qmlFileWithPath(self, path, options);
    return {};
}

QSet<QString> DomEnvironment::qmlFilePaths(DomItem &, EnvLookup lookup) const
{
    return getStrings<std::shared_ptr<ExternalItemInfo<QmlFile>>>(
            [this] {
                DomItem baseObj(m_base);
                return m_base->qmlFilePaths(baseObj, EnvLookup::Normal);
            },
            m_qmlFileWithPath, lookup);
}

std::shared_ptr<ExternalItemInfo<JsFile>>
DomEnvironment::jsFileWithPath(DomItem &self, QString path, EnvLookup options) const
{
    if (options != EnvLookup::BaseOnly) {
        QMutexLocker l(mutex());
        if (m_jsFileWithPath.contains(path))
            return m_jsFileWithPath.value(path);
    }
    if (options != EnvLookup::NoBase && m_base)
        return m_base->jsFileWithPath(self, path, EnvLookup::Normal);
    return {};
}

QSet<QString> DomEnvironment::jsFilePaths(DomItem &, EnvLookup lookup) const
{
    return getStrings<std::shared_ptr<ExternalItemInfo<JsFile>>>(
            [this] {
                DomItem baseObj(m_base);
                return m_base->jsFilePaths(baseObj, EnvLookup::Normal);
            },
            m_jsFileWithPath, lookup);
}

std::shared_ptr<ExternalItemInfo<QmltypesFile>>
DomEnvironment::qmltypesFileWithPath(DomItem &self, QString path, EnvLookup options) const
{
    if (options != EnvLookup::BaseOnly) {
        QMutexLocker l(mutex());
        if (m_qmltypesFileWithPath.contains(path))
            return m_qmltypesFileWithPath.value(path);
    }
    if (options != EnvLookup::NoBase && m_base)
        return m_base->qmltypesFileWithPath(self, path, EnvLookup::Normal);
    return {};
}

QSet<QString> DomEnvironment::qmltypesFilePaths(DomItem &, EnvLookup lookup) const
{
    return getStrings<std::shared_ptr<ExternalItemInfo<QmltypesFile>>>(
            [this] {
                DomItem baseObj(m_base);
                return m_base->qmltypesFilePaths(baseObj, EnvLookup::Normal);
            },
            m_qmltypesFileWithPath, lookup);
}

std::shared_ptr<ExternalItemInfo<GlobalScope>>
DomEnvironment::globalScopeWithName(DomItem &self, QString name, EnvLookup lookupOptions) const
{
    if (lookupOptions != EnvLookup::BaseOnly) {
        QMutexLocker l(mutex());
        auto id = m_globalScopeWithName.find(name);
        if (id != m_globalScopeWithName.end())
            return *id;
    }
    if (lookupOptions != EnvLookup::NoBase && m_base)
        return m_base->globalScopeWithName(self, name, lookupOptions);
    return {};
}

std::shared_ptr<ExternalItemInfo<GlobalScope>>
DomEnvironment::ensureGlobalScopeWithName(DomItem &self, QString name, EnvLookup lookupOptions)
{
    if (auto current = globalScopeWithName(self, name, lookupOptions))
        return current;
    if (auto u = universe()) {
        if (auto newVal = u->ensureGlobalScopeWithName(name)) {
            if (auto current = newVal->current) {
                DomItem currentObj = DomItem(u).copy(current);
                auto newScope = current->makeCopy(currentObj);
                std::shared_ptr<ExternalItemInfo<GlobalScope>> newCopy(
                        new ExternalItemInfo<GlobalScope>(newScope));
                QMutexLocker l(mutex());
                if (auto oldVal = m_globalScopeWithName.value(name))
                    return oldVal;
                m_globalScopeWithName.insert(name, newCopy);
                return newCopy;
            }
        }
    }
    Q_ASSERT_X(false, "DomEnvironment::ensureGlobalScopeWithName", "could not ensure globalScope");
    return {};
}

QSet<QString> DomEnvironment::globalScopeNames(DomItem &, EnvLookup lookupOptions) const
{
    QSet<QString> res;
    if (lookupOptions != EnvLookup::NoBase && m_base) {
        if (m_base) {
            DomItem baseObj(m_base);
            res = m_base->globalScopeNames(baseObj, EnvLookup::Normal);
        }
    }
    if (lookupOptions != EnvLookup::BaseOnly) {
        QMap<QString, std::shared_ptr<ExternalItemInfo<GlobalScope>>> map;
        {
            QMutexLocker l(mutex());
            map = m_globalScopeWithName;
        }
        auto it = map.keyBegin();
        auto end = map.keyEnd();
        while (it != end) {
            res += *it;
            ++it;
        }
    }
    return res;
}

void DomEnvironment::addLoadInfo(DomItem &self, std::shared_ptr<LoadInfo> loadInfo)
{
    if (!loadInfo)
        return;
    Path p = loadInfo->elementCanonicalPath();
    bool addWork = loadInfo->status() != LoadInfo::Status::Done;
    std::shared_ptr<LoadInfo> oldVal;
    {
        QMutexLocker l(mutex());
        oldVal = m_loadInfos.value(p);
        m_loadInfos.insert(p, loadInfo);
        if (addWork)
            m_loadsWithWork.enqueue(p);
    }
    if (oldVal && oldVal->status() != LoadInfo::Status::Done) {
        self.addError(myErrors()
                              .error(tr("addLoadinfo replaces unfinished load info for %1")
                                             .arg(p.toString()))
                              .handle());
    }
}

std::shared_ptr<LoadInfo> DomEnvironment::loadInfo(Path path) const
{
    QMutexLocker l(mutex());
    return m_loadInfos.value(path);
}

QHash<Path, std::shared_ptr<LoadInfo>> DomEnvironment::loadInfos() const
{
    QMutexLocker l(mutex());
    return m_loadInfos;
}

QList<Path> DomEnvironment::loadInfoPaths() const
{
    auto lInfos = loadInfos();
    return lInfos.keys();
}

DomItem::Callback DomEnvironment::callbackForQmlDirectory(DomItem &self, Callback loadCallback,
                                                          Callback allDirectDepsCallback,
                                                          Callback endCallback)
{
    return envCallbackForFile<QmlDirectory>(self, &DomEnvironment::m_qmlDirectoryWithPath,
                                            &DomEnvironment::qmlDirectoryWithPath, loadCallback,
                                            allDirectDepsCallback, endCallback);
}

DomItem::Callback DomEnvironment::callbackForQmlFile(DomItem &self, Callback loadCallback,
                                                     Callback allDirectDepsCallback,
                                                     Callback endCallback)
{
    return envCallbackForFile<QmlFile>(self, &DomEnvironment::m_qmlFileWithPath,
                                       &DomEnvironment::qmlFileWithPath, loadCallback,
                                       allDirectDepsCallback, endCallback);
}

DomTop::Callback DomEnvironment::callbackForQmltypesFile(DomItem &self,
                                                         DomTop::Callback loadCallback,
                                                         Callback allDirectDepsCallback,
                                                         DomTop::Callback endCallback)
{
    return envCallbackForFile<QmltypesFile>(
            self, &DomEnvironment::m_qmltypesFileWithPath, &DomEnvironment::qmltypesFileWithPath,
            [loadCallback](Path p, DomItem &oldV, DomItem &newV) {
                DomItem newFile = newV.field(Fields::currentItem);
                if (std::shared_ptr<QmltypesFile> newFilePtr = newFile.ownerAs<QmltypesFile>())
                    newFilePtr->ensureInModuleIndex(newFile);
                if (loadCallback)
                    loadCallback(p, oldV, newV);
            },
            allDirectDepsCallback, endCallback);
}

DomTop::Callback DomEnvironment::callbackForQmldirFile(DomItem &self, DomTop::Callback loadCallback,
                                                       Callback allDirectDepsCallback,
                                                       DomTop::Callback endCallback)
{
    return envCallbackForFile<QmldirFile>(self, &DomEnvironment::m_qmldirFileWithPath,
                                          &DomEnvironment::qmldirFileWithPath, loadCallback,
                                          allDirectDepsCallback, endCallback);
}

DomEnvironment::DomEnvironment(QStringList loadPaths, Options options,
                               shared_ptr<DomUniverse> universe)
    : m_options(options),
      m_universe(DomUniverse::guaranteeUniverse(universe)),
      m_loadPaths(loadPaths),
      m_implicitImports(defaultImplicitImports())
{}

DomItem DomEnvironment::create(QStringList loadPaths, Options options, DomItem &universe)
{
    std::shared_ptr<DomUniverse> universePtr = universe.ownerAs<DomUniverse>();
    std::shared_ptr<DomEnvironment> envPtr(new DomEnvironment(loadPaths, options, universePtr));
    return DomItem(envPtr);
}

DomEnvironment::DomEnvironment(shared_ptr<DomEnvironment> parent, QStringList loadPaths,
                               Options options)
    : m_options(options),
      m_base(parent),
      m_loadPaths(loadPaths),
      m_implicitImports(defaultImplicitImports())
{}

template<typename T>
std::shared_ptr<ExternalItemInfo<T>>
addExternalItem(std::shared_ptr<T> file, QString key,
                QMap<QString, std::shared_ptr<ExternalItemInfo<T>>> &map, AddOption option,
                QBasicMutex *mutex)
{
    if (!file)
        return {};
    std::shared_ptr<ExternalItemInfo<T>> eInfo(
            new ExternalItemInfo<T>(file, QDateTime::currentDateTime()));
    {
        QMutexLocker l(mutex);
        auto it = map.find(key);
        if (it != map.end()) {
            switch (option) {
            case AddOption::KeepExisting:
                eInfo = *it;
                break;
            case AddOption::Overwrite:
                map.insert(key, eInfo);
                break;
            }
        } else {
            map.insert(key, eInfo);
        }
    }
    return eInfo;
}

std::shared_ptr<ExternalItemInfo<QmlFile>> DomEnvironment::addQmlFile(std::shared_ptr<QmlFile> file,
                                                                      AddOption options)
{
    return addExternalItem<QmlFile>(file, file->canonicalFilePath(), m_qmlFileWithPath, options,
                                    mutex());
}

std::shared_ptr<ExternalItemInfo<QmlDirectory>>
DomEnvironment::addQmlDirectory(std::shared_ptr<QmlDirectory> file, AddOption options)
{
    return addExternalItem<QmlDirectory>(file, file->canonicalFilePath(), m_qmlDirectoryWithPath,
                                         options, mutex());
}

std::shared_ptr<ExternalItemInfo<QmldirFile>>
DomEnvironment::addQmldirFile(std::shared_ptr<QmldirFile> file, AddOption options)
{
    return addExternalItem<QmldirFile>(file, file->canonicalFilePath(), m_qmldirFileWithPath,
                                       options, mutex());
}

std::shared_ptr<ExternalItemInfo<QmltypesFile>>
DomEnvironment::addQmltypesFile(std::shared_ptr<QmltypesFile> file, AddOption options)
{
    return addExternalItem<QmltypesFile>(file, file->canonicalFilePath(), m_qmltypesFileWithPath,
                                         options, mutex());
}

std::shared_ptr<ExternalItemInfo<JsFile>> DomEnvironment::addJsFile(std::shared_ptr<JsFile> file,
                                                                    AddOption options)
{
    return addExternalItem<JsFile>(file, file->canonicalFilePath(), m_jsFileWithPath, options,
                                   mutex());
}

std::shared_ptr<ExternalItemInfo<GlobalScope>>
DomEnvironment::addGlobalScope(std::shared_ptr<GlobalScope> scope, AddOption options)
{
    return addExternalItem<GlobalScope>(scope, scope->name(), m_globalScopeWithName, options,
                                        mutex());
}

bool DomEnvironment::commitToBase(DomItem &self)
{
    if (!base())
        return false;
    QMap<QString, QMap<int, std::shared_ptr<ModuleIndex>>> my_moduleIndexWithUri;
    QMap<QString, std::shared_ptr<ExternalItemInfo<GlobalScope>>> my_globalScopeWithName;
    QMap<QString, std::shared_ptr<ExternalItemInfo<QmlDirectory>>> my_qmlDirectoryWithPath;
    QMap<QString, std::shared_ptr<ExternalItemInfo<QmldirFile>>> my_qmldirFileWithPath;
    QMap<QString, std::shared_ptr<ExternalItemInfo<QmlFile>>> my_qmlFileWithPath;
    QMap<QString, std::shared_ptr<ExternalItemInfo<JsFile>>> my_jsFileWithPath;
    QMap<QString, std::shared_ptr<ExternalItemInfo<QmltypesFile>>> my_qmltypesFileWithPath;
    QHash<Path, std::shared_ptr<LoadInfo>> my_loadInfos;
    {
        QMutexLocker l(mutex());
        my_moduleIndexWithUri = m_moduleIndexWithUri;
        my_globalScopeWithName = m_globalScopeWithName;
        my_qmlDirectoryWithPath = m_qmlDirectoryWithPath;
        my_qmldirFileWithPath = m_qmldirFileWithPath;
        my_qmlFileWithPath = m_qmlFileWithPath;
        my_jsFileWithPath = m_jsFileWithPath;
        my_qmltypesFileWithPath = m_qmltypesFileWithPath;
        my_loadInfos = m_loadInfos;
    }
    {
        QMutexLocker lBase(base()->mutex()); // be more careful about makeCopy calls with lock?
        m_base->m_globalScopeWithName.insert(my_globalScopeWithName);
        m_base->m_qmlDirectoryWithPath.insert(my_qmlDirectoryWithPath);
        m_base->m_qmldirFileWithPath.insert(my_qmldirFileWithPath);
        m_base->m_qmlFileWithPath.insert(my_qmlFileWithPath);
        m_base->m_jsFileWithPath.insert(my_jsFileWithPath);
        m_base->m_qmltypesFileWithPath.insert(my_qmltypesFileWithPath);
        m_base->m_loadInfos.insert(my_loadInfos);
        {
            auto it = my_moduleIndexWithUri.cbegin();
            auto end = my_moduleIndexWithUri.cend();
            while (it != end) {
                QMap<int, shared_ptr<ModuleIndex>> &myVersions =
                        m_base->m_moduleIndexWithUri[it.key()];
                auto it2 = it.value().cbegin();
                auto end2 = it.value().cend();
                while (it2 != end2) {
                    auto oldV = myVersions.value(it2.key());
                    DomItem it2Obj = self.copy(it2.value());
                    auto newV = it2.value()->makeCopy(it2Obj);
                    newV->mergeWith(oldV);
                    myVersions.insert(it2.key(), newV);
                    ++it2;
                }
                ++it;
            }
        }
    }
    return true;
}

void DomEnvironment::loadPendingDependencies(DomItem &self)
{
    while (true) {
        Path elToDo;
        std::shared_ptr<LoadInfo> loadInfo;
        {
            QMutexLocker l(mutex());
            if (m_loadsWithWork.isEmpty())
                break;
            elToDo = m_loadsWithWork.dequeue();
            m_inProgress.append(elToDo);
            loadInfo = m_loadInfos.value(elToDo);
        }
        if (loadInfo) {
            auto cleanup = qScopeGuard([this, elToDo, &self] {
                QList<Callback> endCallbakcs;
                {
                    QMutexLocker l(mutex());
                    m_inProgress.removeOne(elToDo);
                    if (m_inProgress.isEmpty() && m_loadsWithWork.isEmpty()) {
                        endCallbakcs = m_allLoadedCallback;
                        m_allLoadedCallback.clear();
                    }
                }
                for (Callback cb : endCallbakcs)
                    cb(self.canonicalPath(), self, self);
            });
            DomItem loadInfoObj = self.copy(loadInfo);
            loadInfo->advanceLoad(loadInfoObj);
        } else {
            self.addError(myErrors().error(u"DomEnvironment::loadPendingDependencies could not "
                                           u"find loadInfo listed in m_loadsWithWork"));
            {
                QMutexLocker l(mutex());
                m_inProgress.removeOne(elToDo);
            }
            Q_ASSERT(false
                     && "DomEnvironment::loadPendingDependencies could not find loadInfo listed in "
                        "m_loadsWithWork");
        }
    }
}

bool DomEnvironment::finishLoadingDependencies(DomItem &self, int waitMSec)
{
    bool hasPendingLoads = true;
    QDateTime endTime = QDateTime::currentDateTime().addMSecs(waitMSec);
    for (int i = 0; i < waitMSec / 10 + 2; ++i) {
        loadPendingDependencies(self);
        auto lInfos = loadInfos();
        auto it = lInfos.cbegin();
        auto end = lInfos.cend();
        hasPendingLoads = false;
        while (it != end) {
            if (*it && (*it)->status() != LoadInfo::Status::Done)
                hasPendingLoads = true;
        }
        if (!hasPendingLoads)
            break;
        auto missing = QDateTime::currentDateTime().msecsTo(endTime);
        if (missing < 0)
            break;
        if (missing > 100)
            missing = 100;
#if QT_FEATURE_thread
        QThread::msleep(missing);
#endif
    }
    return !hasPendingLoads;
}

void DomEnvironment::addWorkForLoadInfo(Path elementCanonicalPath)
{
    QMutexLocker l(mutex());
    m_loadsWithWork.enqueue(elementCanonicalPath);
}

DomEnvironment::Options DomEnvironment::options() const
{
    return m_options;
}

std::shared_ptr<DomEnvironment> DomEnvironment::base() const
{
    return m_base;
}

QStringList DomEnvironment::loadPaths() const
{
    QMutexLocker l(mutex());
    return m_loadPaths;
}

QString DomEnvironment::globalScopeName() const
{
    return m_globalScopeName;
}

QList<Import> DomEnvironment::defaultImplicitImports()
{
    return QList<Import>({ Import::fromUriString(QLatin1String("QML"), Version(1, 0)),
                           Import(QLatin1String("QtQml"), Version(6, 0)) });
}

QList<Import> DomEnvironment::implicitImports() const
{
    return m_implicitImports;
}

void DomEnvironment::addAllLoadedCallback(DomItem &self, DomTop::Callback c)
{
    if (c) {
        bool immediate = false;
        {
            QMutexLocker l(mutex());
            if (m_loadsWithWork.isEmpty() && m_inProgress.isEmpty())
                immediate = true;
            else
                m_allLoadedCallback.append(c);
        }
        if (immediate)
            c(Path(), self, self);
    }
}

void DomEnvironment::clearReferenceCache()
{
    m_referenceCache.clear();
}

QString ExternalItemInfoBase::canonicalFilePath(DomItem &self) const
{
    shared_ptr<ExternalOwningItem> current = currentItem();
    DomItem currentObj = currentItem(self);
    return current->canonicalFilePath(currentObj);
}

bool ExternalItemInfoBase::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    if (!self.dvValueLazyField(visitor, Fields::currentRevision,
                               [this, &self]() { return currentRevision(self); }))
        return false;
    if (!self.dvValueLazyField(visitor, Fields::lastRevision,
                               [this, &self]() { return lastRevision(self); }))
        return false;
    if (!self.dvValueLazyField(visitor, Fields::lastValidRevision,
                               [this, &self]() { return lastValidRevision(self); }))
        return false;
    if (!visitor(PathEls::Field(Fields::currentItem),
                 [&self, this]() { return currentItem(self); }))
        return false;
    if (!self.dvValueLazyField(visitor, Fields::currentExposedAt,
                               [this]() { return currentExposedAt(); }))
        return false;
    return true;
}

int ExternalItemInfoBase::currentRevision(DomItem &) const
{
    return currentItem()->revision();
}

int ExternalItemInfoBase::lastRevision(DomItem &self) const
{
    Path p = currentItem()->canonicalPath();
    DomItem lastValue = self.universe()[p.mid(1, p.length() - 1)].field(u"revision");
    return static_cast<int>(lastValue.value().toInteger(0));
}

int ExternalItemInfoBase::lastValidRevision(DomItem &self) const
{
    Path p = currentItem()->canonicalPath();
    DomItem lastValidValue = self.universe()[p.mid(1, p.length() - 2)].field(u"validItem").field(u"revision");
    return static_cast<int>(lastValidValue.value().toInteger(0));
}

QString ExternalItemPairBase::canonicalFilePath(DomItem &) const
{
    shared_ptr<ExternalOwningItem> current = currentItem();
    return current->canonicalFilePath();
}

Path ExternalItemPairBase::canonicalPath(DomItem &) const
{
    shared_ptr<ExternalOwningItem> current = currentItem();
    return current->canonicalPath().dropTail();
}

bool ExternalItemPairBase::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    if (!self.dvValueLazyField(visitor, Fields::currentIsValid,
                               [this]() { return currentIsValid(); }))
        return false;
    if (!visitor(PathEls::Field(Fields::validItem), [this, &self]() { return validItem(self); }))
        return false;
    if (!visitor(PathEls::Field(Fields::currentItem),
                 [this, &self]() { return currentItem(self); }))
        return false;
    if (!self.dvValueField(visitor, Fields::validExposedAt, validExposedAt))
        return false;
    if (!self.dvValueField(visitor, Fields::currentExposedAt, currentExposedAt))
        return false;
    return true;
}

bool ExternalItemPairBase::currentIsValid() const
{
    return currentItem() == validItem();
}

RefCacheEntry RefCacheEntry::forPath(DomItem &el, Path canonicalPath)
{
    DomItem env = el.environment();
    std::shared_ptr<DomEnvironment> envPtr = env.ownerAs<DomEnvironment>();
    RefCacheEntry cached;
    if (envPtr) {
        QMutexLocker l(envPtr->mutex());
        cached = envPtr->m_referenceCache.value(canonicalPath, {});
    } else {
        Q_ASSERT(false);
    }
    return cached;
}

bool RefCacheEntry::addForPath(DomItem &el, Path canonicalPath, const RefCacheEntry &entry,
                               AddOption addOption)
{
    DomItem env = el.environment();
    std::shared_ptr<DomEnvironment> envPtr = env.ownerAs<DomEnvironment>();
    bool didSet = false;
    if (envPtr) {
        QMutexLocker l(envPtr->mutex());
        RefCacheEntry &cached = envPtr->m_referenceCache[canonicalPath];
        switch (cached.cached) {
        case RefCacheEntry::Cached::None:
            cached = entry;
            didSet = true;
            break;
        case RefCacheEntry::Cached::First:
            if (addOption == AddOption::Overwrite || entry.cached == RefCacheEntry::Cached::All) {
                cached = entry;
                didSet = true;
            }
            break;
        case RefCacheEntry::Cached::All:
            if (addOption == AddOption::Overwrite || entry.cached == RefCacheEntry::Cached::All) {
                cached = entry;
                didSet = true;
            }
        }
        if (cached.cached == RefCacheEntry::Cached::First && cached.canonicalPaths.isEmpty())
            cached.cached = RefCacheEntry::Cached::All;
    } else {
        Q_ASSERT(false);
    }
    return didSet;
}

} // end namespace Dom
} // end namespace QQmlJS

QT_END_NAMESPACE
