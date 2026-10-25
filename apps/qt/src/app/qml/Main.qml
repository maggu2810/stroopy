// SPDX-License-Identifier: GPL-3.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Material
import QtQuick.Templates as T

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
    function showLobby() {
        stack.replace(null, [homePage, lobbyPage])
    }
    // A stroopy:// link leaves the current game and room, then opens its page prefilled.
    function openLink(page: Component, text: string) {
        if (Multiplayer.active && (gameController.phase === GameController.Countdown
                                   || gameController.phase === GameController.Running))
            Multiplayer.report(gameController.phase === GameController.Running ? gameController.n : 1,
                               gameController.score, true)
        gameController.abort()
        Multiplayer.leave()
        stack.replace(null, [homePage, page, {initialText: text}])
    }
    function moveFocus(forward: bool) {
        const current = window.activeFocusItem
        const next = current ? current.nextItemInFocusChain(forward) : null
        if (next)
            next.forceActiveFocus(forward ? Qt.TabFocusReason : Qt.BacktabFocusReason)
    }
    function activateFocused() {
        const item = window.activeFocusItem as T.AbstractButton
        if (item && item.enabled)
            item.click()
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
            Multiplayer.setBusy(phase === GameController.Countdown || phase === GameController.Running)
            if (phase === GameController.Running || phase === GameController.Over)
                Multiplayer.report(n, score, phase === GameController.Over)
            if (phase !== GameController.Over)
                return
            window.lastRank = Highscores.submit(rules, mode, seedHex, score, reached, isChallenge)
            // Multiplayer: the lobby shows everybody's result.
            if (Multiplayer.active)
                window.showLobby()
            else
                stack.replace(null, resultPage)
        }
        onScoreChanged: if (phase === GameController.Running) Multiplayer.report(n, score, false)
        onChallengeChanged: if (phase === GameController.Running) Multiplayer.report(n, score, false)
    }

    Connections {
        target: Multiplayer

        function onPlay(mode: int, seedHex: string) {
            if (gameController.start(mode, seedHex))
                stack.replace(null, [homePage, lobbyPage, gamePage])
        }
        function onOpened() {
            window.showLobby()
        }
    }

    Connections {
        target: Links

        function onPlay(code: string) {
            window.openLink(enterCodePage, code)
        }
        function onJoin(invitation: string) {
            window.openLink(multiplayerPage, invitation)
        }
    }

    Shortcut {
        sequence: "F11"
        onActivated: window.visibility = window.visibility === Window.FullScreen ? Window.Windowed
                                                                                 : Window.FullScreen
    }

    // Keys not handled by a page: menu navigation (docs/design/product.md, "Key bindings").
    // Pages handle Esc/Back and gamepad B themselves; an unhandled Back on the home page
    // lets Android close the app.
    StackView {
        id: stack

        objectName: "stack"
        anchors.fill: parent
        focus: true
        initialItem: homePage
        // Hand keyboard focus to the new page right away; pages focus a specific control on activation.
        onCurrentItemChanged: currentItem?.forceActiveFocus()

        Keys.onPressed: event => {
            AppTheme.keyHints = true
            switch (event.key) {
            case Qt.Key_Up:
            case Qt.Key_Left:
                window.moveFocus(false)
                break
            case Qt.Key_Down:
            case Qt.Key_Right:
                window.moveFocus(true)
                break
            case Qt.Key_Return:
            case Qt.Key_Enter:
            case Qt.Key_Select:
                window.activateFocused()
                break
            default:
                event.accepted = false
                return
            }
            event.accepted = true
        }
    }

    Connections {
        target: Gamepad

        function onPressed(button: int) {
            AppTheme.keyHints = true
            if (button === Gamepad.A)
                window.activateFocused()
        }
    }

    // Focus frame for keyboard, gamepad and TV remote navigation.
    Rectangle {
        readonly property Item target: {
            const item = window.activeFocusItem as T.Control
            return AppTheme.keyHints && item && item.visualFocus ? item : null
        }
        // Also depends on window size and page transitions, so it follows layout changes.
        readonly property rect geometry: target && window.width > 0 && window.height > 0 && !stack.busy
                                         ? target.mapToItem(parent, 0, 0, target.width, target.height)
                                         : Qt.rect(0, 0, 0, 0)

        parent: Overlay.overlay
        visible: target !== null
        x: geometry.x - 4
        y: geometry.y - 4
        width: geometry.width + 8
        height: geometry.height + 8
        radius: 10
        color: "transparent"
        border.width: 3
        border.color: AppTheme.text
    }

    Component {
        id: homePage

        HomePage {
            onPlay: mode => window.playGame(mode, "")
            onOpenMultiplayer: stack.push(multiplayerPage)
            onOpenHighscores: stack.push(highscoresPage)
            onOpenEnterCode: stack.push(enterCodePage)
            onOpenHelp: stack.push(helpPage)
            onOpenSettings: stack.push(settingsPage)
        }
    }

    Component {
        id: gamePage

        GamePage {
            controller: gameController
            onQuit: {
                // Multiplayer: quitting counts as being out at the current item; stay in the room.
                if (Multiplayer.active) {
                    const running = gameController.phase === GameController.Running
                    Multiplayer.report(running ? gameController.n : 1, gameController.score, true)
                    gameController.abort()
                    window.showLobby()
                    return
                }
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
        id: helpPage

        HelpPage {
            onBack: stack.pop()
        }
    }

    Component {
        id: settingsPage

        SettingsPage {
            onBack: stack.pop()
            onOpenBackup: stack.push(backupPage)
        }
    }

    Component {
        id: multiplayerPage

        MultiplayerPage {
            onBack: stack.pop()
        }
    }

    Component {
        id: lobbyPage

        LobbyPage {
            onBack: {
                Multiplayer.leave()
                window.goHome()
            }
        }
    }

    Component {
        id: backupPage

        BackupPage {
            onBack: stack.pop()
        }
    }
}
