// SPDX-License-Identifier: GPL-3.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

Page {
    id: page

    signal play(mode: int)
    signal openHighscores()
    signal openEnterCode()
    signal openSettings()

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
                { mode: GameController.Classic, hint: qsTr("True or false?"), slot: 0 },
                { mode: GameController.Chrono, hint: qsTr("30 seconds, as many as you can"), slot: 1 },
                { mode: GameController.Find, hint: qsTr("Tap the colour the word names"), slot: 2 },
                { mode: GameController.Tap, hint: qsTr("Only one card is right"), slot: 3 }
            ]

            delegate: Button {
                id: modeButton

                required property var modelData

                Layout.fillWidth: true
                Layout.preferredHeight: 72
                Material.background: AppTheme.ink(modelData.slot)
                onClicked: page.play(modelData.mode)

                contentItem: RowLayout {
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0

                        Label {
                            text: AppTheme.modeName(modeButton.modelData.mode)
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
                    Label {
                        // Re-evaluated whenever highscores change.
                        text: Highscores.revision >= 0 && Highscores.best(modeButton.modelData.mode) > 0
                              ? "★ " + Highscores.best(modeButton.modelData.mode) : ""
                        font.pixelSize: 18
                        font.bold: true
                        color: AppTheme.background
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 8

            Button {
                Layout.fillWidth: true
                text: qsTr("Highscores")
                flat: true
                onClicked: page.openHighscores()
            }
            Button {
                Layout.fillWidth: true
                text: qsTr("Enter code")
                flat: true
                onClicked: page.openEnterCode()
            }
            Button {
                Layout.fillWidth: true
                text: qsTr("Settings")
                flat: true
                onClicked: page.openSettings()
            }
        }
    }
}
