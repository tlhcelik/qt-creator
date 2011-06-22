/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/
#include "maddedeviceconfigurationfactory.h"

#include "maemodeviceconfigwizard.h"

#include <utils/qtcassert.h>

namespace RemoteLinux {
namespace Internal {

MaddeDeviceConfigurationFactory::MaddeDeviceConfigurationFactory(QObject *parent)
    : GenericLinuxDeviceConfigurationFactory(parent)
{
}

QString MaddeDeviceConfigurationFactory::displayName() const
{
    return tr("Devices with MADDE support (Fremantle, Harmattan, MeeGo)");
}

ILinuxDeviceConfigurationWizard *MaddeDeviceConfigurationFactory::createWizard(QWidget *parent) const
{
    return new MaemoDeviceConfigWizard(parent);
}

bool MaddeDeviceConfigurationFactory::supportsOsType(const QString &osType) const
{
    return osType == LinuxDeviceConfiguration::Maemo5OsType
            || osType == LinuxDeviceConfiguration::HarmattanOsType
            || osType == LinuxDeviceConfiguration::MeeGoOsType;
}

QString MaddeDeviceConfigurationFactory::displayNameForOsType(const QString &osType) const
{
    QTC_ASSERT(supportsOsType(osType), return QString());
    if (osType == LinuxDeviceConfiguration::Maemo5OsType)
        return tr("Maemo5/Fremantle");
    if (osType == LinuxDeviceConfiguration::HarmattanOsType)
        return tr("Harmattan");
    return tr("MeeGo");
}

} // namespace Internal
} // namespace RemoteLinux