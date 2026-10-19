// SPDX-License-Identifier: GPL-3.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

SubPage {
    id: page

    readonly property list<int> modes: [GameController.Classic, GameController.Chrono,
                                        GameController.Find, GameController.Tap]
    readonly property int mode: modes[tabs.currentIndex]

    signal playEntry(mode: int, seedHex: string)

    title: qsTr("Highscores")
    StackView.onActivated: tabs.forceActiveFocus()

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        TabBar {
            id: tabs

            Layout.fillWidth: true

            Repeater {
                model: page.modes

                delegate: TabButton {
                    required property int modelData

                    text: AppTheme.modeName(modelData)
                }
            }
        }

        ListView {
            id: list

            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: Highscores.revision >= 0 ? Highscores.entries(page.mode) : []

            delegate: ItemDelegate {
                id: entry

                required property var modelData
                required property int index

                width: ListView.view.width

                contentItem: RowLayout {
                    spacing: 12

                    Label {
                        Layout.preferredWidth: 28
                        text: entry.index + 1
                        color: AppTheme.textMuted
                        font.pixelSize: 18
                    }
                    Label {
                        Layout.preferredWidth: 56
                        text: entry.modelData.score
                        color: entry.index === 0 ? AppTheme.accent : AppTheme.text
                        font.pixelSize: 26
                        font.bold: true
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0

                        Label {
                            text: entry.modelData.challenge ? qsTr("challenge · out at #%1").arg(entry.modelData.reached)
                                                            : qsTr("out at #%1").arg(entry.modelData.reached)
                            color: AppTheme.text
                        }
                        Label {
                            text: Qt.formatDateTime(entry.modelData.time, Locale.ShortFormat)
                            color: AppTheme.textMuted
                            font.pixelSize: 12
                        }
                    }
                    ToolButton {
                        text: qsTr("Share")
                        onClicked: {
                            Sharing.copyToClipboard(Sharing.encode(entry.modelData.rules, entry.modelData.mode,
                                                                   entry.modelData.seedHex, entry.modelData.score,
                                                                   entry.modelData.reached))
                            copied.open()
                        }
                    }
                    ToolButton {
                        text: "▶"
                        enabled: entry.modelData.rules === 1
                        onClicked: page.playEntry(entry.modelData.mode, entry.modelData.seedHex)
                    }
                }
            }

            Label {
                anchors.centerIn: parent
                visible: list.count === 0
                text: qsTr("No highscores yet")
                color: AppTheme.textMuted
            }
        }
    }

    ToolTip {
        id: copied

        text: qsTr("Code copied to the clipboard")
        timeout: 2000
        x: (page.width - width) / 2
        y: page.height - height - 48
    }
}
