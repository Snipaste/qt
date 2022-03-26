#!/usr/bin/env bash

#############################################################################
##
## Copyright (C) 2020 The Qt Company Ltd.
## Contact: http://www.qt.io/licensing/
##
## This file is part of the provisioning scripts of the Qt Toolkit.
##
## $QT_BEGIN_LICENSE:LGPL21$
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and The Qt Company. For licensing terms
## and conditions see http://www.qt.io/terms-conditions. For further
## information use the contact form at http://www.qt.io/contact-us.
##
## GNU Lesser General Public License Usage
## Alternatively, this file may be used under the terms of the GNU Lesser
## General Public License version 2.1 or version 3 as published by the Free
## Software Foundation and appearing in the file LICENSE.LGPLv21 and
## LICENSE.LGPLv3 included in the packaging of this file. Please review the
## following information to ensure the GNU Lesser General Public License
## requirements will be met: https://www.gnu.org/licenses/lgpl.html and
## http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
##
## As a special exception, The Qt Company gives you certain additional
## rights. These rights are described in The Qt Company LGPL Exception
## version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
##
## $QT_END_LICENSE$
##
#############################################################################

# This script installs PostgreSQL

# PostgreSQL is needed for Qt to be able to support PostgreSQL

set -ex

# shellcheck source=../common/unix/SetEnvVar.sh
source "${BASH_SOURCE%/*}/../unix/SetEnvVar.sh"
# shellcheck source=../common/unix/DownloadURL.sh
source "${BASH_SOURCE%/*}/../unix/DownloadURL.sh"

psqlAppVersion="2.5"
psqlVersion="14"

packageName="Postgres-$psqlAppVersion-$psqlVersion.dmg"

PrimaryUrl="http://ci-files01-hki.intra.qt.io/input/mac/macos_10.12_sierra/$packageName"
AltUrl="https://github.com/PostgresApp/PostgresApp/releases/download/v$psqlAppVersion/$packageName"
SHA1="04cb6939704c5ede5646c1da8a686da3ded98a26"
appPrefix=""

DownloadURL "$PrimaryUrl" "$AltUrl" "$SHA1" "/tmp/$packageName"

mountpoint="/tmp/pg-mount"
mkdir -p "$mountpoint"

echo "Mounting $packageName in $mountpoint"
hdiutil attach -nobrowse -mountpoint "$mountpoint" "/tmp/$packageName"

rm -Rf /Applications/Postgres.app
cp -Rf "$mountpoint/Postgres.app" /Applications

umount "$mountpoint"
echo "Removing $packageName"
rm "/tmp/$packageName"

SetEnvVar "POSTGRESQLBINPATH" "/Applications/Postgres.app/Contents/Versions/$psqlVersion/bin"
echo "PostgreSQL = $psqlVersion ($psqlAppVersion)" >> ~/versions.txt
