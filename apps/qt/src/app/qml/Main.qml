// SPDX-License-Identifier: GPL-3.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Material

ApplicationWindow {
    id: window

    property int lastRank: 0

    function playGame(mode: int, seedHex: string) {
        if (gameController.start(mode, seedHex))
            stack.replace(null, gamePage)
    }
    function goHome() {
        stack.replace(null, homePage)
    }

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
            if (phase !== GameController.Over)
                return
            window.lastRank = Highscores.submit(rules, mode, seedHex, score, reached, isChallenge)
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
            onPlay: mode => window.playGame(mode, "")
            onOpenHighscores: stack.push(highscoresPage)
            onOpenEnterCode: stack.push(enterCodePage)
            onOpenSettings: stack.push(settingsPage)
        }
    }

    Component {
        id: gamePage

        GamePage {
            controller: gameController
            onQuit: {
                gameController.abort()
                window.goHome()
            }
        }
    }

    Component {
        id: resultPage

        ResultPage {
            controller: gameController
            rank: window.lastRank
            onReplay: {
                gameController.restart()
                stack.replace(null, gamePage)
            }
            onNewGame: window.playGame(gameController.mode, "")
            onHome: window.goHome()
        }
    }

    Component {
        id: highscoresPage

        HighscoresPage {
            onBack: stack.pop()
            onPlayEntry: (mode, seedHex) => window.playGame(mode, seedHex)
        }
    }

    Component {
        id: enterCodePage

        EnterCodePage {
            onBack: stack.pop()
            onPlay: (mode, seedHex, score, reached) => {
                if (gameController.startChallenge(mode, seedHex, score, reached))
                    stack.replace(null, gamePage)
            }
        }
    }

    Component {
        id: settingsPage

        SettingsPage {
            onBack: stack.pop()
        }
    }
}
