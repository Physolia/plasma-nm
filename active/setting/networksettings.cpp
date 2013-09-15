/*
    Copyright 2013 Jan Grulich <jgrulich@redhat.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) version 3, or any
    later version accepted by the membership of KDE e.V. (or its
    successor approved by the membership of KDE e.V.), which shall
    act as a proxy defined in Section 6 of version 3 of the license.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "networksettings.h"
#include "uiutils.h"

#include <NetworkManagerQt/Manager>
#include <NetworkManagerQt/Settings>
#include <NetworkManagerQt/Connection>

#include <KDebug>
#include <KLocale>
#include <KConfig>
#include <KConfigGroup>

class NetworkSettingsPrivate {
public:
    NetworkSettings *q;
    QString icon;
    QString settingName;
    QString status;

    NetworkManager::ConnectionSettings::ConnectionType connectionType;
//     QStringList detailKeys;
    QString path;

    void initNetwork();
};

NetworkSettings::NetworkSettings()
{
    d = new NetworkSettingsPrivate;
    d->q = this;
    d->connectionType = NetworkManager::ConnectionSettings::Unknown;
    d->settingName = i18n("Network Settings");
    d->status = i18n("Network status and control");

//     QStringList detailKeys;
//     detailKeys << "interface:status" << "interface:bitrate" << "interface:hardwareaddress" << "ipv4:address" << "ipv6:address" << "wireless:ssid" << "wireless:signal";
//     detailKeys << "wireless:security" << "mobile:operator" << "mobile:quality" << "mobile:technology" << "vpn:plugin" << "vpn:banner";

    KConfig config("networkactivesettingrc");
    KConfigGroup generalGroup(&config, "General");

//     setDetailKeys(generalGroup.readEntry("DetailKeys", detailKeys));

    kDebug() << "NetworkSettings module loaded.";
}

NetworkSettings::~NetworkSettings()
{
    kDebug() << "========================== NetworkSettings destroyed";
    delete d;
}

int NetworkSettings::connectionType() const
{
    return (int) d->connectionType;
}

void NetworkSettings::setConnectionType(int type)
{
    NetworkManager::ConnectionSettings::ConnectionType newType = (NetworkManager::ConnectionSettings::ConnectionType) type;

    if (d->connectionType != newType) {
        d->connectionType = newType;
        emit connectionTypeChanged();
    }
}

// QStringList NetworkSettings::detailKeys() const
// {
//     return d->detailKeys;
// }
//
// void NetworkSettings::setDetailKeys(const QStringList &keys)
// {
//     if (d->detailKeys != keys) {
//         d->detailKeys = keys;
//         updateDetails();
//         emit detailKeysChanged();
//     }
// }

QString NetworkSettings::settingName() const
{
    return d->settingName;
}

void NetworkSettings::setSettingName(const QString &name)
{
    if (d->settingName != name) {
        d->settingName = name;
        emit settingNameChanged();
    }
}

QString NetworkSettings::status() const
{
    return d->status;
}

void NetworkSettings::setStatus(const QString &status)
{
    if (d->status != status) {
        d->status = status;
        emit statusChanged();
    }
}

void NetworkSettings::setNetworkSetting(uint type, const QString &path)
{
    if (d->connectionType == NetworkManager::ConnectionSettings::Vpn) {
        disconnect(NetworkManager::notifier(), 0, this, 0);
    } else if (d->connectionType != NetworkManager::ConnectionSettings::Unknown) {
        NetworkManager::Device::Ptr device = NetworkManager::findNetworkInterface(path);
        if (device) {
            disconnect(device.data(), 0, this, 0);
        }
    }

    setConnectionType(type);
    d->path = path;

    if (d->connectionType == NetworkManager::ConnectionSettings::Wired ||
        d->connectionType == NetworkManager::ConnectionSettings::Gsm ||
        d->connectionType == NetworkManager::ConnectionSettings::Wireless) {
        NetworkManager::Device::Ptr device = NetworkManager::findNetworkInterface(path);

        if (device) {
            connect(device.data(), SIGNAL(connectionStateChanged()),
                    SLOT(updateStatus()), Qt::UniqueConnection);
            connect(device.data(), SIGNAL(activeConnectionChanged()),
                    SLOT(updateStatus()), Qt::UniqueConnection);
        }
    } else if (d->connectionType == NetworkManager::ConnectionSettings::Vpn) {
        foreach (const NetworkManager::ActiveConnection::Ptr & activeConnection, NetworkManager::activeConnections()) {
            if (activeConnection && activeConnection->vpn()) {
                connect(activeConnection.data(), SIGNAL(stateChanged(NetworkManager::ActiveConnection::State)),
                        SLOT(updateStatus()), Qt::UniqueConnection);
            }
        }

        connect(NetworkManager::notifier(), SIGNAL(activeConnectionAdded(QString)),
                SLOT(activeConnectionAdded(QString)), Qt::UniqueConnection);
    }

//     updateConnectionType();
    updateSettingName();
    updateStatus();
}

void NetworkSettings::activeConnectionAdded(const QString &active)
{
    NetworkManager::ActiveConnection::Ptr activeConnection = NetworkManager::findActiveConnection(active);

    if (activeConnection && activeConnection->vpn()) {
        connect(activeConnection.data(), SIGNAL(stateChanged(NetworkManager::ActiveConnection::State)),
                SLOT(updateStatus()), Qt::UniqueConnection);
        updateStatus();
    }
}

void NetworkSettings::updateSettingName()
{
    switch (d->connectionType) {
        case NetworkManager::ConnectionSettings::Wired:
        setSettingName(i18n("Ethernet Settings"));
        break;
    case NetworkManager::ConnectionSettings::Gsm:
        setSettingName(i18n("Modem Settings"));
        break;
    case NetworkManager::ConnectionSettings::Vpn:
        setSettingName(i18n("VPN Settings"));
        break;
    case NetworkManager::ConnectionSettings::Wireless:
        setSettingName(i18n("Wireless Settings"));
        break;
    default:
        break;
    }
}

void NetworkSettings::updateStatus()
{
    if (d->connectionType == NetworkManager::ConnectionSettings::Vpn) {
        // TODO: maybe check for the case when there are two active VPN connections
        bool vpnFound = false;
        foreach (const NetworkManager::ActiveConnection::Ptr & activeConnection, NetworkManager::activeConnections()) {
            if (activeConnection && activeConnection->vpn()) {
                vpnFound = true;
                if (activeConnection->state() == NetworkManager::ActiveConnection::Unknown) {
                    setStatus(UiUtils::connectionStateToString(NetworkManager::Device::UnknownState));
                } else if (activeConnection->state() == NetworkManager::ActiveConnection::Activating) {
                    setStatus(i18n("Connecting"));
                } else if (activeConnection->state() == NetworkManager::ActiveConnection::Activated) {
                    setStatus(UiUtils::connectionStateToString(NetworkManager::Device::Activated) % i18n(" to %1").arg(activeConnection->connection()->name()));
                } else if (activeConnection->state() == NetworkManager::ActiveConnection::Deactivating) {
                    setStatus(UiUtils::connectionStateToString(NetworkManager::Device::Deactivating));
                } else if (activeConnection->state() == NetworkManager::ActiveConnection::Deactivated) {
                    setStatus(UiUtils::connectionStateToString(NetworkManager::Device::Disconnected));
                }
            }
        }

        if (!vpnFound) {
            setStatus(UiUtils::connectionStateToString(NetworkManager::Device::Disconnected));
        }
    } else if (d->connectionType != NetworkManager::ConnectionSettings::Unknown) {
        NetworkManager::Device::Ptr device = NetworkManager::findNetworkInterface(d->path);
        if (device) {
            NetworkManager::ActiveConnection::Ptr activeConnection = device->activeConnection();
            if (activeConnection) {
                setStatus(UiUtils::connectionStateToString(device->state(), activeConnection->connection()->name()));
            } else {
                setStatus(UiUtils::connectionStateToString(device->state()));
            }
        }
    } else {
        setStatus(UiUtils::connectionStateToString(NetworkManager::Device::UnknownState));
    }
}

#include "networksettings.moc"
