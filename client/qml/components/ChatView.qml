import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Voxter 1.0
import "."

Rectangle {
    id: root
    color: "#0a121d"
    border.color: "#22384d"
    border.width: 1

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        ListView {
            id: messagesList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 4
            model: app.store.messages

            delegate: Rectangle {
                id: messageRow
                required property var modelData
                width: ListView.view.width
                radius: 10
                color: hoverHandler.hovered ? "#112134" : "transparent"
                height: contentColumn.implicitHeight + 14

                ColumnLayout {
                    id: contentColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    anchors.top: parent.top
                    anchors.topMargin: 7
                    spacing: 4

                    RowLayout {
                        spacing: 8

                        Label {
                            text: modelData.author && modelData.author.displayName ? modelData.author.displayName : modelData.authorId
                            color: "#d9ecff"
                            font.bold: true
                            font.pixelSize: 13
                        }

                        Label {
                            text: modelData.createdAt ? modelData.createdAt.toString().slice(0, 19).replace("T", " ") : ""
                            color: "#7f9dbc"
                            font.pixelSize: 11
                        }

                        Item { Layout.fillWidth: true }

                        Button {
                            id: reactButton
                            text: "+1"
                            onClicked: app.chat.reactToMessage(modelData.id, "thumbsup")
                            background: Rectangle {
                                radius: 7
                                color: reactButton.down ? "#2d5b85" : (reactButton.hovered ? "#2a5278" : "#223f5d")
                            }
                            contentItem: Label {
                                text: reactButton.text
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                color: "#dbeeff"
                                font.pixelSize: 11
                            }
                        }

                        Button {
                            id: editButton
                            text: "Edit"
                            onClicked: app.chat.editMessage(modelData.id, modelData.content + " (edited)")
                            background: Rectangle {
                                radius: 7
                                color: editButton.down ? "#3f4154" : (editButton.hovered ? "#37394a" : "#2f3140")
                            }
                            contentItem: Label {
                                text: editButton.text
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                color: "#d7dbf2"
                                font.pixelSize: 11
                            }
                        }

                        Button {
                            id: deleteButton
                            text: "Del"
                            onClicked: app.chat.deleteMessage(modelData.id)
                            background: Rectangle {
                                radius: 7
                                color: deleteButton.down ? "#60343f" : (deleteButton.hovered ? "#55303a" : "#472833")
                            }
                            contentItem: Label {
                                text: deleteButton.text
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                color: "#ffd5de"
                                font.pixelSize: 11
                            }
                        }
                    }

                    Label {
                        text: modelData.content
                        color: "#e9f4ff"
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                        lineHeight: 1.18
                    }

                    Repeater {
                        model: modelData.attachments ? modelData.attachments : []
                        delegate: Rectangle {
                            required property var modelData
                            Layout.fillWidth: true
                            implicitHeight: 28
                            radius: 6
                            color: "#111e30"
                            border.color: "#2d4967"

                            Label {
                                anchors.fill: parent
                                anchors.leftMargin: 8
                                anchors.rightMargin: 8
                                verticalAlignment: Text.AlignVCenter
                                text: "Attachment: " + modelData.fileName
                                color: "#93c3ff"
                                elide: Text.ElideRight
                            }
                        }
                    }
                }

                HoverHandler { id: hoverHandler }
            }
        }

        MessageComposer {
            Layout.fillWidth: true
            Layout.preferredHeight: implicitHeight
        }
    }
}
