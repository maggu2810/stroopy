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
            width: Math.min(scroll.availableWidth - 48, 720)
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
            Entry { term: "← / F    → / J"; definition: qsTr("FALSE / TRUE (Classic, Chrono)") }
            Entry { term: "↑ → ↓ ←   W D S A"; definition: qsTr("Card at that position (Tap)") }
            Entry { term: "↑ → ↓ ←"; definition: qsTr("Inner colours: yellow, red, green, blue (Find)") }
            Entry { term: "⇧ + ↑ → ↓ ←"; definition: qsTr("Outer colours, clockwise from top right (Find)") }
            Entry { term: "1 – 8"; definition: qsTr("Colour by its number on the button (Find)") }
            Entry { term: qsTr("Arrows, Tab"); definition: qsTr("Move between buttons in menus") }
            Entry { term: qsTr("Enter, Space"); definition: qsTr("Press the selected button") }
            Entry { term: "Esc"; definition: qsTr("Back; quit a running game") }
            Entry { term: "R / N"; definition: qsTr("Replay / new game on the result screen") }
            Entry { term: "F11"; definition: qsTr("Fullscreen") }

            Heading { text: qsTr("Gamepad") }
            Entry { term: "B / A"; definition: qsTr("FALSE / TRUE (Classic, Chrono)") }
            Entry { term: qsTr("Y B A X or D-pad"); definition: qsTr("Top, right, bottom, left (Tap, Find)") }
            Entry { term: qsTr("L1/R1 + direction"); definition: qsTr("Outer colours (Find)") }
            Entry { term: qsTr("A / B in menus"); definition: qsTr("Press / back") }
            Entry { term: qsTr("Start, Select"); definition: qsTr("Quit a running game") }

            Heading { text: qsTr("TV remote") }
            Paragraph {
                text: qsTr("Use the D-pad like the arrow keys, OK to press and Back to go back. In Find, colours "
                           + "beyond the first four need a keyboard, gamepad or touch.")
            }

            Heading { text: qsTr("Sharing and challenges") }
            Paragraph {
                text: qsTr("Every game is a sequence that can be replayed exactly. After a game, or for any "
                           + "highscore, copy the code (for example SY1-…) and send it to a friend. They paste it "
                           + "under “Enter code” – even a whole message works – and play the very same sequence "
                           + "against your score.")
            }

            Heading { text: qsTr("Multiplayer") }
            Paragraph {
                text: qsTr("Create a room and send its code (SYR1-…) to your friends; they join under "
                           + "“Multiplayer”. The host picks the mode and starts, then everyone plays the same "
                           + "sequence at the same time and sees the others’ progress. Rooms run over public "
                           + "Nostr relays: no account, no server of ours, but names and scores are not encrypted.")
            }

            Heading { text: qsTr("About") }
            Entry { term: qsTr("Version"); definition: Qt.application.version }
            Entry { term: qsTr("Compatibility"); definition: qsTr("rules 1 · share code 1 · backup 1 · protocol 1") }
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
        font.pixelSize: 22
        font.bold: true
        color: AppTheme.accent
    }

    component Paragraph: Label {
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        color: AppTheme.text
        font.pixelSize: 16
    }

    component Entry: RowLayout {
        id: entry

        property string term
        property string definition

        Layout.fillWidth: true
        spacing: 16

        Label {
            Layout.preferredWidth: Math.min(200, page.width * 0.38)
            Layout.alignment: Qt.AlignTop
            text: entry.term
            wrapMode: Text.Wrap
            font.bold: true
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
