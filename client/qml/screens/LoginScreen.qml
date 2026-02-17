import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    readonly property color bgTop: "#0a1220"
    readonly property color bgBottom: "#070b13"
    readonly property color panel: "#0f1929"
    readonly property color panelBorder: "#2a3f5d"
    readonly property color textPrimary: "#f0f6ff"
    readonly property color textMuted: "#89a7c8"
    readonly property color fieldBg: "#0a1320"
    readonly property color fieldBorder: "#314a68"
    readonly property color accent: "#4ba3ff"
    readonly property color accentHover: "#63b1ff"

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: root.bgTop }
            GradientStop { position: 1.0; color: root.bgBottom }
        }
    }

    Rectangle {
        width: Math.max(parent.width * 0.55, 520)
        height: Math.max(parent.height * 0.5, 420)
        x: -width * 0.35
        y: -height * 0.2
        radius: width / 2
        color: "#1e365755"
        antialiasing: true
    }

    Rectangle {
        id: authCard
        width: Math.min(460, parent.width - 48)
        height: isRegister ? 440 : 400
        anchors.centerIn: parent
        radius: 18
        color: root.panel
        border.color: root.panelBorder
        border.width: 1
        antialiasing: true

        property bool isRegister: false

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: 4
            radius: 18
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#2ce1a8" }
                GradientStop { position: 0.55; color: "#4ba3ff" }
                GradientStop { position: 1.0; color: "#7ec3ff" }
            }
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 26
            spacing: 14

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Rectangle {
                    Layout.preferredWidth: 34
                    Layout.preferredHeight: 34
                    radius: 10
                    color: "#173455"
                    border.color: "#3d648f"

                    Label {
                        anchors.centerIn: parent
                        text: "V"
                        color: root.textPrimary
                        font.bold: true
                        font.pixelSize: 18
                    }
                }

                ColumnLayout {
                    spacing: 0

                    Label {
                        text: authCard.isRegister ? "Create account" : "Welcome to Voxter"
                        color: root.textPrimary
                        font.pixelSize: 21
                        font.bold: true
                    }
                    Label {
                        text: authCard.isRegister ? "Set up your identity and join channels" : "Sign in to continue your conversations"
                        color: root.textMuted
                        font.pixelSize: 12
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                radius: 10
                color: "#0b1422"
                border.color: "#2a3f5d"

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 4
                    spacing: 6

                    Button {
                        id: loginTabButton
                        Layout.fillWidth: true
                        text: "Login"
                        flat: true
                        onClicked: authCard.isRegister = false
                        background: Rectangle {
                            radius: 8
                            color: !authCard.isRegister ? "#1b3048" : "transparent"
                        }
                        contentItem: Label {
                            text: loginTabButton.text
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            color: !authCard.isRegister ? "#f5fbff" : "#88a8cb"
                            font.bold: true
                        }
                    }

                    Button {
                        id: registerTabButton
                        Layout.fillWidth: true
                        text: "Register"
                        flat: true
                        onClicked: authCard.isRegister = true
                        background: Rectangle {
                            radius: 8
                            color: authCard.isRegister ? "#1b3048" : "transparent"
                        }
                        contentItem: Label {
                            text: registerTabButton.text
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            color: authCard.isRegister ? "#f5fbff" : "#88a8cb"
                            font.bold: true
                        }
                    }
                }
            }

            TextField {
                id: email
                Layout.fillWidth: true
                Layout.preferredHeight: 42
                placeholderText: "Email"
                color: root.textPrimary
                placeholderTextColor: root.textMuted
                selectionColor: "#2f5c87"
                selectedTextColor: "#ffffff"
                background: Rectangle {
                    radius: 10
                    color: root.fieldBg
                    border.color: parent.activeFocus ? root.accent : root.fieldBorder
                    border.width: 1
                }
            }

            TextField {
                id: displayName
                visible: authCard.isRegister
                Layout.fillWidth: true
                Layout.preferredHeight: 42
                placeholderText: "Display name"
                color: root.textPrimary
                placeholderTextColor: root.textMuted
                selectionColor: "#2f5c87"
                selectedTextColor: "#ffffff"
                background: Rectangle {
                    radius: 10
                    color: root.fieldBg
                    border.color: parent.activeFocus ? root.accent : root.fieldBorder
                    border.width: 1
                }
            }

            TextField {
                id: password
                Layout.fillWidth: true
                Layout.preferredHeight: 42
                echoMode: TextInput.Password
                placeholderText: "Password"
                color: root.textPrimary
                placeholderTextColor: root.textMuted
                selectionColor: "#2f5c87"
                selectedTextColor: "#ffffff"
                background: Rectangle {
                    radius: 10
                    color: root.fieldBg
                    border.color: parent.activeFocus ? root.accent : root.fieldBorder
                    border.width: 1
                }
            }

            Label {
                Layout.fillWidth: true
                visible: app.auth.error.length > 0
                text: app.auth.error
                color: "#ff9e9e"
                wrapMode: Text.Wrap
                font.pixelSize: 12
            }

            Item { Layout.fillHeight: true; visible: false }

            Button {
                id: authSubmitButton
                Layout.fillWidth: true
                Layout.preferredHeight: 44
                enabled: !app.auth.busy
                text: app.auth.busy ? "Please wait..." : (authCard.isRegister ? "Create account" : "Sign in")
                onClicked: {
                    if (authCard.isRegister) {
                        app.auth.registerUser(email.text, password.text, displayName.text)
                    } else {
                        app.auth.login(email.text, password.text)
                    }
                }
                background: Rectangle {
                    radius: 10
                    color: authSubmitButton.down ? "#3b8ddd" : (authSubmitButton.hovered ? root.accentHover : root.accent)
                }
                contentItem: Label {
                    text: authSubmitButton.text
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: "#ffffff"
                    font.bold: true
                    font.pixelSize: 14
                }
            }
        }
    }
}
