// SPDX-License-Identifier: GPL-3.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

// In-app documentation (docs/design/product.md, "Help"): modes, controls, sharing, about.
SubPage {
    id: page

    title: qsTr("Help")

    ScrollView {
        id: scroll

        anchors.fill: parent
        contentWidth: availableWidth
        focus: true
        StackView.onActivated: scroll.forceActiveFocus()

        // Let arrow keys scroll the help text.
        Keys.onUpPressed: ScrollBar.vertical.decrease()
        Keys.onDownPressed: ScrollBar.vertical.increase()

        ColumnLayout {
            x: (scroll.availableWidth - width) / 2
            width: Math.min(scroll.availableWidth - 32, 560)
            spacing: 8

            Heading { text: qsTr("The idea") }
            Paragraph {
                text: qsTr("A colour word is shown in an ink colour. Your brain reads the word faster than it "
                           + "names the ink – that conflict is the Stroop effect. Be quick and precise: the "
                           + "game gets faster, and from item 20 on more colours join in.")
            }

            Heading { text: qsTr("Modes") }
            Entry {
                term: AppTheme.modeName(GameController.Classic)
                definition: qsTr("Does the ink match the word? Answer TRUE or FALSE before the ring runs out. "
                                 + "One mistake ends the game.")
            }
            Entry {
                term: AppTheme.modeName(GameController.Chrono)
                definition: qsTr("TRUE or FALSE for 30 seconds. +1 for a right answer, −1 for a wrong one.")
            }
            Entry {
                term: AppTheme.modeName(GameController.Find)
                definition: qsTr("The ink never matches. Tap the colour that the word names, not the ink. "
                                 + "One mistake ends the game.")
            }
            Entry {
                term: AppTheme.modeName(GameController.Tap)
                definition: qsTr("Four cards, exactly one of them shows its word in the matching ink. Find it "
                                 + "before the bar runs out.")
            }

            Heading { text: qsTr("Keyboard") }
            Entry { keys: true; term: "← / F    → / J"; definition: qsTr("FALSE / TRUE (Classic, Chrono)") }
            Entry { keys: true; term: "↑ → ↓ ←   W D S A"; definition: qsTr("Card at that position (Tap)") }
            Entry { keys: true; term: "↑ → ↓ ←"; definition: qsTr("Inner colours: yellow, red, green, blue (Find)") }
            Entry { keys: true; term: "⇧ + ↑ → ↓ ←"; definition: qsTr("Outer colours, clockwise from top right (Find)") }
            Entry { keys: true; term: "1 – 8"; definition: qsTr("Colour by its number on the button (Find)") }
            Entry { keys: true; term: qsTr("Arrows, Tab"); definition: qsTr("Move between buttons in menus") }
            Entry { keys: true; term: qsTr("Enter, Space"); definition: qsTr("Press the selected button") }
            Entry { keys: true; term: "Esc"; definition: qsTr("Back; quit a running game") }
            Entry { keys: true; term: "R / N"; definition: qsTr("Replay / new game on the result screen") }
            Entry { keys: true; term: "F11"; definition: qsTr("Fullscreen") }

            Heading { text: qsTr("Gamepad") }
            Entry { keys: true; term: "B / A"; definition: qsTr("FALSE / TRUE (Classic, Chrono)") }
            Entry { keys: true; term: qsTr("Y B A X or D-pad"); definition: qsTr("Top, right, bottom, left (Tap, Find)") }
            Entry { keys: true; term: qsTr("L1/R1 + direction"); definition: qsTr("Outer colours (Find)") }
            Entry { keys: true; term: qsTr("A / B in menus"); definition: qsTr("Press / back") }
            Entry { keys: true; term: qsTr("Start, Select"); definition: qsTr("Quit a running game") }

            Heading { text: qsTr("TV remote") }
            Paragraph {
                text: qsTr("Use the D-pad like the arrow keys, OK to press and Back to go back. In Find, colours "
                           + "beyond the first four need a keyboard, gamepad or touch.")
            }

            Heading { text: qsTr("Sharing and challenges") }
            Paragraph {
                text: qsTr("Every game is a sequence that can be replayed exactly. After a game, or for any "
                           + "highscore, copy the code (for example SYG1-…) and send it to a friend. They paste it "
                           + "under “Enter code” – even a whole message works – and play the very same sequence "
                           + "against your score.")
            }

            Heading { text: qsTr("Multiplayer") }
            Paragraph {
                text: qsTr("Create a room and send its invitation (SYM1-…, as a link or message) to your "
                           + "friends; they join under “Multiplayer”. The host picks the mode and starts, then "
                           + "everyone plays the same sequence at the same time and sees the others’ progress. "
                           + "Rooms run over public Nostr relays chosen by whoever creates the room: no account, "
                           + "no server of ours, but names and scores are not encrypted.")
            }

            Heading { text: qsTr("About") }
            Entry { term: qsTr("Version"); definition: Qt.application.version }
            Entry { term: qsTr("Compatibility"); definition: qsTr("rules 1 · game 1 · multiplayer 1 · backup 1") }
            Paragraph {
                Layout.bottomMargin: 24
                text: qsTr("Stroopy is free software under the GNU General Public License, version 3 or "
                           + "later. It contains no ads and no tracking, and it works without Google services.")
            }
        }
    }

    component Heading: Label {
        Layout.fillWidth: true
        Layout.topMargin: 20
        font.pixelSize: 20
        font.bold: true
        color: AppTheme.accent
    }

    component Paragraph: Label {
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        color: AppTheme.text
        font.pixelSize: 16
    }

    // Term and definition side by side; stacked on narrow screens, as in the web app.
    component Entry: GridLayout {
        id: entry

        property string term
        property string definition
        // Key names: monospace, as in the web app.
        property bool keys: false
        readonly property bool narrow: page.width <= 480

        Layout.fillWidth: true
        Layout.bottomMargin: narrow ? 8 : 0
        columns: narrow ? 1 : 2
        columnSpacing: 16
        rowSpacing: 2

        Label {
            Layout.fillWidth: entry.narrow
            Layout.preferredWidth: entry.narrow ? -1 : Math.min(200, page.width * 0.38)
            Layout.alignment: Qt.AlignTop
            text: entry.term
            wrapMode: Text.Wrap
            font.family: entry.keys ? "monospace" : Qt.application.font.family
            font.weight: entry.keys ? Font.DemiBold : Font.Bold
            font.pixelSize: 16
            color: AppTheme.text
        }
        Label {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop
            text: entry.definition
            wrapMode: Text.Wrap
            font.pixelSize: 16
            color: AppTheme.textMuted
        }
    }
}
