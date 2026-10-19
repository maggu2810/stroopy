// SPDX-License-Identifier: GPL-3.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

Page {
    id: page

    required property GameController controller

    signal replay()
    signal newGame()
    signal home()

    background: Rectangle { color: AppTheme.background }

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - 48, 360)
        spacing: 12

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: page.controller.score
            font.pixelSize: 96
            font.bold: true
            color: AppTheme.accent
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
            Layout.topMargin: 24
            text: qsTr("Seed")
            color: AppTheme.textMuted
        }
        TextField {
            Layout.fillWidth: true
            text: page.controller.seedHex
            readOnly: true
            horizontalAlignment: Text.AlignHCenter
            font.family: "monospace"
            font.pixelSize: 20
        }

        Button {
            Layout.fillWidth: true
            Layout.topMargin: 24
            text: qsTr("Play this sequence again")
            onClicked: page.replay()
        }
        Button {
            Layout.fillWidth: true
            text: qsTr("New game")
            highlighted: true
            onClicked: page.newGame()
        }
        Button {
            Layout.fillWidth: true
            text: qsTr("Home")
            flat: true
            onClicked: page.home()
        }
    }
}
