import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    property string selectedVoiceChannelId: ""

    function channelNameById(id) {
        for (let i = 0; i < app.store.channels.length; ++i) {
            const channel = app.store.channels[i]
            if (channel.id === id) {
                return channel.name
            }
        }
        return id
    }

    color: "#0c1623"
    border.color: "#24384f"
    border.width: 1

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Rectangle {
                Layout.preferredWidth: 9
                Layout.preferredHeight: 9
                radius: 4.5
                color: app.voice.connected ? "#35d78f" : "#70879f"
                border.color: app.voice.connected ? "#7df0bc" : "#8da4bc"
            }

            Label {
                Layout.fillWidth: true
                text: app.voice.connected
                      ? "Connected / " + root.channelNameById(app.voice.activeChannelId)
                      : (root.selectedVoiceChannelId.length > 0
                         ? "Click voice channel to join / " + root.channelNameById(root.selectedVoiceChannelId)
                         : "Click a voice channel to join")
                color: "#d4e9ff"
                elide: Text.ElideRight
                font.bold: true
            }

            Button {
                id: leaveButton
                text: "Leave"
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
                id: muteButton
                text: app.voice.muted ? "Unmute" : "Mute"
                enabled: app.voice.connected
                onClicked: app.voice.toggleMute()
                background: Rectangle {
                    radius: 8
                    color: muteButton.down ? "#3d404d" : (muteButton.hovered ? "#353844" : "#2f323d")
                }
                contentItem: Label {
                    text: muteButton.text
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: "#dce7f5"
                    font.pixelSize: 12
                }
            }

            Button {
                id: deafenButton
                text: app.voice.deafened ? "Undeafen" : "Deafen"
                enabled: app.voice.connected
                onClicked: app.voice.toggleDeafen()
                background: Rectangle {
                    radius: 8
                    color: deafenButton.down ? "#3d404d" : (deafenButton.hovered ? "#353844" : "#2f323d")
                }
                contentItem: Label {
                    text: deafenButton.text
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: "#dce7f5"
                    font.pixelSize: 12
                }
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

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: "#20354b"
        }

        Label {
            text: "Voice Participants"
            color: "#d7e9fd"
            font.bold: true
            font.pixelSize: 12
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 5
            model: app.store.voiceParticipants

            delegate: Rectangle {
                id: participantRow
                required property var modelData
                width: ListView.view.width
                height: 38
                radius: 7
                color: "#132436"
                border.color: "#2f4a67"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    spacing: 8

                    Label {
                        Layout.fillWidth: true
                        text: participantRow.modelData.user
                              ? participantRow.modelData.user.displayName
                              : participantRow.modelData.userId
                        color: "#e5f3ff"
                        elide: Text.ElideRight
                    }

                    Label {
                        text: participantRow.modelData.muted ? "Muted" : "Live"
                        color: participantRow.modelData.muted ? "#f5c18e" : "#87dcb5"
                        font.pixelSize: 11
                    }
                }
            }
        }
    }
}
