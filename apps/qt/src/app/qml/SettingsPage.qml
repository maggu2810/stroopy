// SPDX-License-Identifier: GPL-3.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

SubPage {
    id: page

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

            // "" = follow system; language names are shown in their own language.
            readonly property var codes: ["", "en", "de"]

            Layout.fillWidth: true
            model: [qsTr("Follow system"), "English", "Deutsch"]
            currentIndex: Math.max(0, codes.indexOf(AppSettings.language))
            onActivated: index => AppSettings.language = codes[index]
        }

        Switch {
            text: qsTr("Colour-blind palette")
            checked: AppSettings.colourBlind
            onToggled: AppSettings.colourBlind = checked
        }
        Switch {
            text: qsTr("Vibrate on failure")
            checked: AppSettings.vibration
            onToggled: AppSettings.vibration = checked
        }

        Button {
            Layout.fillWidth: true
            Layout.topMargin: 24
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
