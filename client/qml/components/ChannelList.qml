import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    signal voiceChannelPicked(string channelId)

    property string selectedVoiceChannelId: ""
    property int bottomInset: 0
    property string pendingChannelType: "TEXT"

    function voiceChannelNameById(id) {
        for (let i = 0; i < app.store.channels.length; ++i) {
            const channel = app.store.channels[i]
            if (channel.id === id && channel.type === "VOICE") {
                return channel.name
            }
        }
        return ""
    }

    color: "#0d1724"
    border.color: "#24384f"
    border.width: 1

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        anchors.topMargin: 12
        anchors.bottomMargin: 12 + root.bottomInset
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                Layout.fillWidth: true
                text: "Channels"
                color: "#d7e9fd"
                font.pixelSize: 14
                font.bold: true
            }

            ToolButton {
                id: createChannelMenuButton
                text: "Create"
                font.pixelSize: 12
                onClicked: createChannelMenu.open()
                background: Rectangle {
                    radius: 8
                    color: createChannelMenuButton.down ? "#2f7eca"
                                                        : (createChannelMenuButton.hovered ? "#3a95eb" : "#3588d8")
                }
                contentItem: Label {
                    text: createChannelMenuButton.text
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: "#ffffff"
                    font.bold: true
                }
            }

            Menu {
                id: createChannelMenu
                y: createChannelMenuButton.height

                MenuItem {
                    text: "Text Channel"
                    onTriggered: {
                        root.pendingChannelType = "TEXT"
                        createChannelName.text = ""
                        createChannelDialog.open()
                    }
                }

                MenuItem {
                    text: "Voice Channel"
                    onTriggered: {
                        root.pendingChannelType = "VOICE"
                        createChannelName.text = ""
                        createChannelDialog.open()
                    }
                }
            }
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 4
            model: app.store.channels

            delegate: Rectangle {
                id: channelRow
                required property var modelData
                width: ListView.view.width
                readonly property var participantsByChannel: app.store.voiceParticipantsByChannel
                readonly property bool isCurrentText: app.store.currentChannelId === modelData.id
                readonly property bool isCurrentVoice: app.voice.activeChannelId === modelData.id
                readonly property bool isPendingVoice: modelData.type === "VOICE"
                                                      && !app.voice.connected
                                                      && root.selectedVoiceChannelId === modelData.id
                readonly property bool active: isCurrentText || isCurrentVoice || isPendingVoice
                readonly property var channelParticipants: participantsByChannel[modelData.id] ? participantsByChannel[modelData.id] : []
                readonly property bool showVoiceParticipants: modelData.type === "VOICE"
                                                             && channelParticipants.length > 0
                height: 34 + (showVoiceParticipants ? (voiceParticipantsColumn.implicitHeight + 8) : 0)
                radius: 8
                color: active ? "#1f3852" : (mouse.containsMouse ? "#182b3f" : "transparent")

                Column {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    anchors.topMargin: 6
                    spacing: 6

                    RowLayout {
                        width: parent.width
                        spacing: 8

                        Label {
                            text: channelRow.modelData.type === "VOICE" ? "V" : "#"
                            color: channelRow.modelData.type === "VOICE" ? "#89dece" : "#8fb2d2"
                            font.bold: true
                        }

                        Label {
                            Layout.fillWidth: true
                            text: channelRow.modelData.name
                            color: channelRow.active ? "#f1f8ff" : "#b7cfe7"
                            font.bold: channelRow.active
                            elide: Text.ElideRight
                        }
                    }

                    Column {
                        id: voiceParticipantsColumn
                        visible: channelRow.showVoiceParticipants
                        spacing: 4
                        width: parent.width

                        Repeater {
                            model: channelRow.channelParticipants
                            delegate: Item {
                                required property var modelData
                                readonly property bool isSpeaking: !!modelData.speaking
                                readonly property bool isMuted: !!modelData.muted
                                readonly property bool isDeafened: !!modelData.deafened
                                width: channelRow.width - 24
                                height: 18

                                Rectangle {
                                    anchors.fill: parent
                                    radius: 6
                                    color: isSpeaking ? "#1d3b2f" : "transparent"
                                    border.color: isSpeaking ? "#2e7a57" : "transparent"
                                }

                                Row {
                                    anchors.fill: parent
                                    anchors.leftMargin: 4
                                    anchors.rightMargin: 4
                                    spacing: 6

                                    Rectangle {
                                        width: 6
                                        height: 6
                                        radius: 3
                                        y: 5
                                        color: isSpeaking ? "#57f287" : (isMuted ? "#f0bf65" : "#35d78f")
                                    }

                                    Label {
                                        text: modelData.user ? modelData.user.displayName : modelData.userId
                                        color: isSpeaking ? "#e4fff1" : "#a7c4df"
                                        font.bold: isSpeaking
                                        font.pixelSize: 11
                                        elide: Text.ElideRight
                                        width: channelRow.width - 100
                                    }

                                    Label {
                                        visible: isMuted || isDeafened
                                        text: (isMuted ? "M" : "") + (isMuted && isDeafened ? " " : "") + (isDeafened ? "D" : "")
                                        color: "#f4c879"
                                        font.pixelSize: 10
                                        font.bold: true
                                    }
                                }
                            }
                        }
                    }
                }

                MouseArea {
                    id: mouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        if (channelRow.modelData.type === "VOICE") {
                            app.voice.notifyUserGesture()
                            root.selectedVoiceChannelId = channelRow.modelData.id
                            root.voiceChannelPicked(channelRow.modelData.id)
                            app.store.setCurrentDmThreadId("")
                            if (app.voice.connected && app.voice.activeChannelId === channelRow.modelData.id) {
                                return
                            }
                            app.voice.joinVoice(channelRow.modelData.id)
                        } else {
                            root.selectedVoiceChannelId = ""
                            app.store.setCurrentDmThreadId("")
                            app.store.setCurrentChannelId(channelRow.modelData.id)
                            app.chat.loadChannelMessages(true)
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: "#20354b"
        }

        Label {
            text: "Voice Controls"
            color: "#d7e9fd"
            font.pixelSize: 13
            font.bold: true
        }

        Label {
            Layout.fillWidth: true
            text: app.voice.connected
                  ? ("Connected: " + root.voiceChannelNameById(app.voice.activeChannelId))
                  : (root.selectedVoiceChannelId.length > 0
                     ? ("Selected: " + root.voiceChannelNameById(root.selectedVoiceChannelId))
                     : "Select a voice channel")
            color: "#9dbad6"
            elide: Text.ElideRight
            font.pixelSize: 11
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Button {
                id: leaveButton
                Layout.fillWidth: true
                text: "Leave"
                visible: app.voice.connected
                enabled: app.voice.connected
                onClicked: app.voice.leaveVoice()
                background: Rectangle {
                    radius: 8
                    color: leaveButton.down ? "#2f7eca" : (leaveButton.hovered ? "#3a95eb" : "#3588d8")
                }
                contentItem: Label {
                    text: leaveButton.text
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: "#ffffff"
                    font.bold: true
                    font.pixelSize: 12
                }
            }

            Button {
                id: talkButton
                Layout.fillWidth: true
                visible: app.voice.connected && app.voice.activationMode === "PUSH_TO_TALK"
                text: "Talk"
                onPressed: app.voice.setPttPressed(true)
                onReleased: app.voice.setPttPressed(false)
                onCanceled: app.voice.setPttPressed(false)
                background: Rectangle {
                    radius: 8
                    color: talkButton.pressed ? "#3d5aa8"
                                              : (talkButton.hovered ? "#35508f" : "#2d457d")
                }
                contentItem: Label {
                    text: talkButton.text
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: "#ffffff"
                    font.pixelSize: 12
                    font.bold: true
                }
                ToolTip.visible: hovered
                ToolTip.text: "PTT key: " + app.voice.pttHotkey
            }

            Button {
                id: muteButton
                Layout.fillWidth: true
                text: app.voice.muted ? "Unmute" : "Mute"
                enabled: app.voice.connected
                onClicked: app.voice.toggleMute()
            }

            Button {
                id: deafenButton
                Layout.fillWidth: true
                text: app.voice.deafened ? "Undeafen" : "Deafen"
                enabled: app.voice.connected
                onClicked: app.voice.toggleDeafen()
            }
        }

        Label {
            Layout.fillWidth: true
            visible: app.voice.error.length > 0
            text: app.voice.error
            color: "#ff9e9e"
            font.pixelSize: 11
            elide: Text.ElideRight
        }

        Button {
            id: createInviteButton
            Layout.fillWidth: true
            text: app.guild.lastInviteCode.length > 0 ? "Invite: " + app.guild.lastInviteCode : "Create Invite"
            onClicked: app.guild.createInvite()
            background: Rectangle {
                radius: 9
                color: createInviteButton.down ? "#23476d" : (createInviteButton.hovered ? "#2d5a87" : "#264d74")
            }
            contentItem: Label {
                text: createInviteButton.text
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                color: "#dbeeff"
                elide: Text.ElideMiddle
            }
        }
    }

    Dialog {
        id: createChannelDialog
        parent: Overlay.overlay
        width: 340
        modal: true
        title: root.pendingChannelType === "VOICE" ? "Create Voice Channel" : "Create Text Channel"
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        standardButtons: Dialog.NoButton

        ColumnLayout {
            anchors.fill: parent
            spacing: 10

            TextField {
                id: createChannelName
                Layout.fillWidth: true
                placeholderText: "Channel name"
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Button {
                    Layout.fillWidth: true
                    text: "Cancel"
                    onClicked: createChannelDialog.close()
                }

                Button {
                    Layout.fillWidth: true
                    text: "Create"
                    onClicked: {
                        app.guild.createChannel(createChannelName.text, root.pendingChannelType)
                        createChannelName.text = ""
                        createChannelDialog.close()
                    }
                }
            }
        }
    }
}
