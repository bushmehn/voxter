import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    signal openSettingsRequested()
    property int bottomInset: 0
    color: "#16181c"
    border.color: "#262a31"
    border.width: 1

    function initials(name) {
        if (!name || name.length === 0) {
            return "?"
        }
        return name.slice(0, 2).toUpperCase()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 8
        anchors.topMargin: 8
        anchors.bottomMargin: 8 + root.bottomInset
        spacing: 8

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 52

            Button {
                id: addServerButton
                anchors.centerIn: parent
                width: 46
                height: 46
                text: "+"
                onClicked: guildDialog.open()
                background: Rectangle {
                    radius: addServerButton.hovered ? 15 : 23
                    color: addServerButton.down ? "#2b8eea" : (addServerButton.hovered ? "#3ba55d" : "#2b2d31")
                    border.color: "#40444b"
                    Behavior on radius { NumberAnimation { duration: 120 } }
                }
                contentItem: Label {
                    text: addServerButton.text
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: "#ffffff"
                    font.pixelSize: 22
                    font.bold: true
                }
                ToolTip.visible: hovered
                ToolTip.text: "Create or Join"
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: "#2f3136"
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 0
            Layout.preferredHeight: 1
            clip: true
            spacing: 8
            model: app.store.guilds

            delegate: Item {
                id: itemRoot
                required property var modelData
                width: ListView.view.width
                height: 54

                Rectangle {
                    width: 4
                    height: app.store.currentGuildId === itemRoot.modelData.id ? 34 : 10
                    radius: 2
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    color: "#ffffff"
                    visible: app.store.currentGuildId === itemRoot.modelData.id || mouseArea.containsMouse
                    Behavior on height { NumberAnimation { duration: 120 } }
                }

                Rectangle {
                    id: badge
                    width: 46
                    height: 46
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.horizontalCenter: parent.horizontalCenter
                    radius: app.store.currentGuildId === itemRoot.modelData.id ? 15 : 23
                    color: app.store.currentGuildId === itemRoot.modelData.id
                           ? "#5865f2"
                           : (mouseArea.containsMouse ? "#5865f2" : "#2b2d31")
                    border.color: "#40444b"
                    Behavior on radius { NumberAnimation { duration: 120 } }
                    Behavior on color { ColorAnimation { duration: 120 } }

                    Label {
                        anchors.centerIn: parent
                        text: root.initials(itemRoot.modelData.name)
                        color: "#f2f3f5"
                        font.bold: true
                        font.pixelSize: 14
                    }
                }

                ToolTip.visible: mouseArea.containsMouse
                ToolTip.text: itemRoot.modelData.name || "Unnamed"
                ToolTip.delay: 250

                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: app.guild.selectGuild(itemRoot.modelData.id)
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 46

            Button {
                id: settingsButton
                anchors.centerIn: parent
                width: 46
                height: 38
                text: "SET"
                onClicked: root.openSettingsRequested()
                background: Rectangle {
                    radius: 10
                    color: settingsButton.down ? "#29405c"
                                               : (settingsButton.hovered ? "#22354b" : "#1c2b3d")
                    border.color: "#3a5778"
                }
                contentItem: Label {
                    text: settingsButton.text
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: "#d7e9fd"
                    font.pixelSize: 10
                    font.bold: true
                }
                ToolTip.visible: hovered
                ToolTip.text: "Settings"
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: "#2f3136"
        }
    }

    Dialog {
        id: guildDialog
        parent: Overlay.overlay
        width: 380
        modal: true
        title: "Server"
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        standardButtons: Dialog.NoButton

        ColumnLayout {
            anchors.fill: parent
            spacing: 10

            TabBar {
                id: serverTabBar
                Layout.fillWidth: true
                TabButton { text: "Create" }
                TabButton { text: "Join" }
            }

            StackLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 88
                currentIndex: serverTabBar.currentIndex

                Item {
                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 8
                        TextField {
                            id: createGuildName
                            Layout.fillWidth: true
                            placeholderText: "Server name"
                        }
                    }
                }

                Item {
                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 8
                        TextField {
                            id: joinInviteCode
                            Layout.fillWidth: true
                            placeholderText: "Invite code"
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Button {
                    Layout.fillWidth: true
                    text: "Cancel"
                    onClicked: guildDialog.close()
                }

                Button {
                    Layout.fillWidth: true
                    text: serverTabBar.currentIndex === 0 ? "Create" : "Join"
                    onClicked: {
                        if (serverTabBar.currentIndex === 0) {
                            app.guild.createGuild(createGuildName.text)
                            createGuildName.text = ""
                        } else {
                            app.guild.joinInvite(joinInviteCode.text)
                            joinInviteCode.text = ""
                        }
                        guildDialog.close()
                    }
                }
            }
        }
    }

}
