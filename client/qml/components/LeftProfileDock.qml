import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: "#111214"
    border.color: "#2b2d31"
    border.width: 1
    radius: 12

    function initials(name) {
        if (!name || name.length === 0) {
            return "?"
        }
        return name.slice(0, 2).toUpperCase()
    }

    function presenceColor(status) {
        if (status === "ONLINE") {
            return "#23a559"
        }
        if (status === "IDLE") {
            return "#f0b232"
        }
        if (status === "DND") {
            return "#f23f43"
        }
        return "#747f8d"
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        Item {
            Layout.preferredWidth: 44
            Layout.preferredHeight: 44

            Rectangle {
                anchors.fill: parent
                radius: 22
                color: "#313338"
                border.color: "#4f545c"
                clip: true

                Label {
                    anchors.centerIn: parent
                    text: root.initials(app.store.currentUser.displayName ? app.store.currentUser.displayName : app.store.currentUser.email)
                    color: "#f2f3f5"
                    font.bold: true
                    font.pixelSize: 13
                }
            }

            Rectangle {
                width: 12
                height: 12
                radius: 6
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                color: root.presenceColor(app.store.currentUser.presence)
                border.color: "#111214"
                border.width: 2
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 1

            Label {
                Layout.fillWidth: true
                text: app.store.currentUser.displayName ? app.store.currentUser.displayName : "User"
                color: "#f2f3f5"
                font.bold: true
                font.pixelSize: 13
                elide: Text.ElideRight
            }

            Label {
                Layout.fillWidth: true
                text: app.store.currentUser.presence ? app.store.currentUser.presence : "OFFLINE"
                color: "#a3a6aa"
                font.pixelSize: 11
                elide: Text.ElideRight
            }
        }

        RowLayout {
            spacing: 6

            Button {
                id: editProfileButton
                Layout.preferredWidth: 56
                text: "Edit"
                onClicked: {
                    profileName.text = app.store.currentUser.displayName ? app.store.currentUser.displayName : ""
                    profileDialog.open()
                }
                background: Rectangle {
                    radius: 8
                    color: editProfileButton.down ? "#3a3d44" : "#2b2d31"
                }
                contentItem: Label {
                    text: editProfileButton.text
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: "#dbdee1"
                    font.pixelSize: 11
                }
            }

            Button {
                id: logoutButton
                Layout.preferredWidth: 56
                text: "Logout"
                onClicked: app.auth.logout()
                background: Rectangle {
                    radius: 8
                    color: logoutButton.down ? "#6b2f3a" : "#3a2027"
                }
                contentItem: Label {
                    text: logoutButton.text
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: "#ffd7de"
                    font.pixelSize: 11
                }
            }
        }
    }

    Dialog {
        id: profileDialog
        parent: Overlay.overlay
        width: 380
        modal: true
        title: "Profile Settings"
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        standardButtons: Dialog.NoButton

        ColumnLayout {
            anchors.fill: parent
            spacing: 10

            TextField {
                id: profileName
                Layout.fillWidth: true
                placeholderText: "Display Name"
            }

            Label {
                Layout.fillWidth: true
                visible: app.profile.error.length > 0
                text: app.profile.error
                color: "#ff9e9e"
                wrapMode: Text.Wrap
                font.pixelSize: 11
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Button {
                    Layout.fillWidth: true
                    text: "Cancel"
                    onClicked: profileDialog.close()
                }

                Button {
                    Layout.fillWidth: true
                    text: app.profile.busy ? "Saving..." : "Save"
                    enabled: !app.profile.busy
                    onClicked: {
                        app.profile.saveProfile(profileName.text)
                    }
                }
            }
        }
    }

    Connections {
        target: app.profile
        function onSaved() {
            profileDialog.close()
        }
    }
}
