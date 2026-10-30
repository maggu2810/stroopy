// SPDX-License-Identifier: GPL-3.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

// Room lobby with the ranking of the last game (docs/design/product.md, "Multiplayer").
SubPage {
    id: page

    readonly property bool landscape: width > height
    readonly property list<int> modes: [GameController.Classic, GameController.Chrono,
                                        GameController.Find, GameController.Tap]

    title: qsTr("Room")
    StackView.onActivated: (Multiplayer.isHost ? startButton : inviteButton).forceActiveFocus()

    ScrollView {
        id: scroll

        anchors.fill: parent
        contentWidth: availableWidth

        GridLayout {
            x: (scroll.availableWidth - width) / 2
            width: Math.min(scroll.availableWidth - 48, page.landscape ? 1100 : 560)
            columns: page.landscape ? 2 : 1
            columnSpacing: 48
            rowSpacing: 16

            ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                Layout.alignment: Qt.AlignTop
                Layout.topMargin: 24
                spacing: 12

                RowLayout {
                    spacing: 16

                    AppButton {
                        id: inviteButton

                        text: qsTr("Invite")
                        onClicked: {
                            Sharing.copyToClipboard(Multiplayer.inviteText())
                            copied.open()
                        }
                    }
                    Label {
                        text: qsTr("Relays: %1/%2").arg(Multiplayer.connected).arg(Multiplayer.relayCount)
                        color: Multiplayer.connected > 0 ? AppTheme.textMuted : AppTheme.bad
                    }
                }
                Label {
                    Layout.fillWidth: true
                    text: Multiplayer.relays.map(url => url.replace(/^wss?:\/\//, "")).join(" · ")
                    color: AppTheme.textMuted
                    wrapMode: Text.WrapAnywhere
                }

                Label {
                    visible: Multiplayer.ranking.length > 0
                    Layout.topMargin: 8
                    text: qsTr("Last game: %1").arg(AppTheme.modeName(Multiplayer.lastMode))
                    font.pixelSize: 20
                    font.bold: true
                    color: AppTheme.text
                }
                Repeater {
                    model: Multiplayer.ranking

                    delegate: RowLayout {
                        id: rankRow

                        required property var modelData
                        required property int index

                        Layout.fillWidth: true
                        spacing: 12

                        Label {
                            Layout.preferredWidth: 24
                            text: rankRow.index + 1
                            color: AppTheme.textMuted
                        }
                        Label {
                            Layout.fillWidth: true
                            text: rankRow.modelData.me ? qsTr("%1 (you)").arg(rankRow.modelData.name) : rankRow.modelData.name
                            color: rankRow.modelData.me ? AppTheme.accent : AppTheme.text
                            elide: Text.ElideRight
                        }
                        Label {
                            text: rankRow.modelData.score
                            font.pixelSize: 22
                            font.bold: true
                            color: AppTheme.text
                        }
                        Label {
                            Layout.preferredWidth: 110
                            horizontalAlignment: Text.AlignRight
                            text: rankRow.modelData.over ? qsTr("out at #%1").arg(rankRow.modelData.n)
                                                         : qsTr("playing #%1").arg(rankRow.modelData.n)
                            color: AppTheme.textMuted
                        }
                    }
                }

                Label {
                    Layout.topMargin: 8
                    text: qsTr("Players")
                    font.pixelSize: 20
                    font.bold: true
                    color: AppTheme.text
                }
                Repeater {
                    model: Multiplayer.players

                    delegate: Label {
                        required property var modelData

                        text: (modelData.host ? "★ " : "") + (modelData.me ? qsTr("%1 (you)").arg(modelData.name)
                                                                           : modelData.name)
                        color: AppTheme.text
                    }
                }
                Label {
                    Layout.fillWidth: true
                    visible: Multiplayer.players.length === 1
                    text: qsTr("Nobody else is here yet. Invite others with the invitation.")
                    color: AppTheme.textMuted
                    wrapMode: Text.Wrap
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                Layout.alignment: Qt.AlignTop
                Layout.topMargin: 24
                Layout.bottomMargin: 24
                spacing: 12

                AppButton {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 64
                    visible: Multiplayer.pending
                    text: qsTr("Join the running game")
                    primary: true
                    onClicked: Multiplayer.joinPending()
                }
                Label {
                    visible: Multiplayer.isHost
                    text: qsTr("Mode")
                    font.pixelSize: 20
                    font.bold: true
                    color: AppTheme.text
                }
                GridLayout {
                    Layout.fillWidth: true
                    visible: Multiplayer.isHost
                    columns: 2

                    Repeater {
                        model: page.modes

                        delegate: AppButton {
                            required property int modelData

                            Layout.fillWidth: true
                            text: AppTheme.modeName(modelData)
                            checkable: true
                            checked: Multiplayer.mode === modelData
                            radius: 10
                            onClicked: Multiplayer.setMode(modelData)
                        }
                    }
                }
                AppButton {
                    id: startButton

                    Layout.fillWidth: true
                    Layout.preferredHeight: 64
                    visible: Multiplayer.isHost
                    enabled: Multiplayer.canStart
                    text: qsTr("Start")
                    primary: true
                    onClicked: Multiplayer.start()
                }
                Label {
                    Layout.fillWidth: true
                    visible: !Multiplayer.isHost
                    text: Multiplayer.hostName.length > 0
                          ? qsTr("Waiting for %1 to start: %2").arg(Multiplayer.hostName).arg(AppTheme.modeName(Multiplayer.mode))
                          : qsTr("Waiting for the host…")
                    color: AppTheme.text
                    font.pixelSize: 18
                    wrapMode: Text.Wrap
                }
                AppButton {
                    Layout.fillWidth: true
                    text: qsTr("Leave room")
                    flat: true
                    Material.foreground: AppTheme.bad
                    onClicked: page.back()
                }
            }
        }
    }

    ToolTip {
        id: copied

        text: qsTr("Invitation copied to the clipboard")
        timeout: 2000
        x: (page.width - width) / 2
        y: page.height - height - 48
    }
}
