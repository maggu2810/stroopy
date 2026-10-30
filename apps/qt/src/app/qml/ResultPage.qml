// SPDX-License-Identifier: GPL-3.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

Page {
    id: page

    required property GameController controller
    // 1-based highscore rank of this game, 0 if it did not make the list.
    property int rank: 0
    readonly property bool landscape: width > height
    readonly property string code: Sharing.encode(controller.rules, controller.mode, controller.seedHex,
                                                  controller.score, controller.reached)

    signal replay()
    signal newGame()
    signal home()

    background: Rectangle { color: AppTheme.background }
    StackView.onActivated: newGameButton.forceActiveFocus()

    Keys.onPressed: event => {
        if (event.key === Qt.Key_R) {
            page.replay()
            event.accepted = true
        } else if (event.key === Qt.Key_N) {
            page.newGame()
            event.accepted = true
        } else if (event.key === Qt.Key_Escape || event.key === Qt.Key_Back) {
            page.home()
            event.accepted = true
        }
    }

    Connections {
        target: Gamepad
        enabled: page.StackView.status === StackView.Active

        function onPressed(button: int) {
            if (button === Gamepad.B)
                page.home()
        }
    }

    GridLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - 48, page.landscape ? 900 : 380)
        columns: page.landscape ? 2 : 1
        columnSpacing: 48
        rowSpacing: 12

        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignCenter
            spacing: 8

            Label {
                Layout.alignment: Qt.AlignHCenter
                text: AppTheme.modeName(page.controller.mode)
                color: AppTheme.textMuted
                font.pixelSize: 18
            }
            Label {
                Layout.alignment: Qt.AlignHCenter
                text: page.controller.score
                font.pixelSize: 96
                font.bold: true
                color: page.rank === 1 ? AppTheme.accent : AppTheme.text
            }
            Label {
                Layout.alignment: Qt.AlignHCenter
                visible: page.rank > 0
                text: page.rank === 1 ? qsTr("New best!") : qsTr("Highscore #%1").arg(page.rank)
                color: AppTheme.accent
                font.pixelSize: 22
                font.bold: true
            }
            Label {
                Layout.alignment: Qt.AlignHCenter
                text: page.controller.mode === GameController.Chrono
                      ? qsTr("time is up at #%1").arg(page.controller.reached)
                      : qsTr("out at #%1").arg(page.controller.reached)
                color: AppTheme.textMuted
                font.pixelSize: 18
            }
            Label {
                Layout.alignment: Qt.AlignHCenter
                visible: page.controller.isChallenge
                font.pixelSize: 18
                font.bold: true
                color: page.controller.score > page.controller.challengeScore ? AppTheme.good
                     : page.controller.score < page.controller.challengeScore ? AppTheme.bad : AppTheme.text
                text: page.controller.score > page.controller.challengeScore ? qsTr("You beat %1!").arg(page.controller.challengeScore)
                    : page.controller.score < page.controller.challengeScore ? qsTr("%1 to beat, try again!").arg(page.controller.challengeScore)
                    : qsTr("A draw at %1!").arg(page.controller.challengeScore)
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8

            TextField {
                Layout.fillWidth: true
                text: page.code
                readOnly: true
                focusPolicy: Qt.ClickFocus
                horizontalAlignment: Text.AlignHCenter
                font.family: "monospace"
                font.pixelSize: 13
            }
            AppButton {
                Layout.fillWidth: true
                text: qsTr("Copy code to share")
                onClicked: {
                    Sharing.copyToClipboard(qsTr("Can you beat %1 in Stroopy %2? %3")
                                            .arg(page.controller.score)
                                            .arg(AppTheme.modeName(page.controller.mode))
                                            .arg(page.code))
                    copied.open()
                }
            }
            AppButton {
                Layout.fillWidth: true
                Layout.topMargin: 12
                text: AppTheme.keyHints ? qsTr("Play this sequence again") + "  [R]" : qsTr("Play this sequence again")
                onClicked: page.replay()
            }
            AppButton {
                id: newGameButton

                Layout.fillWidth: true
                text: AppTheme.keyHints ? qsTr("New game") + "  [N]" : qsTr("New game")
                primary: true
                onClicked: page.newGame()
            }
            AppButton {
                Layout.fillWidth: true
                text: qsTr("Home")
                flat: true
                onClicked: page.home()
            }
        }
    }

    ToolTip {
        id: copied

        text: qsTr("Message with code copied to the clipboard")
        timeout: 2000
        x: (page.width - width) / 2
        y: page.height - height - 48
    }
}
