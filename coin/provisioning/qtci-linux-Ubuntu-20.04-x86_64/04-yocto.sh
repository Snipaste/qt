#!/usr/bin/env bash

#############################################################################
##
## Copyright (C) 2021 The Qt Company Ltd.
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

# This script installs the Yocto toolchain

set -ex

# shellcheck source=../common/unix/DownloadURL.sh
source "${BASH_SOURCE%/*}/../common/unix/DownloadURL.sh"
# shellcheck source=../common/unix/SetEnvVar.sh
source "${BASH_SOURCE%/*}/../common/unix/SetEnvVar.sh"

primaryBaseUrlPath="http://ci-files01-hki.intra.qt.io/input/boot2qt/gatesgarth"
altBaseUrlPath="http://download.qt.io/development_releases/prebuilt/boot2qt/gatesgarth"

echo "Installing Yocto toolchain for 32-bit b2qt ARMV7..."

versionARM="3.2"
package="b2qt-x86_64-meta-toolchain-b2qt-ci-sdk-qemuarm-a9d5156a.sh"
PrimaryUrl="$primaryBaseUrlPath/$package"
AltUrl="$altBaseUrlPath/$package"
SHA1="f9f7d51656067a1cc9d7ab92ddcddb219886ab22"
yoctoInstaller="/tmp/yocto-toolchain-ARMv7.sh"
yoctoLocationARMv7="/opt/b2qt/$versionARM"
sysrootARMv7="armv7vet2hf-neon-poky-linux-gnueabi"
crosscompileARMv7="sysroots/x86_64-pokysdk-linux/usr/bin/arm-poky-linux-gnueabi/arm-poky-linux-gnueabi-"
envSetupARMv7="environment-setup-$sysrootARMv7"
toolchainFileARMv7="sysroots/x86_64-pokysdk-linux/usr/share/cmake/OEToolchainConfig.cmake"

DownloadURL "$PrimaryUrl" "$AltUrl" "$SHA1" "$yoctoInstaller"
chmod +x "$yoctoInstaller"

/bin/bash "$yoctoInstaller" -y -d "$yoctoLocationARMv7"
rm -rf "$yoctoInstaller"

echo "Installing Yocto toolchain for 64-bit b2qt ARM64..."

versionARM64="3.2"
package="b2qt-x86_64-meta-toolchain-b2qt-ci-sdk-qemuarm64-a9d5156a.sh"
PrimaryUrl="$primaryBaseUrlPath/$package"
AltUrl="$altBaseUrlPath/$package"
SHA1="f490cbcc4e0d5a87f4e07607a71013aeeabce94a"
yoctoInstaller="/tmp/yocto-toolchain-ARM64.sh"
yoctoLocationARM64="/opt/b2qt/$versionARM64"
sysrootARM64="cortexa57-poky-linux"
crosscompileARM64="sysroots/x86_64-pokysdk-linux/usr/bin/aarch64-poky-linux/aarch64-poky-linux-"
envSetupARM64="environment-setup-$sysrootARM64"
toolchainFileARM64="sysroots/x86_64-pokysdk-linux/usr/share/cmake/OEToolchainConfig.cmake"

DownloadURL "$PrimaryUrl" "$AltUrl" "$SHA1" "$yoctoInstaller"
chmod +x "$yoctoInstaller"

/bin/bash "$yoctoInstaller" -y -d "$yoctoLocationARM64"
rm -rf "$yoctoInstaller"

echo "Installing Yocto toolchain for 64-bit b2qt MIPS64..."

versionMIPS64="3.2"
package="b2qt-x86_64-meta-toolchain-b2qt-ci-sdk-qemumips64-a9d5156a.sh"
PrimaryUrl="$primaryBaseUrlPath/$package"
AltUrl="$altBaseUrlPath/$package"
SHA1="5d3a8bb4384de273937286d275d1dab36f969951"
yoctoInstaller="/tmp/yocto-toolchain-mips64.sh"
yoctoLocationMIPS64="/opt/b2qt/$versionMIPS64"
sysrootMIPS64="mips64r2-poky-linux"
crosscompileMIPS64="sysroots/x86_64-pokysdk-linux/usr/bin/mips64-poky-linux/mips64-poky-linux-"
envSetupMIPS64="environment-setup-$sysrootMIPS64"
toolchainFileMIPS64="sysroots/x86_64-pokysdk-linux/usr/share/cmake/OEToolchainConfig.cmake"

DownloadURL "$PrimaryUrl" "$AltUrl" "$SHA1" "$yoctoInstaller"
chmod +x "$yoctoInstaller"

/bin/bash "$yoctoInstaller" -y -d "$yoctoLocationMIPS64"
rm -rf "$yoctoInstaller"



if [ -e "$yoctoLocationARMv7/sysroots/$sysrootARMv7" ] && [ -e "$yoctoLocationARMv7/${crosscompileARMv7}g++" ] && \
   [ -e "$yoctoLocationARMv7/$envSetupARMv7" ] && [ -e "$yoctoLocationARMv7/$toolchainFileARMv7" ] && \
   [ -e "$yoctoLocationARM64/sysroots/$sysrootARM64" ] && [ -e "$yoctoLocationARM64/${crosscompileARM64}g++" ] && \
   [ -e "$yoctoLocationARM64/$envSetupARM64" ] && [ -e "$yoctoLocationARM64/$toolchainFileARM64" ] && \
   [ -e "$yoctoLocationMIPS64/sysroots/$sysrootMIPS64" ] && [ -e "$yoctoLocationMIPS64/${crosscompileMIPS64}g++" ] && \
   [ -e "$yoctoLocationMIPS64/$envSetupMIPS64" ] && [ -e "$yoctoLocationMIPS64/$toolchainFileMIPS64" ]; then
    SetEnvVar "QEMUARMV7_TOOLCHAIN_SYSROOT" "$yoctoLocationARMv7/sysroots/$sysrootARMv7"
    SetEnvVar "QEMUARMV7_TOOLCHAIN_CROSS_COMPILE" "$yoctoLocationARMv7/$crosscompileARMv7"
    SetEnvVar "QEMUARMV7_TOOLCHAIN_ENVSETUP" "$yoctoLocationARMv7/$envSetupARMv7"
    SetEnvVar "QEMUARMV7_TOOLCHAIN_FILE" "$yoctoLocationARMv7/$toolchainFileARMv7"
    SetEnvVar "QEMUARM64_TOOLCHAIN_SYSROOT" "$yoctoLocationARM64/sysroots/$sysrootARM64"
    SetEnvVar "QEMUARM64_TOOLCHAIN_CROSS_COMPILE" "$yoctoLocationARM64/$crosscompileARM64"
    SetEnvVar "QEMUARM64_TOOLCHAIN_ENVSETUP" "$yoctoLocationARM64/$envSetupARM64"
    SetEnvVar "QEMUARM64_TOOLCHAIN_FILE" "$yoctoLocationARM64/$toolchainFileARM64"
    SetEnvVar "QEMUMIPS64_TOOLCHAIN_SYSROOT" "$yoctoLocationMIPS64/sysroots/$sysrootMIPS64"
    SetEnvVar "QEMUMIPS64_TOOLCHAIN_CROSS_COMPILE" "$yoctoLocationMIPS64/$crosscompileMIPS64"
    SetEnvVar "QEMUMIPS64_TOOLCHAIN_ENVSETUP" "$yoctoLocationMIPS64/$envSetupMIPS64"
    SetEnvVar "QEMUMIPS64_TOOLCHAIN_FILE" "$yoctoLocationMIPS64/$toolchainFileMIPS64"
else
    echo "Error! Couldn't find installation paths for Yocto toolchain. Aborting provisioning." 1>&2
    exit 1
fi

echo "Yocto ARMv7 toolchain = $versionARM" >> ~/versions.txt
echo "Yocto ARM64 toolchain = $versionARM64" >> ~/versions.txt
echo "Yocto MIPS64 toolchain = $versionMIPS64" >> ~/versions.txt

# List qt user in qemu toolchain sysroots
sudo sh -c "grep ^qt /etc/passwd >> $yoctoLocationARMv7/sysroots/$sysrootARMv7/etc/passwd"
sudo sh -c "grep ^qt /etc/group >> $yoctoLocationARMv7/sysroots/$sysrootARMv7/etc/group"
sudo sh -c "grep ^qt /etc/passwd >> $yoctoLocationARM64/sysroots/$sysrootARM64/etc/passwd"
sudo sh -c "grep ^qt /etc/group >> $yoctoLocationARM64/sysroots/$sysrootARM64/etc/group"

# Install qemu binfmt for 32bit and 64bit arm architectures
sudo update-binfmts --package qemu-arm --install arm $yoctoLocationARMv7/sysroots/x86_64-pokysdk-linux/usr/bin/qemu-arm \
--magic "\x7f\x45\x4c\x46\x01\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x02\x00\x28\x00" \
--mask "\xff\xff\xff\xff\xff\xff\xff\x00\xff\xff\xff\xff\xff\xff\xff\xff\xfe\xff\xff\xff"
sudo update-binfmts --package qemu-aarch64 --install aarch64 $yoctoLocationARM64/sysroots/x86_64-pokysdk-linux/usr/bin/qemu-aarch64 \
--magic "\x7f\x45\x4c\x46\x02\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x02\x00\xb7\x00" \
--mask "\xff\xff\xff\xff\xff\xff\xff\x00\xff\xff\xff\xff\xff\xff\xff\xff\xfe\xff\xff\xff"
