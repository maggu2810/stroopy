// SPDX-License-Identifier: GPL-3.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

SubPage {
    id: page

    signal openBackup()

    title: qsTr("Settings")

    StackView.onActivated: languageBox.forceActiveFocus()

    ScrollView {
        id: scroll

        anchors.fill: parent
        contentWidth: availableWidth

    ColumnLayout {
        x: (scroll.availableWidth - width) / 2
        width: Math.min(scroll.availableWidth - 48, 560)
        spacing: 16

        Label {
            Layout.topMargin: 24
            text: qsTr("Language")
            color: AppTheme.textMuted
        }
        ComboBox {
            id: languageBox

            // Up/Down leave the box (as on every other control); Enter opens it.
            KeyNavigation.down: colourBlindSwitch

            // "" = follow system; language names are shown in their own language.
            readonly property var codes: ["", "en", "de"]

            Layout.fillWidth: true
            model: [qsTr("Follow system"), "English", "Deutsch"]
            currentIndex: Math.max(0, codes.indexOf(AppSettings.language))
            onActivated: index => AppSettings.language = codes[index]
        }

        Switch {
            id: colourBlindSwitch

            text: qsTr("Colour-blind palette")
            checked: AppSettings.colourBlind
            onToggled: AppSettings.colourBlind = checked
        }
        Switch {
            text: qsTr("Vibrate on failure")
            checked: AppSettings.vibration
            onToggled: AppSettings.vibration = checked
        }

        Label {
            Layout.topMargin: 8
            text: qsTr("Player name")
            color: AppTheme.textMuted
        }
        TextField {
            Layout.fillWidth: true
            text: AppSettings.playerName
            maximumLength: 24
            onEditingFinished: AppSettings.playerName = text
        }
        Label {
            Layout.fillWidth: true
            text: qsTr("Shown to other players. Names and scores travel unencrypted through public Nostr relays.")
            color: AppTheme.textMuted
            font.pixelSize: 12
            wrapMode: Text.Wrap
        }

        Label {
            Layout.topMargin: 8
            text: qsTr("Nostr relays for rooms you create (separated by spaces, empty = default)")
            color: AppTheme.textMuted
        }
        // A single line, so arrow keys and a TV remote don't get trapped in a text area.
        TextField {
            Layout.fillWidth: true
            text: AppSettings.relays.split("\n").join(" ")
            placeholderText: AppSettings.defaultRelays.join(" ")
            inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase | Qt.ImhUrlCharactersOnly
            onEditingFinished: AppSettings.relays = text.split(/[\s,]+/).join("\n")
        }

        Button {
            Layout.fillWidth: true
            Layout.topMargin: 24
            text: qsTr("Backup: export / import")
            onClicked: page.openBackup()
        }

        Button {
            Layout.fillWidth: true
            Layout.bottomMargin: 24
            text: qsTr("Clear highscores")
            flat: true
            Material.foreground: AppTheme.bad
            onClicked: confirmClear.open()
        }
    }
    }

    Dialog {
        id: confirmClear

        anchors.centerIn: parent
        title: qsTr("Clear all highscores?")
        standardButtons: Dialog.Yes | Dialog.No
        onAccepted: Highscores.clear()
    }
}
