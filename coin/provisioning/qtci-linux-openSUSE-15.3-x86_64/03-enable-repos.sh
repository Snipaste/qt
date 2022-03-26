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

set -ex

sudo sed -i "s#baseurl=.*#baseurl=http://repo-clones.ci.qt.io/repos/opensuse/distribution/leap/15.3/repo/oss/#g" /etc/zypp/repos.d/repo-oss.repo
sudo sed -i "s#baseurl=.*#baseurl=http://repo-clones.ci.qt.io/repos/opensuse/distribution/leap/15.3/repo/non-oss/#g" /etc/zypp/repos.d/repo-non-oss.repo
sudo sed -i "s#baseurl=.*#baseurl=http://repo-clones.ci.qt.io/repos/opensuse/update/leap/15.3/oss/#g" /etc/zypp/repos.d/repo-update.repo
sudo sed -i "s#baseurl=.*#baseurl=http://repo-clones.ci.qt.io/repos/opensuse/update/leap/15.3/non-oss/#g" /etc/zypp/repos.d/repo-update-non-oss.repo

sudo zypper lr -u
