// SPDX-License-Identifier: GPL-3.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

Page {
    id: page

    signal play(mode: int, seedHex: string)

    background: Rectangle { color: AppTheme.background }

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - 48, 360)
        spacing: 16

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: "STROOPY"
            font.pixelSize: 48
            font.bold: true
            font.letterSpacing: 4
            color: AppTheme.ink(2)
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            Layout.bottomMargin: 16
            text: qsTr("Does the colour match the word?")
            color: AppTheme.textMuted
        }

        Repeater {
            model: [
                { mode: GameController.Classic, name: qsTr("Classic"), hint: qsTr("True or false?"), slot: 0 },
                { mode: GameController.Chrono, name: qsTr("Chrono"), hint: qsTr("30 seconds, as many as you can"), slot: 1 },
                { mode: GameController.Find, name: qsTr("Find the colour"), hint: qsTr("Tap the colour the word names"), slot: 2 },
                { mode: GameController.Tap, name: qsTr("Tap the true one"), hint: qsTr("Only one card is right"), slot: 3 }
            ]

            delegate: Button {
                id: modeButton

                required property var modelData

                Layout.fillWidth: true
                Layout.preferredHeight: 72
                Material.background: AppTheme.ink(modelData.slot)
                Material.foreground: AppTheme.background
                onClicked: page.play(modelData.mode, seedField.acceptableInput ? seedField.text : "")

                contentItem: Column {
                    Label {
                        text: modeButton.modelData.name
                        font.pixelSize: 20
                        font.bold: true
                        color: AppTheme.background
                    }
                    Label {
                        text: modeButton.modelData.hint
                        color: AppTheme.background
                        opacity: 0.8
                    }
                }
            }
        }

        TextField {
            id: seedField

            Layout.fillWidth: true
            Layout.topMargin: 8
            placeholderText: qsTr("Seed (optional, 16 hex digits)")
            validator: RegularExpressionValidator { regularExpression: /[0-9a-fA-F]{16}/ }
            inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhUppercaseOnly
        }

        Switch {
            text: qsTr("Colour-blind palette")
            checked: AppTheme.colourBlind
            onToggled: AppTheme.colourBlind = checked
        }
    }
}
