/*
    SPDX-FileCopyrightText: 2013-2017 Jan Grulich <jgrulich@redhat.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

import QtQuick 2.2
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.networkmanagement 0.2 as PlasmaNM
import org.kde.kquickcontrolsaddons 2.0
import QtQuick.Layouts 1.1

Item {
    id: mainWindow

    readonly property string kcm: "kcm_networkmanagement"
    readonly property bool kcmAuthorized: KCMShell.authorize("kcm_networkmanagement.desktop").length == 1

    Plasmoid.toolTipMainText: i18n("Networks")
    Plasmoid.toolTipSubText: networkStatus.activeConnections
    Plasmoid.icon: connectionIconProvider.connectionTooltipIcon
    Plasmoid.switchWidth: PlasmaCore.Units.gridUnit * 10
    Plasmoid.switchHeight: PlasmaCore.Units.gridUnit * 10
    Plasmoid.compactRepresentation: CompactRepresentation { }
    Plasmoid.fullRepresentation: PopupDialog {
        id: dialogItem
        Layout.minimumWidth: PlasmaCore.Units.iconSizes.medium * 10
        Layout.minimumHeight: PlasmaCore.Units.gridUnit * 20
        anchors.fill: parent
        focus: true
    }

    function action_wifiSwitch() {
        handler.enableWireless(plasmoid.action("wifiSwitch").checked)
    }

    function action_wwanSwitch() {
        handler.enableWwan(plasmoid.action("wifiSwitch").checked)
    }

    function action_planeModeSwitch() {
        let checked = plasmoid.action("wifiSwitch").checked
        handler.enableAirplaneMode(checked)
        PlasmaNM.Configuration.airplaneModeEnabled = checked
    }

    function action_openKCM() {
        KCMShell.openSystemSettings(kcm)
    }

    function action_showPortal() {
        Qt.openUrlExternally("http://networkcheck.kde.org")
    }

    Component.onCompleted: {
        Plasmoid.setAction("wifiSwitch", i18n("Enable Wi-Fi"), "network-wireless-on")
        plasmoid.action("wifiSwitch").priority = 0
        plasmoid.action("wifiSwitch").checkable = true
        plasmoid.action("wifiSwitch").checked = Qt.binding(() => enabledConnections.wirelessEnabled)
        plasmoid.action("wifiSwitch").visible = Qt.binding(() => enabledConnections.wirelessHwEnabled
                                                                 && availableDevices.wirelessDeviceAvailable
                                                                 && !PlasmaNM.Configuration.airplaneModeEnabled)

        Plasmoid.setAction("wwanSwitch", i18n("Enable Mobile Network"), "network-mobile-on")
        plasmoid.action("wwanSwitch").priority = 0
        plasmoid.action("wwanSwitch").checkable = true
        plasmoid.action("wwanSwitch").checked = Qt.binding(() => enabledConnections.wwanEnabled)
        plasmoid.action("wwanSwitch").visible = Qt.binding(() => enabledConnections.wwanHwEnabled
                                                                 && availableDevices.modemDeviceAvailable
                                                                 && !PlasmaNM.Configuration.airplaneModeEnabled)

        Plasmoid.setAction("planeModeSwitch", i18n("Enable Airplane Mode"), "network-flightmode-on")
        plasmoid.action("planeModeSwitch").priority = 0
        plasmoid.action("planeModeSwitch").checkable = true
        plasmoid.action("planeModeSwitch").checked = Qt.binding(() => PlasmaNM.Configuration.airplaneModeEnabled)

        plasmoid.setAction("showPortal", i18n("Open Network Login Page…"), "internet-services")
        plasmoid.action("showPortal").visible = Qt.binding(() => connectionIconProvider.needsPortal)

        plasmoid.removeAction("configure")
        if (kcmAuthorized)
            plasmoid.setAction("openKCM", i18n("&Configure Network Connections…"), "configure")
    }

    PlasmaNM.EnabledConnections {
        id: enabledConnections
    }

    PlasmaNM.AvailableDevices {
        id: availableDevices
    }

    PlasmaNM.NetworkStatus {
        id: networkStatus
    }

    PlasmaNM.ConnectionIcon {
        id: connectionIconProvider
    }

    PlasmaNM.Handler {
        id: handler
    }

    Timer {
        id: scanTimer
        interval: 10200
        repeat: true
        running: plasmoid.expanded && !PlasmaNM.Configuration.airplaneModeEnabled

        onTriggered: handler.requestScan()
    }
}
