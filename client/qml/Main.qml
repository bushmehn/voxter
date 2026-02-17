import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Voxter 1.0

ApplicationWindow {
    id: root
    width: 1440
    height: 900
    minimumWidth: 1100
    minimumHeight: 700
    visible: true
    title: "Voxter"
    color: "#0c1118"
    font.family: Qt.platform.os === "windows" ? "Segoe UI Variable Text" : "Noto Sans"
    font.pixelSize: 14
    onClosing: function(close) {
        close.accepted = true
        app.shutdown()
        Qt.callLater(Qt.quit)
    }

    function hotkeyFromEvent(event) {
        if (event.key === Qt.Key_Space) {
            return "SPACE"
        }
        if (event.text && event.text.length === 1) {
            return event.text.toUpperCase()
        }
        return ""
    }

    function isPttHotkeyEvent(event) {
        return hotkeyFromEvent(event) === app.voice.pttHotkey
    }

    function hasTextInputFocus() {
        let item = root.activeFocusItem
        while (item) {
            if (item.hasOwnProperty("cursorPosition")
                || item.hasOwnProperty("selectedText")
                || item.hasOwnProperty("echoMode")
                || item.hasOwnProperty("inputMethodComposing")) {
                return true
            }
            item = item.parent
        }
        return false
    }

    Item {
        id: pttKeyCatcher
        anchors.fill: parent
        focus: true

        Keys.onPressed: function(event) {
            if (event.isAutoRepeat) {
                return
            }
            if (!app.store.authenticated || !app.voice.connected) {
                return
            }
            if (app.voice.activationMode !== "PUSH_TO_TALK") {
                return
            }
            if (!app.voice.pttHotkeyEnabled || root.hasTextInputFocus()) {
                return
            }
            if (root.isPttHotkeyEvent(event)) {
                app.voice.setPttPressed(true)
                event.accepted = true
            }
        }

        Keys.onReleased: function(event) {
            if (event.isAutoRepeat) {
                return
            }
            if (!app.store.authenticated || !app.voice.connected) {
                return
            }
            if (app.voice.activationMode !== "PUSH_TO_TALK") {
                return
            }
            if (!app.voice.pttHotkeyEnabled || root.hasTextInputFocus()) {
                return
            }
            if (root.isPttHotkeyEvent(event)) {
                app.voice.setPttPressed(false)
                event.accepted = true
            }
        }
    }

    StackLayout {
        anchors.fill: parent
        currentIndex: app.store.authenticated ? 1 : 0

        LoginScreen {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        MainScreen {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}
