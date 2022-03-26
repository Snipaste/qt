#!/usr/bin/env bash

set -ex

sudo zypper -nq install git gcc9 gcc9-c++ ninja
sudo /usr/sbin/update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 1 \
                                     --slave /usr/bin/g++ g++ /usr/bin/g++-9 \
                                     --slave /usr/bin/cc cc /usr/bin/gcc-9 \
                                     --slave /usr/bin/c++ c++ /usr/bin/g++-9

sudo zypper -nq install bison flex gperf \
        zlib-devel \
        libudev-devel \
        glib2-devel \
        libopenssl-devel \
        freetype2-devel \
        fontconfig-devel \
        sqlite3-devel \
        libxkbcommon-devel \
        libxkbcommon-x11-devel \
        pcre2-devel libpng16-devel

# EGL support
sudo zypper -nq install Mesa-libEGL-devel Mesa-libGL-devel


# Xinput2
sudo zypper -nq install libXi-devel

# system provided XCB libraries
sudo zypper -nq install xcb-util-devel xcb-util-image-devel xcb-util-keysyms-devel \
         xcb-util-wm-devel xcb-util-renderutil-devel

# ICU
sudo zypper -nq install libicu-devel

# qtwebengine
sudo zypper -nq install alsa-devel dbus-1-devel libxkbfile-devel \
         libXcomposite-devel libXcursor-devel libXrandr-devel libXtst-devel \
         mozilla-nspr-devel mozilla-nss-devel nodejs12 glproto-devel \
         libxshmfence-devel libXdamage-devel

# qtwebkit
sudo zypper -nq install libxml2-devel libxslt-devel

# GStreamer (qtwebkit and qtmultimedia), pulseaudio (qtmultimedia)
sudo zypper -nq install gstreamer-devel gstreamer-plugins-base-devel libpulse-devel

# cups
sudo zypper -nq install cups-devel

#speech-dispatcher
sudo zypper -nq install libspeechd-devel

# make
sudo zypper -nq install make

gccVersion="$(gcc --version |grep gcc |cut -b 17-23)"
echo "GCC = $gccVersion" >> versions.txt

OpenSSLVersion="$(openssl version |cut -b 9-14)"
echo "OpenSSL = $OpenSSLVersion" >> ~/versions.txt
