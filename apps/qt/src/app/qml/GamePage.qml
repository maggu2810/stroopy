// SPDX-License-Identifier: GPL-3.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

// Play view for all modes. Layouts and bindings: docs/design/product.md, "Layout" and "Input".
FocusScope {
    id: page

    required property GameController controller
    readonly property bool landscape: width > height
    readonly property bool running: controller.phase === GameController.Running
    readonly property bool trueFalseMode: controller.mode === GameController.Classic
                                          || controller.mode === GameController.Chrono
    readonly property bool findMode: controller.mode === GameController.Find
    readonly property bool tapMode: controller.mode === GameController.Tap

    // Diamond directions: 0 up, 1 right, 2 down, 3 left.
    // Find: inner slots top 0, right 1, bottom 3, left 2 (d ^ (d >> 1), self-inverse);
    // outer slots 4..7 clockwise from top right.
    function innerSlot(direction: int): int {
        return direction ^ (direction >> 1)
    }

    signal quit()

    function answerDirection(direction: int, outer: bool) {
        if (tapMode) {
            controller.answer(direction)
        } else if (findMode) {
            const slot = outer ? 4 + direction : innerSlot(direction)
            if (slot < controller.pool)
                controller.answer(slot)
        } else if (direction === 1 || direction === 3) {
            controller.answer(direction === 1 ? 1 : 0)
        }
    }

    function handleGamepad(button: int) {
        if (button === Gamepad.Start || button === Gamepad.Select) {
            quit()
            return
        }
        if (!running)
            return
        // Classic/chrono: B = FALSE, A = TRUE (docs/design/product.md, "Key bindings").
        if (trueFalseMode && (button === Gamepad.B || button === Gamepad.A)) {
            controller.answer(button === Gamepad.A ? 1 : 0)
            return
        }
        // Face buttons in diamond order: Y top, B right, A bottom, X left.
        switch (button) {
        case Gamepad.Y: answerDirection(0, Gamepad.shoulderHeld); break
        case Gamepad.B: answerDirection(1, Gamepad.shoulderHeld); break
        case Gamepad.A: answerDirection(2, Gamepad.shoulderHeld); break
        case Gamepad.X: answerDirection(3, Gamepad.shoulderHeld); break
        }
    }

    focus: true
    StackView.onActivated: forceActiveFocus()

    Keys.onPressed: event => {
        AppTheme.noteKey(event.key)
        if (event.key === Qt.Key_Escape || event.key === Qt.Key_Back) {
            quit()
            event.accepted = true
            return
        }
        event.accepted = true
        if (!running)
            return
        const outer = (event.modifiers & Qt.ShiftModifier) || Gamepad.shoulderHeld
        switch (event.key) {
        case Qt.Key_Up: case Qt.Key_W: answerDirection(0, outer); return
        case Qt.Key_Right: case Qt.Key_D: answerDirection(1, outer); return
        case Qt.Key_Down: case Qt.Key_S: answerDirection(2, outer); return
        case Qt.Key_Left: case Qt.Key_A: answerDirection(3, outer); return
        case Qt.Key_F:
            if (trueFalseMode)
                controller.answer(0)
            return
        case Qt.Key_J:
            if (trueFalseMode)
                controller.answer(1)
            return
        }
        const digit = event.key - Qt.Key_1
        if (findMode && digit >= 0 && digit < controller.pool)
            controller.answer(digit)
    }

    Connections {
        target: Gamepad
        enabled: page.StackView.status === StackView.Active

        function onPressed(button: int) {
            page.handleGamepad(button)
        }
    }

    Connections {
        target: page.controller

        function onChallengeChanged() {
            if (!page.running)
                return
            if (page.controller.mode === GameController.Chrono) {
                if (page.controller.n === 1)
                    timeBar.restart(page.controller.limitMs)
            } else if (page.tapMode) {
                timeBar.restart(page.controller.limitMs)
            } else {
                ring.restart(page.controller.limitMs)
            }
        }
        function onAnswered(correct: bool) {
            flash.color = correct ? AppTheme.good : AppTheme.bad
            flashAnim.restart()
            if (!correct && AppSettings.vibration)
                Haptics.vibrate(80)
        }
        function onPhaseChanged() {
            if (page.controller.phase === GameController.Over && AppSettings.vibration)
                Haptics.vibrate(120)
            if (!page.running) {
                ring.stop()
                timeBar.stop()
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: AppTheme.background
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

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            ToolButton {
                text: "×"
                font.pixelSize: 36
                focusPolicy: Qt.NoFocus
                onClicked: page.quit()
            }
            Item { Layout.fillWidth: true }
            Label {
                text: qsTr("#%1").arg(page.controller.n)
                color: AppTheme.textMuted
                font.pixelSize: 18
            }
            Label {
                text: page.controller.score
                color: AppTheme.text
                font.pixelSize: 32
                font.bold: true
            }
        }

        // Multiplayer: every other player in this game, with item and score, ✗ once out.
        Flow {
            Layout.fillWidth: true
            visible: Multiplayer.active && Multiplayer.opponents.length > 0
            spacing: 16

            Repeater {
                model: Multiplayer.active ? Multiplayer.opponents : []

                delegate: Label {
                    required property var modelData

                    text: modelData.name + "  <b>" + modelData.score + "</b>  "
                          + (modelData.over ? "×" : "#" + modelData.n)
                    textFormat: Text.StyledText
                    color: AppTheme.text
                    opacity: modelData.over ? 0.6 : 1
                    font.pixelSize: 15
                }
            }
        }

        Bar {
            id: timeBar

            Layout.fillWidth: true
            visible: page.controller.mode === GameController.Chrono || page.tapMode
        }

        // Countdown 3-2-1-GO
        Label {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: page.controller.phase === GameController.Countdown
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: page.controller.countdown > 0 ? page.controller.countdown : qsTr("GO")
            font.pixelSize: 120
            font.bold: true
            color: AppTheme.text
        }

        GridLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: page.running
            columns: page.landscape ? 2 : 1
            rowSpacing: 16
            columnSpacing: 24

            // Word modes: the word inside the ring timer.
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 1
                Layout.preferredHeight: 1
                visible: !page.tapMode

                RingTimer {
                    id: ring

                    anchors.centerIn: parent
                    width: Math.min(parent.width, parent.height)
                    height: width
                    visible: page.controller.mode !== GameController.Chrono
                    color: AppTheme.ink(page.controller.ink)
                    strokeWidth: width * Math.max(0.02, 0.07 * (1 - page.controller.n / 300))
                }

                Label {
                    anchors.centerIn: parent
                    width: ring.width * 0.8
                    horizontalAlignment: Text.AlignHCenter
                    text: AppTheme.word(page.controller.word)
                    color: AppTheme.ink(page.controller.ink)
                    font.bold: true
                    font.pixelSize: Math.min(64, ring.width / 5)
                    fontSizeMode: Text.HorizontalFit
                    minimumPixelSize: 12
                }
            }

            // Classic / chrono: FALSE left, TRUE right.
            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: page.landscape
                Layout.preferredWidth: 1
                Layout.preferredHeight: page.landscape ? -1 : 110
                visible: page.trueFalseMode
                spacing: 16

                AnswerButton {
                    Layout.fillHeight: true
                    text: qsTr("FALSE")
                    hint: "← F"
                    fill: AppTheme.bad
                    onClicked: page.controller.answer(0)
                }
                AnswerButton {
                    Layout.fillHeight: true
                    text: qsTr("TRUE")
                    hint: "J →"
                    fill: AppTheme.good
                    onClicked: page.controller.answer(1)
                }
            }

            // Find: colour buttons at fixed diamond/ring positions.
            DiamondGrid {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 1
                Layout.preferredHeight: 1
                visible: page.findMode
                // Cell (row-major 3x3) -> colour slot; -1 = empty.
                slots: [7, 0, 4, 2, -1, 1, 6, 3, 5]

                delegate: AnswerButton {
                    required property int slot

                    readonly property string arrows: "↑→↓←"

                    anchors.fill: parent
                    visible: slot < page.controller.pool
                    fill: AppTheme.ink(slot)
                    hint: (slot + 1) + "  " + (slot < 4 ? arrows.charAt(page.innerSlot(slot))
                                                        : "⇧" + arrows.charAt(slot - 4))
                    onClicked: page.controller.answer(slot)
                }
            }

            // Tap: four cards as a diamond (top, right, bottom, left).
            DiamondGrid {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.columnSpan: page.landscape ? 2 : 1
                visible: page.tapMode
                slots: [-1, 0, -1, 3, -1, 1, -1, 2, -1]

                delegate: Button {
                    id: card

                    required property int slot

                    readonly property list<string> hints: ["↑ W", "→ D", "↓ S", "← A"]

                    anchors.fill: parent
                    focusPolicy: Qt.NoFocus
                    Material.roundedScale: Material.MediumScale
                    Material.background: AppTheme.surface
                    onClicked: page.controller.answer(slot)

                    contentItem: Item {
                        Label {
                            anchors.fill: parent
                            text: AppTheme.word(page.controller.cardWords[card.slot] ?? 0)
                            color: AppTheme.ink(page.controller.cardInks[card.slot] ?? 0)
                            font.bold: true
                            font.pixelSize: 30
                            fontSizeMode: Text.HorizontalFit
                            minimumPixelSize: 10
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        Label {
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            visible: AppTheme.keyHints
                            text: card.hints[card.slot]
                            color: AppTheme.textMuted
                            font.pixelSize: 11
                        }
                    }
                }
            }
        }
    }

    component AnswerButton: Button {
        id: answerButton

        property color fill
        property string hint

        Layout.fillWidth: true
        Layout.fillHeight: true
        // Material insets the background by 6 px; fill the whole cell as the web app does.
        topInset: 0
        bottomInset: 0
        focusPolicy: Qt.NoFocus
        Material.roundedScale: Material.MediumScale
        Material.background: fill
        Material.foreground: AppTheme.background
        font.pixelSize: 24
        font.bold: true

        Label {
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 10
            visible: AppTheme.keyHints && answerButton.hint !== ""
            text: answerButton.hint
            color: AppTheme.background
            opacity: 0.7
            font.pixelSize: 12
        }
    }

    // 3x3 grid, square, centred; `slots` maps each cell to a slot (-1 = empty).
    component DiamondGrid: Item {
        id: grid

        property list<int> slots
        property Component delegate

        GridLayout {
            anchors.centerIn: parent
            width: Math.min(grid.width, grid.height)
            height: width
            columns: 3
            rowSpacing: 10
            columnSpacing: 10

            Repeater {
                model: grid.slots

                delegate: Item {
                    id: cell

                    required property int modelData

                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredWidth: 1
                    Layout.preferredHeight: 1
                    Component.onCompleted: {
                        if (modelData >= 0)
                            grid.delegate.createObject(cell, { slot: modelData })
                    }
                }
            }
        }
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
