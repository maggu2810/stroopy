// SPDX-License-Identifier: GPL-3.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

Page {
    id: page

    required property GameController controller
    readonly property bool wordMode: controller.mode !== GameController.Tap
    readonly property bool trueFalseMode: controller.mode === GameController.Classic
                                          || controller.mode === GameController.Chrono

    signal quit()

    background: Rectangle { color: AppTheme.background }

    Connections {
        target: page.controller

        function onChallengeChanged() {
            if (page.controller.phase !== GameController.Running)
                return
            if (page.controller.mode === GameController.Chrono) {
                if (page.controller.n === 1)
                    chronoBar.restart(page.controller.limitMs)
            } else if (page.wordMode) {
                ring.restart(page.controller.limitMs)
            } else {
                roundBar.restart(page.controller.limitMs)
            }
        }
        function onAnswered(correct: bool) {
            flash.color = correct ? AppTheme.good : AppTheme.bad
            flashAnim.restart()
        }
        function onPhaseChanged() {
            if (page.controller.phase !== GameController.Running) {
                ring.stop()
                roundBar.stop()
                chronoBar.stop()
            }
        }
    }

    Rectangle {
        id: flash

        anchors.fill: parent
        opacity: 0

        NumberAnimation on opacity {
            id: flashAnim

            running: false
            from: 0.25
            to: 0
            duration: 250
        }
    }

    header: RowLayout {
        spacing: 16

        ToolButton {
            text: "✕"
            onClicked: page.quit()
        }
        Item { Layout.fillWidth: true }
        Label {
            text: qsTr("#%1").arg(page.controller.n)
            color: AppTheme.textMuted
            font.pixelSize: 18
        }
        Label {
            Layout.rightMargin: 16
            text: page.controller.score
            color: AppTheme.text
            font.pixelSize: 32
            font.bold: true
        }
    }

    // Countdown 3-2-1-GO
    Label {
        anchors.centerIn: parent
        visible: page.controller.phase === GameController.Countdown
        text: page.controller.countdown > 0 ? page.controller.countdown : qsTr("GO")
        font.pixelSize: 120
        font.bold: true
        color: AppTheme.text
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 24
        visible: page.controller.phase === GameController.Running

        // Chrono: total time bar
        Bar {
            id: chronoBar

            Layout.fillWidth: true
            visible: page.controller.mode === GameController.Chrono
        }

        // Word modes: word inside the ring timer
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: page.wordMode

            RingTimer {
                id: ring

                anchors.centerIn: parent
                width: Math.min(parent.width, parent.height)
                height: width
                visible: page.controller.mode !== GameController.Chrono
                color: AppTheme.ink(page.controller.ink)
                strokeWidth: Math.max(4, 24 * (1 - page.controller.n / 300))
            }

            Label {
                anchors.centerIn: parent
                text: AppTheme.word(page.controller.word)
                color: AppTheme.ink(page.controller.ink)
                font.pixelSize: Math.min(56, ring.width / Math.max(5, text.length) * 1.4)
                font.bold: true
            }
        }

        // Classic / chrono: TRUE / FALSE
        RowLayout {
            Layout.fillWidth: true
            visible: page.trueFalseMode
            spacing: 16

            AnswerButton {
                text: qsTr("FALSE")
                fill: AppTheme.bad
                onClicked: page.controller.answer(0)
            }
            AnswerButton {
                text: qsTr("TRUE")
                fill: AppTheme.good
                onClicked: page.controller.answer(1)
            }
        }

        // Find: one button per colour in the pool
        GridLayout {
            Layout.fillWidth: true
            visible: page.controller.mode === GameController.Find
            columns: 4
            rowSpacing: 12
            columnSpacing: 12

            Repeater {
                model: page.controller.pool

                delegate: AnswerButton {
                    required property int index

                    fill: AppTheme.ink(index)
                    Layout.preferredHeight: 64
                    onClicked: page.controller.answer(index)
                }
            }
        }

        // Tap: four cards, one of them true
        Bar {
            id: roundBar

            Layout.fillWidth: true
            visible: page.controller.mode === GameController.Tap
        }

        GridLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: page.controller.mode === GameController.Tap
            columns: 2
            rowSpacing: 16
            columnSpacing: 16

            Repeater {
                model: page.controller.cardInks.length

                delegate: Button {
                    id: card

                    required property int index

                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Material.background: AppTheme.surface
                    onClicked: page.controller.answer(index)

                    contentItem: Label {
                        text: AppTheme.word(page.controller.cardWords[card.index] ?? 0)
                        color: AppTheme.ink(page.controller.cardInks[card.index] ?? 0)
                        font.pixelSize: 26
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }
    }

    component AnswerButton: Button {
        property color fill

        Layout.fillWidth: true
        Layout.preferredHeight: 88
        Material.background: fill
        Material.foreground: AppTheme.background
        font.pixelSize: 24
        font.bold: true
    }

    component Bar: Rectangle {
        id: bar

        property real progress: 1.0

        function restart(duration: int) {
            anim.stop()
            progress = 1.0
            anim.duration = duration
            anim.start()
        }
        function stop() {
            anim.stop()
        }

        implicitHeight: 10
        radius: 5
        color: AppTheme.surface

        Rectangle {
            width: parent.width * bar.progress
            height: parent.height
            radius: parent.radius
            color: AppTheme.text
        }

        NumberAnimation {
            id: anim

            target: bar
            property: "progress"
            from: 1.0
            to: 0.0
        }
    }
}
