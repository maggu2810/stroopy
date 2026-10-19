// SPDX-License-Identifier: GPL-3.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

Page {
    id: page

    readonly property bool landscape: width > height

    signal play(mode: int)
    signal openHighscores()
    signal openEnterCode()
    signal openHelp()
    signal openSettings()

    background: Rectangle { color: AppTheme.background }
    StackView.onActivated: firstMode.forceActiveFocus()

    GridLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - 48, page.landscape ? 960 : 420)
        columns: page.landscape ? 2 : 1
        columnSpacing: 48
        rowSpacing: 24

        ColumnLayout {
            Layout.alignment: Qt.AlignCenter
            spacing: 8

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
                text: qsTr("Does the colour match the word?")
                color: AppTheme.textMuted
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 16

            GridLayout {
                Layout.fillWidth: true
                columns: page.landscape ? 2 : 1
                rowSpacing: 12
                columnSpacing: 12

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
                        required property int index

                        Layout.fillWidth: true
                        Layout.preferredHeight: 72
                        Material.background: AppTheme.ink(modelData.slot)
                        onClicked: page.play(modelData.mode)
                        Component.onCompleted: if (index === 0) firstMode.target = modeButton

                        contentItem: RowLayout {
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 0

                                Label {
                                    Layout.fillWidth: true
                                    text: AppTheme.modeName(modeButton.modelData.mode)
                                    font.pixelSize: 20
                                    font.bold: true
                                    color: AppTheme.background
                                    elide: Text.ElideRight
                                }
                                Label {
                                    Layout.fillWidth: true
                                    text: modeButton.modelData.hint
                                    color: AppTheme.background
                                    opacity: 0.8
                                    elide: Text.ElideRight
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
            }

            GridLayout {
                Layout.fillWidth: true
                columns: page.landscape ? 4 : 2
                rowSpacing: 0

                LinkButton { text: qsTr("Highscores"); onClicked: page.openHighscores() }
                LinkButton { text: qsTr("Enter code"); onClicked: page.openEnterCode() }
                LinkButton { text: qsTr("Help"); onClicked: page.openHelp() }
                LinkButton { text: qsTr("Settings"); onClicked: page.openSettings() }
            }
        }
    }

    QtObject {
        id: firstMode

        property Item target

        function forceActiveFocus() {
            target?.forceActiveFocus()
        }
    }

    component LinkButton: Button {
        Layout.fillWidth: true
        flat: true
    }
}
