import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Rectangle {
    id: root
    implicitHeight: 64
    radius: 11
    color: "#101b2a"
    border.color: "#27415d"
    border.width: 1

    property string selectedFilePath: ""

    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        TextField {
            id: input
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            placeholderText: app.store.currentDmThreadId.length > 0 ? "Message in DM..." : "Message #channel..."
            color: "#e9f4ff"
            placeholderTextColor: "#7999b9"
            onTextEdited: app.chat.sendTyping()
            onAccepted: sendAction.triggered()
            background: Rectangle {
                radius: 9
                color: "#0a1321"
                border.color: input.activeFocus ? "#4ba3ff" : "#304964"
            }
        }

        Button {
            id: attachButton
            text: selectedFilePath.length > 0 ? "Attached" : "Attach"
            onClicked: fileDialog.open()
            background: Rectangle {
                radius: 9
                color: attachButton.down ? "#2b547e" : (attachButton.hovered ? "#2f5d8a" : "#284f76")
            }
            contentItem: Label {
                text: attachButton.text
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                color: "#def0ff"
                font.pixelSize: 12
            }
        }

        Action {
            id: sendAction
            onTriggered: {
                if (selectedFilePath.length > 0) {
                    app.chat.uploadAttachment(selectedFilePath, input.text)
                    selectedFilePath = ""
                } else if (app.store.currentDmThreadId.length > 0) {
                    app.chat.sendDmMessage(input.text)
                } else {
                    app.chat.sendMessage(input.text)
                }
                input.text = ""
            }
        }

        Button {
            id: sendButton
            text: "Send"
            onClicked: sendAction.triggered()
            background: Rectangle {
                radius: 9
                color: sendButton.down ? "#2f7eca" : (sendButton.hovered ? "#3a95eb" : "#3588d8")
            }
            contentItem: Label {
                text: sendButton.text
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                color: "#ffffff"
                font.bold: true
            }
        }
    }

    FileDialog {
        id: fileDialog
        title: "Select attachment"
        onAccepted: selectedFilePath = selectedFile.toString()
    }
}
