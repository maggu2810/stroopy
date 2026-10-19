// SPDX-License-Identifier: GPL-3.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Material

ApplicationWindow {
    id: window

    width: 420
    height: 820
    visible: true
    title: qsTr("Stroopy")
    color: AppTheme.background

    Material.theme: Material.Dark
    Material.accent: AppTheme.accent

    GameController {
        id: gameController

        onPhaseChanged: {
            if (phase === GameController.Over)
                stack.replace(null, resultPage)
        }
    }

    StackView {
        id: stack

        anchors.fill: parent
        initialItem: homePage
    }

    Component {
        id: homePage

        HomePage {
            onPlay: (mode, seedHex) => {
                if (gameController.start(mode, seedHex))
                    stack.replace(null, gamePage)
            }
        }
    }

    Component {
        id: gamePage

        GamePage {
            controller: gameController
            onQuit: {
                gameController.abort()
                stack.replace(null, homePage)
            }
        }
    }

    Component {
        id: resultPage

        ResultPage {
            controller: gameController
            onReplay: {
                gameController.restart()
                stack.replace(null, gamePage)
            }
            onNewGame: {
                gameController.start(gameController.mode)
                stack.replace(null, gamePage)
            }
            onHome: stack.replace(null, homePage)
        }
    }
}
