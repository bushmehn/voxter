import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: "#0d1724"
    border.color: "#24384f"
    border.width: 1

    function currentGuildOwnerId() {
        for (let i = 0; i < app.store.guilds.length; ++i) {
            const guild = app.store.guilds[i]
            if (guild.id === app.store.currentGuildId) {
                return guild.ownerId || ""
            }
        }
        return ""
    }

    function memberIsOwner(member) {
        if (!member) {
            return false
        }
        if (!!member.isOwner) {
            return true
        }
        return member.id && member.id === currentGuildOwnerId()
    }

    function memberIsAdmin(member) {
        if (!member) {
            return false
        }
        return memberIsOwner(member) || !!member.isAdmin
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        Label {
            text: "Server Members"
            color: "#d7e9fd"
            font.bold: true
            font.pixelSize: 14
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 6
            model: app.store.guildMembers

            delegate: Rectangle {
                id: memberCard
                required property var modelData
                readonly property bool isOwnerMember: root.memberIsOwner(memberCard.modelData)
                readonly property bool isAdminMember: root.memberIsAdmin(memberCard.modelData)
                width: ListView.view.width
                height: 50
                radius: 8
                color: "#132436"
                border.color: "#2f4a67"

                Column {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    anchors.topMargin: 6
                    spacing: 2

                    Row {
                        width: parent.width
                        spacing: 6

                        Label {
                            width: memberCard.isAdminMember ? parent.width - 64 : parent.width
                            text: memberCard.modelData.displayName ? memberCard.modelData.displayName : memberCard.modelData.id
                            color: "#e5f3ff"
                            elide: Text.ElideRight
                        }

                        Rectangle {
                            visible: memberCard.isAdminMember
                            width: memberCard.isOwnerMember ? 42 : 40
                            height: 16
                            radius: 8
                            color: memberCard.isOwnerMember ? "#f0b232" : "#5865f2"
                            border.color: memberCard.isOwnerMember ? "#ffd67a" : "#7f8bff"

                            Label {
                                anchors.centerIn: parent
                                text: memberCard.isOwnerMember ? "OWNER" : "ADMIN"
                                color: "#ffffff"
                                font.pixelSize: 9
                                font.bold: true
                            }
                        }
                    }

                    Row {
                        spacing: 6

                        Rectangle {
                            width: 8
                            height: 8
                            radius: 4
                            y: 3
                            color: {
                                const presence = memberCard.modelData.presence
                                if (presence === "ONLINE") {
                                    return "#35d78f"
                                }
                                if (presence === "IDLE") {
                                    return "#f0bf65"
                                }
                                if (presence === "DND") {
                                    return "#f26b78"
                                }
                                return "#6f8297"
                            }
                        }

                        Label {
                            text: memberCard.modelData.presence ? memberCard.modelData.presence : "OFFLINE"
                            color: "#9eb8d2"
                            font.pixelSize: 11
                            elide: Text.ElideRight
                        }
                    }
                }
            }
        }
    }
}
