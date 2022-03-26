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

# PySide versions following 5.6 use a C++ parser based on Clang (http://clang.org/).
# The Clang library (C-bindings), version 3.9 or higher is required for building.

# This same script is used to provision libclang to Linux and macOS.
# In case of Linux, we expect to get the values as args
set -e

# shellcheck source=./check_and_set_proxy.sh
source "${BASH_SOURCE%/*}/check_and_set_proxy.sh"
# shellcheck source=./SetEnvVar.sh
source "${BASH_SOURCE%/*}/SetEnvVar.sh"
# shellcheck source=./DownloadURL.sh
source "${BASH_SOURCE%/*}/DownloadURL.sh"

libclang_version=12.0

if uname -a |grep -q Darwin; then
    version=$libclang_version
    url="https://download.qt.io/development_releases/prebuilt/libclang/qt/libclang-release_${version//\./}-based-mac.7z"
    url_cached="http://ci-files01-hki.intra.qt.io/input/libclang/qt/libclang-release_${version//\./}-based-mac.7z"
    sha1="bb9223450c1c36ee37d8c91e876dba82db117a7a"
elif test -f /etc/redhat-release && cat /etc/redhat-release | grep "Red Hat" | grep -v "8" ; then
    version=$libclang_version
    url="https://download.qt.io/development_releases/prebuilt/libclang/qt/libclang-release_${version//\./}-based-linux-CentOS8.3-gcc9.2-x86_64.7z"
    url_cached="http://ci-files01-hki.intra.qt.io/input/libclang/qt/libclang-release_${version//\./}-based-linux-CentOS8.3-gcc9.2-x86_64.7z"
    sha1="d02e4dd30fe3f810c3a0cdcbed5870dec56ed91b"
else
    version=$libclang_version
    url="https://download.qt.io/development_releases/prebuilt/libclang/qt/libclang-release_${version//\./}-based-linux-Ubuntu20.04-gcc9.3-x86_64.7z"
    url_cached="http://ci-files01-hki.intra.qt.io/input/libclang/qt/libclang-release_${version//\./}-based-linux-Ubuntu20.04-gcc9.3-x86_64.7z"
    sha1="81b3e640befa23b5cdebd59e206dc79e4de20ba3"
fi

zip="/tmp/libclang.7z"
destination="/usr/local/libclang-$version"

DownloadURL $url_cached $url $sha1 $zip
if command -v 7zr &> /dev/null; then
    sudo 7zr x $zip -o/usr/local/
else
    sudo 7z x $zip -o/usr/local/
fi
sudo mv /usr/local/libclang "$destination"
rm -rf $zip


echo "export LLVM_INSTALL_DIR=$destination" >> ~/.bash_profile
echo "libClang = $version" >> ~/versions.txt

# This is a hacked static build of libclang which requires special
# handling on the qdoc side.
SetEnvVar "QDOC_USE_STATIC_LIBCLANG" "1"
