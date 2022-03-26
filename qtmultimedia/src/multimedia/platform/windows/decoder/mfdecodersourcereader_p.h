/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Mobility Components.
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

#ifndef MFDECODERSOURCEREADER_H
#define MFDECODERSOURCEREADER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>

#include <QtCore/qobject.h>
#include "qaudioformat.h"
#include <private/qwindowsiupointer_p.h>

QT_USE_NAMESPACE

class MFDecoderSourceReader : public QObject, public IMFSourceReaderCallback
{
    Q_OBJECT
public:
    MFDecoderSourceReader() {}
    ~MFDecoderSourceReader() override {}

    void clearSource() { m_sourceReader.reset(); }
    QWindowsIUPointer<IMFMediaType> setSource(IMFMediaSource *source, QAudioFormat::SampleFormat);

    void readNextSample();

    //from IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObject) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    //from IMFSourceReaderCallback
    STDMETHODIMP OnReadSample(HRESULT hrStatus, DWORD dwStreamIndex,
        DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample *pSample) override;
    STDMETHODIMP OnFlush(DWORD) override { return S_OK; }
    STDMETHODIMP OnEvent(DWORD, IMFMediaEvent *) override { return S_OK; }

Q_SIGNALS:
    void newSample(QWindowsIUPointer<IMFSample>);
    void finished();

private:
    long m_cRef = 1;
    QWindowsIUPointer<IMFSourceReader> m_sourceReader;

};
#endif//MFDECODERSOURCEREADER_H
