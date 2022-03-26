/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
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

//! [0]
QFuture<QString> future = ...;

QFuture<QString>::const_iterator i;
for (i = future.constBegin(); i != future.constEnd(); ++i)
    cout << *i << Qt::endl;
//! [0]


//! [1]
QFuture<QString> future;
...
QFutureIterator<QString> i(future);
while (i.hasNext())
    QString s = i.next();
//! [1]


//! [2]
QFutureIterator<QString> i(future);
i.toBack();
while (i.hasPrevious())
    QString s = i.previous();
//! [2]

//! [3]
using NetworkReply = std::variant<QByteArray, QNetworkReply::NetworkError>;

enum class IOError { FailedToRead, FailedToWrite };
using IOResult = std::variant<QString, IOError>;
//! [3]

//! [4]
QFuture<IOResult> future = QtConcurrent::run([url] {
        ...
        return NetworkReply(QNetworkReply::TimeoutError);
}).then([](NetworkReply reply) {
    if (auto error = std::get_if<QNetworkReply::NetworkError>(&reply))
        return IOResult(IOError::FailedToRead);

    auto data = std::get_if<QByteArray>(&reply);
    // try to write *data and return IOError::FailedToWrite on failure
    ...
});

auto result = future.result();
if (auto filePath = std::get_if<QString>(&result)) {
    // do something with *filePath
else
    // process the error
//! [4]

//! [5]
QFuture<int> future = ...;
    future.then([](QFuture<int> f) {
        try {
            ...
            auto result = f.result();
            ...
        } catch (QException &e) {
            // handle the exception
        }
    }).then(...);
//! [5]

//! [6]
QFuture<int> future = ...;
auto continuation = future.then([](int res1){ ... }).then([](int res2){ ... })...
...
// future throws an exception
try {
    auto result = continuation.result();
} catch (QException &e) {
    // handle the exception
}
//! [6]

//! [7]
QFuture<int> future = ...;
auto resultFuture = future.then([](int res) {
    ...
    throw Error();
    ...
}).onFailed([](const Error &e) {
    // Handle exceptions of type Error
    ...
    return -1;
}).onFailed([] {
    // Handle all other types of errors
    ...
    return -1;
});

auto result = resultFuture.result(); // result is -1
//! [7]

//! [8]
QFuture<int> future = ...;
future.then([](int res) {
    ...
    throw std::runtime_error("message");
    ...
}).onFailed([](const std::exception &e) {
    // This handler will be invoked
}).onFailed([](const std::runtime_error &e) {
    // This handler won't be invoked, because of the handler above.
});
//! [8]

//! [9]
QFuture<int> future = ...;
auto resultFuture = future.then([](int res) {
    ...
    throw Error("message");
    ...
}).onFailed([](const std::exception &e) {
    // Won't be invoked
}).onFailed([](const QException &e) {
    // Won't be invoked
});

try {
    auto result = resultFuture.result();
} catch(...) {
    // Handle the exception
}
//! [9]

//! [10]
class Object : public QObject
{
    Q_OBJECT
    ...
signals:
    void noArgSignal();
    void singleArgSignal(int value);
    void multipleArgs(int value1, double value2, const QString &value3);
};
//! [10]

//! [11]
Object object;
QFuture<void> voidFuture = QtFuture::connect(&object, &Object::noArgSignal);
QFuture<int> intFuture = QtFuture::connect(&object, &Object::singleArgSignal);

using Args = std::tuple<int, double, QString>;
QFuture<Args> tupleFuture = QtFuture::connect(&object, &Object::multipleArgs)
//! [11]

//! [12]
QtFuture::connect(&object, &Object::singleArgSignal).then([](int value) {
    // do something with the value
});
//! [12]

//! [13]
QtFuture::connect(&object, &Object::singleArgSignal).then(QtFuture::Launch::Async, [](int value) {
    // this will run in a new thread
});
//! [13]

//! [14]
QtFuture::connect(&object, &Object::singleArgSignal).then([](int value) {
    ...
    throw std::exception();
    ...
}).onFailed([](const std::exception &e) {
    // handle the exception
}).onFailed([] {
    // handle other exceptions
});
//! [14]

//! [15]
QFuture<int> testFuture = ...;
auto resultFuture = testFuture.then([](int res) {
    // Block 1
}).onCanceled([] {
    // Block 2
}).onFailed([] {
    // Block 3
}).then([] {
    // Block 4
}).onFailed([] {
    // Block 5
}).onCanceled([] {
    // Block 6
});
//! [15]

//! [16]
QFuture<int> testFuture = ...;
auto resultFuture = testFuture.then([](int res) {
    // Block 1
}).onFailed([] {
    // Block 3
}).then([] {
    // Block 4
}).onFailed([] {
    // Block 5
}).onCanceled([] {
    // Block 6
});
//! [16]

//! [17]
// somewhere in the main thread
auto future = QtConcurrent::run([] {
    // This will run in a separate thread
    ...
}).then(this, [] {
   // Update UI elements
});
//! [17]

//! [18]
auto future = QtConcurrent::run([] {
    ...
}).then(this, [] {
   // Update UI elements
}).then([] {
    // This will also run in the main thread
});
//! [18]

//! [19]
// somewhere in the main thread
auto future = QtConcurrent::run([] {
    // This will run in a separate thread
    ...
    throw std::exception();
}).onFailed(this, [] {
   // Update UI elements
});
//! [19]

//! [20]
QObject *context = ...;
auto future = cachedResultsReady ? QtFuture::makeReadyFuture(results)
                                 : QtConcurrent::run([] { /* compute results */});
auto continuation = future.then(context, [] (Results results) {
    // Runs in the context's thread
}).then([] {
    // May or may not run in the context's thread
});
//! [20]

//! [21]
QFuture<int> testFuture = ...;
auto resultFuture = testFuture.then([](int res) {
    // Block 1
    ...
    return 1;
}).then([](int res) {
    // Block 2
    ...
    return 2;
}).onCanceled([] {
    // Block 3
    ...
    return -1;
});
//! [21]
