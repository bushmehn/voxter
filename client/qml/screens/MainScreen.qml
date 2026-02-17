import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Voxter 1.0
import "../components"

Item {
    id: root
    property bool showMembers: true
    property string selectedVoiceChannelId: ""
    property int leftProfileDockHeight: 74
    property int leftProfileDockMargin: 10
    property int leftPanelsBottomInset: leftProfileDockHeight + leftProfileDockMargin + 6

    function findDeviceIndex(devices, deviceId) {
        for (let i = 0; i < devices.length; ++i) {
            if (devices[i].id === deviceId) {
                return i
            }
        }
        return 0
    }

    function guildNameById(id) {
        for (let i = 0; i < app.store.guilds.length; ++i) {
            const guild = app.store.guilds[i]
            if (guild.id === id) {
                return guild.name
            }
        }
        return ""
    }

    function channelNameById(id) {
        for (let i = 0; i < app.store.channels.length; ++i) {
            const channel = app.store.channels[i]
            if (channel.id === id) {
                return channel.name
            }
        }
        return ""
    }

    function hotkeyFromKeyEvent(event) {
        if (event.key === Qt.Key_Space) {
            return "SPACE"
        }
        if (event.text && event.text.length === 1) {
            return event.text.toUpperCase()
        }
        return ""
    }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#0a1018" }
            GradientStop { position: 1.0; color: "#070b12" }
        }
    }

    Connections {
        target: app.voice
        function onActiveChannelIdChanged() {
            if (app.voice.activeChannelId.length > 0) {
                root.selectedVoiceChannelId = app.voice.activeChannelId
            }
        }
    }

    Connections {
        target: app.store
        function onCurrentGuildIdChanged() {
            root.selectedVoiceChannelId = ""
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        ServerList {
            id: serverList
            Layout.fillHeight: true
            Layout.preferredWidth: 98
            bottomInset: root.leftPanelsBottomInset
            onOpenSettingsRequested: {
                app.voice.requestAudioDevices(true)
                settingsDialog.syncFromState()
                settingsDialog.open()
            }
        }

        ChannelList {
            id: channelList
            Layout.fillHeight: true
            Layout.preferredWidth: 296
            bottomInset: root.leftPanelsBottomInset
            selectedVoiceChannelId: root.selectedVoiceChannelId
            onVoiceChannelPicked: function(channelId) {
                root.selectedVoiceChannelId = channelId
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 58
                color: "#0d1723"
                border.color: "#22354d"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 18
                    anchors.rightMargin: 14
                    spacing: 10

                    Rectangle {
                        Layout.preferredWidth: 9
                        Layout.preferredHeight: 9
                        radius: 4.5
                        color: "#39d98a"
                        border.color: "#7ff3b4"
                    }

                    Label {
                        Layout.fillWidth: true
                        text: root.guildNameById(app.store.currentGuildId).length > 0
                              ? root.guildNameById(app.store.currentGuildId)
                              : "Select a server"
                        color: "#e8f2ff"
                        font.pixelSize: 16
                        font.bold: true
                        elide: Text.ElideRight
                    }

                    Label {
                        visible: app.store.currentChannelId.length > 0
                        text: root.channelNameById(app.store.currentChannelId).length > 0
                              ? "#" + root.channelNameById(app.store.currentChannelId)
                              : ""
                        color: "#8daecc"
                        font.pixelSize: 12
                    }

                    Button {
                        id: membersToggleButton
                        text: root.showMembers ? "Hide Members" : "Show Members"
                        onClicked: root.showMembers = !root.showMembers
                        background: Rectangle {
                            radius: 8
                            color: membersToggleButton.down ? "#27394e" : (membersToggleButton.hovered ? "#22354a" : "#1a2b3f")
                            border.color: "#304a66"
                        }
                        contentItem: Label {
                            text: membersToggleButton.text
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            color: "#d9ebff"
                            font.pixelSize: 12
                        }
                    }

                    Rectangle {
                        Layout.preferredHeight: 26
                        Layout.preferredWidth: statusLabel.implicitWidth + 18
                        radius: 13
                        color: {
                            const state = app.gateway.connectionState
                            if (state === "connected") {
                                return "#193d2c"
                            }
                            if (state === "connecting") {
                                return "#3f3620"
                            }
                            return "#4b2329"
                        }
                        border.color: {
                            const state = app.gateway.connectionState
                            if (state === "connected") {
                                return "#2f9f65"
                            }
                            if (state === "connecting") {
                                return "#c9984b"
                            }
                            return "#d06a78"
                        }

                        Label {
                            id: statusLabel
                            anchors.centerIn: parent
                            text: {
                                const state = app.gateway.connectionState
                                if (state === "connected") {
                                    return "Connected"
                                }
                                if (state === "connecting") {
                                    return "Connecting..."
                                }
                                if (state === "reconnecting") {
                                    return "Reconnecting (" + app.gateway.reconnectAttempt + ")"
                                }
                                return "Disconnected"
                            }
                            color: "#e9f5ff"
                            font.pixelSize: 11
                            font.bold: true
                        }
                    }

                }
            }

            ChatView {
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }

        MembersPanel {
            Layout.fillHeight: true
            Layout.preferredWidth: root.showMembers ? 280 : 0
            visible: root.showMembers
        }
    }

    LeftProfileDock {
        id: leftProfileDock
        z: 20
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.leftMargin: root.leftProfileDockMargin
        anchors.bottomMargin: root.leftProfileDockMargin
        width: serverList.width + channelList.width - (root.leftProfileDockMargin * 2)
        height: root.leftProfileDockHeight
    }

    Dialog {
        id: settingsDialog
        parent: Overlay.overlay
        modal: true
        width: 520
        height: 560
        title: "Settings"
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        standardButtons: Dialog.NoButton
        property bool capturePttKey: false

        function syncFromState() {
            inputDeviceCombo.currentIndex = app.voice.inputDevices.length > 0
                                           ? root.findDeviceIndex(app.voice.inputDevices, app.voice.selectedInputDeviceId)
                                           : -1
            outputDeviceCombo.currentIndex = app.voice.outputDevices.length > 0
                                            ? root.findDeviceIndex(app.voice.outputDevices, app.voice.selectedOutputDeviceId)
                                            : -1
            activationModeCombo.currentIndex = app.voice.activationMode === "PUSH_TO_TALK" ? 1 : 0
        }

        onOpened: {
            app.voice.requestAudioDevices(true)
            capturePttKey = false
            syncFromState()
            settingsContent.forceActiveFocus()
        }

        onClosed: capturePttKey = false

        ColumnLayout {
            id: settingsContent
            anchors.fill: parent
            spacing: 10
            focus: true

            Keys.onPressed: function(event) {
                if (!settingsDialog.capturePttKey) {
                    return
                }
                const keyName = root.hotkeyFromKeyEvent(event)
                if (keyName.length === 0) {
                    return
                }
                app.voice.setPttHotkey(keyName)
                settingsDialog.capturePttKey = false
                event.accepted = true
            }

            TabBar {
                id: settingsTabs
                Layout.fillWidth: true
                TabButton { text: "General" }
                TabButton { text: "Audio" }
            }

            StackLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 430
                currentIndex: settingsTabs.currentIndex

                Item {
                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 8

                        Label {
                            Layout.fillWidth: true
                            text: "General Settings"
                            color: "#dbeeff"
                            font.bold: true
                        }

                        Label {
                            Layout.fillWidth: true
                            text: "More sections can be added here (notifications, appearance, privacy)."
                            color: "#8fb2d2"
                            wrapMode: Text.Wrap
                            font.pixelSize: 11
                        }
                    }
                }

                Item {
                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 8

                        Label {
                            Layout.fillWidth: true
                            text: "Microphone Input"
                            color: "#dbeeff"
                            font.bold: true
                        }

                        ComboBox {
                            id: inputDeviceCombo
                            Layout.fillWidth: true
                            model: app.voice.inputDevices
                            textRole: "label"
                            valueRole: "id"
                            onActivated: app.voice.setSelectedInputDeviceId(currentValue)
                        }

                        Label {
                            Layout.fillWidth: true
                            text: "Audio Output"
                            color: "#dbeeff"
                            font.bold: true
                        }

                        ComboBox {
                            id: outputDeviceCombo
                            Layout.fillWidth: true
                            model: app.voice.outputDevices
                            textRole: "label"
                            valueRole: "id"
                            onActivated: app.voice.setSelectedOutputDeviceId(currentValue)
                        }

                        Button {
                            text: "Refresh devices"
                            onClicked: app.voice.requestAudioDevices(true)
                        }

                        Label {
                            Layout.fillWidth: true
                            text: "Detected: " + Math.max(0, app.voice.inputDevices.length - 1)
                                  + " input, " + Math.max(0, app.voice.outputDevices.length - 1) + " output"
                            color: "#8fb2d2"
                            font.pixelSize: 11
                        }

                        Label {
                            Layout.fillWidth: true
                            visible: app.voice.inputDevices.length <= 1 || app.voice.outputDevices.length <= 1
                            text: "If only Default is shown: click Refresh devices."
                            color: "#c4d9ef"
                            wrapMode: Text.Wrap
                            font.pixelSize: 11
                        }

                        Label {
                            Layout.fillWidth: true
                            text: "Microphone Activation"
                            color: "#dbeeff"
                            font.bold: true
                        }

                        ComboBox {
                            id: activationModeCombo
                            Layout.fillWidth: true
                            model: [
                                { id: "VOICE_ACTIVITY", label: "Voice Activity" },
                                { id: "PUSH_TO_TALK", label: "Push-To-Talk" }
                            ]
                            textRole: "label"
                            valueRole: "id"
                            onActivated: app.voice.setActivationMode(currentValue)
                        }

                        CheckBox {
                            Layout.fillWidth: true
                            text: "Enable keyboard Push-To-Talk hotkey"
                            checked: app.voice.pttHotkeyEnabled
                            visible: activationModeCombo.currentValue === "PUSH_TO_TALK"
                            onToggled: app.voice.setPttHotkeyEnabled(checked)
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            visible: activationModeCombo.currentValue === "PUSH_TO_TALK"
                            spacing: 8

                            Label {
                                text: "PTT key:"
                                color: "#c8ddf3"
                            }

                            Rectangle {
                                Layout.preferredWidth: 80
                                Layout.preferredHeight: 30
                                radius: 6
                                color: "#1c2a3a"
                                border.color: "#3d5674"

                                Label {
                                    anchors.centerIn: parent
                                    text: app.voice.pttHotkey
                                    color: "#e3f1ff"
                                    font.bold: true
                                }
                            }

                            Button {
                                text: settingsDialog.capturePttKey ? "Press key..." : "Set key"
                                onClicked: settingsDialog.capturePttKey = true
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            text: activationModeCombo.currentValue === "PUSH_TO_TALK"
                                  ? "Use selected key or hold the Talk button in Voice Controls."
                                  : "Microphone transmits automatically when unmuted."
                            color: "#8fb2d2"
                            wrapMode: Text.Wrap
                            font.pixelSize: 11
                        }
                    }
                }

            }

            RowLayout {
                Layout.fillWidth: true

                Item {
                    Layout.fillWidth: true
                }

                Button {
                    text: "Close"
                    onClicked: settingsDialog.close()
                }
            }
        }

        Connections {
            target: app.voice
            function onInputDevicesChanged() { settingsDialog.syncFromState() }
            function onOutputDevicesChanged() { settingsDialog.syncFromState() }
            function onSelectedInputDeviceIdChanged() { settingsDialog.syncFromState() }
            function onSelectedOutputDeviceIdChanged() { settingsDialog.syncFromState() }
            function onActivationModeChanged() { settingsDialog.syncFromState() }
        }
    }
}
