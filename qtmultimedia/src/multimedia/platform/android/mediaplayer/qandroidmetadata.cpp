/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
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

#include "qandroidmetadata_p.h"

#include "androidmediametadataretriever_p.h"
#include <QtMultimedia/qmediametadata.h>
#include <qsize.h>
#include <QDate>
#include <QtCore/qlist.h>
#include <QtConcurrent/qtconcurrentrun.h>
#include <QLoggingCategory>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcaMetadata, "qt.multimedia.android.metadata")

// Genre name ordered by ID
// see: http://id3.org/id3v2.3.0#Appendix_A_-_Genre_List_from_ID3v1
static const char* qt_ID3GenreNames[] =
{
    "Blues", "Classic Rock", "Country", "Dance", "Disco", "Funk", "Grunge", "Hip-Hop", "Jazz",
    "Metal", "New Age", "Oldies", "Other", "Pop", "R&B", "Rap", "Reggae", "Rock", "Techno",
    "Industrial", "Alternative", "Ska", "Death Metal", "Pranks", "Soundtrack", "Euro-Techno",
    "Ambient", "Trip-Hop", "Vocal", "Jazz+Funk", "Fusion", "Trance", "Classical", "Instrumental",
    "Acid", "House", "Game", "Sound Clip", "Gospel", "Noise", "AlternRock", "Bass", "Soul", "Punk",
    "Space", "Meditative", "Instrumental Pop", "Instrumental Rock", "Ethnic", "Gothic", "Darkwave",
    "Techno-Industrial", "Electronic", "Pop-Folk", "Eurodance", "Dream", "Southern Rock", "Comedy",
    "Cult", "Gangsta", "Top 40", "Christian Rap", "Pop/Funk", "Jungle", "Native American",
    "Cabaret", "New Wave", "Psychadelic", "Rave", "Showtunes", "Trailer", "Lo-Fi", "Tribal",
    "Acid Punk", "Acid Jazz", "Polka", "Retro", "Musical", "Rock & Roll", "Hard Rock", "Folk",
    "Folk-Rock", "National Folk", "Swing", "Fast Fusion", "Bebob", "Latin", "Revival", "Celtic",
    "Bluegrass", "Avantgarde", "Gothic Rock", "Progressive Rock", "Psychedelic Rock",
    "Symphonic Rock", "Slow Rock", "Big Band", "Chorus", "Easy Listening", "Acoustic", "Humour",
    "Speech", "Chanson", "Opera", "Chamber Music", "Sonata", "Symphony", "Booty Bass", "Primus",
    "Porn Groove", "Satire", "Slow Jam", "Club", "Tango", "Samba", "Folklore", "Ballad",
    "Power Ballad", "Rhythmic Soul", "Freestyle", "Duet", "Punk Rock", "Drum Solo", "A capella",
    "Euro-House", "Dance Hall"
};

QMediaMetaData QAndroidMetaData::extractMetadata(const QUrl &url)
{
    QMediaMetaData metadata;

    if (!url.isEmpty()) {
        AndroidMediaMetadataRetriever retriever;
        if (!retriever.setDataSource(url))
            return metadata;

        QString mimeType = retriever.extractMetadata(AndroidMediaMetadataRetriever::MimeType);
        if (!mimeType.isNull())
            metadata.insert(QMediaMetaData::MediaType, mimeType);

        bool isVideo = !retriever.extractMetadata(AndroidMediaMetadataRetriever::HasVideo).isNull()
                || mimeType.startsWith(QStringLiteral("video"));

        QString string = retriever.extractMetadata(AndroidMediaMetadataRetriever::Album);
        if (!string.isNull())
            metadata.insert(QMediaMetaData::AlbumTitle, string);

        string = retriever.extractMetadata(AndroidMediaMetadataRetriever::AlbumArtist);
        if (!string.isNull())
            metadata.insert(QMediaMetaData::AlbumArtist, string);

        string = retriever.extractMetadata(AndroidMediaMetadataRetriever::Artist);
        if (!string.isNull()) {
            metadata.insert(isVideo ? QMediaMetaData::LeadPerformer
                                    : QMediaMetaData::ContributingArtist,
                            string.split(QLatin1Char('/'), Qt::SkipEmptyParts));
        }

        string = retriever.extractMetadata(AndroidMediaMetadataRetriever::Author);
        if (!string.isNull())
            metadata.insert(QMediaMetaData::Author, string.split(QLatin1Char('/'), Qt::SkipEmptyParts));

        string = retriever.extractMetadata(AndroidMediaMetadataRetriever::Bitrate);
        if (!string.isNull()) {
            metadata.insert(isVideo ? QMediaMetaData::VideoBitRate
                                    : QMediaMetaData::AudioBitRate,
                            string.toInt());
        }

        string = retriever.extractMetadata(AndroidMediaMetadataRetriever::CDTrackNumber);
        if (!string.isNull())
            metadata.insert(QMediaMetaData::TrackNumber, string.toInt());

        string = retriever.extractMetadata(AndroidMediaMetadataRetriever::Composer);
        if (!string.isNull())
            metadata.insert(QMediaMetaData::Composer, string.split(QLatin1Char('/'), Qt::SkipEmptyParts));

        string = retriever.extractMetadata(AndroidMediaMetadataRetriever::Date);
        if (!string.isNull())
            metadata.insert(QMediaMetaData::Date, QDateTime::fromString(string, QStringLiteral("yyyyMMddTHHmmss.zzzZ")).date());

        string = retriever.extractMetadata(AndroidMediaMetadataRetriever::Duration);
        if (!string.isNull())
            metadata.insert(QMediaMetaData::Duration, string.toLongLong());

        string = retriever.extractMetadata(AndroidMediaMetadataRetriever::Genre);
        if (!string.isNull()) {
            // The genre can be returned as an ID3v2 id, get the name for it in that case
            if (string.startsWith(QLatin1Char('(')) && string.endsWith(QLatin1Char(')'))) {
                bool ok = false;
                const int genreId = QStringView{string}.mid(1, string.length() - 2).toInt(&ok);
                if (ok && genreId >= 0 && genreId <= 125)
                    string = QLatin1String(qt_ID3GenreNames[genreId]);
            }
            metadata.insert(QMediaMetaData::Genre, string);
        }

        string = retriever.extractMetadata(AndroidMediaMetadataRetriever::Title);
        if (!string.isNull())
            metadata.insert(QMediaMetaData::Title, string);

        string = retriever.extractMetadata(AndroidMediaMetadataRetriever::VideoHeight);
        if (!string.isNull()) {
            const int height = string.toInt();
            const int width = retriever.extractMetadata(AndroidMediaMetadataRetriever::VideoWidth).toInt();
            metadata.insert(QMediaMetaData::Resolution, QSize(width, height));
        }

//        string = retriever.extractMetadata(AndroidMediaMetadataRetriever::Writer);
//        if (!string.isNull())
//            metadata.insert(QMediaMetaData::Writer, string.split('/', Qt::SkipEmptyParts));

    }

    return metadata;
}

QLocale::Language getLocaleLanguage(const QString &language)
{
    // undefined language or uncoded language
    if (language == QLatin1String("und") || language == QStringLiteral("mis"))
        return QLocale::AnyLanguage;

    QLocale locale(language);

    if (locale == QLocale::c()) {
        qCWarning(lcaMetadata) << "Could not parse language:" << language
                               << ". It is not a valid Unicode CLDR language code.";
        return QLocale::AnyLanguage;
    }

    return locale.language();
}

QAndroidMetaData::QAndroidMetaData(int trackType, int androidTrackType, int androidTrackNumber,
                                   const QString &mimeType, const QString &language)
    : mTrackType(trackType),
      mAndroidTrackType(androidTrackType),
      mAndroidTrackNumber(androidTrackNumber)
{
    insert(QMediaMetaData::MediaType, mimeType);
    insert(QMediaMetaData::Language, getLocaleLanguage(language));
}

int QAndroidMetaData::trackType() const
{
    return mTrackType;
}

int QAndroidMetaData::androidTrackType() const
{
    return mAndroidTrackType;
}

int QAndroidMetaData::androidTrackNumber() const
{
    return mAndroidTrackNumber;
}

QT_END_NAMESPACE
