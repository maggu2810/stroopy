// SPDX-License-Identifier: GPL-3.0-or-later
pragma Singleton
import QtQuick

// Presentation of colour slots (docs/spec/rules/v1.md, appendix A) and app colours.
QtObject {
    readonly property bool colourBlind: AppSettings.colourBlind

    readonly property list<color> normalInks: ["#f1c40f", "#e74c3c", "#3498db", "#2ecc71",
                                               "#e67e22", "#9b59b6", "#ff89e5", "#a67235"]
    readonly property list<color> colourBlindInks: ["#ffd71d", "#f95341", "#0a88dc", "#18df6c",
                                                    "#04e5e3", "#cfb891", "#8b0eaa", "#7d4f1b"]
    readonly property list<string> normalWords: [qsTr("YELLOW"), qsTr("RED"), qsTr("BLUE"), qsTr("GREEN"),
                                                 qsTr("ORANGE"), qsTr("PURPLE"), qsTr("PINK"), qsTr("BROWN")]
    readonly property list<string> colourBlindWords: [qsTr("YELLOW"), qsTr("RED"), qsTr("BLUE"), qsTr("GREEN"),
                                                      qsTr("CYAN"), qsTr("BEIGE"), qsTr("PURPLE"), qsTr("BROWN")]

    readonly property color background: "#1d2330"
    readonly property color surface: "#2a3242"
    readonly property color text: "#f2f4f8"
    readonly property color textMuted: "#9aa4b5"
    readonly property color accent: "#f1c40f"
    readonly property color good: "#2ecc71"
    readonly property color bad: "#e74c3c"

    function modeName(mode: int): string {
        switch (mode) {
        case GameController.Classic: return qsTr("Classic")
        case GameController.Chrono: return qsTr("Chrono")
        case GameController.Find: return qsTr("Find the colour")
        case GameController.Tap: return qsTr("Tap the true one")
        }
        return qsTr("Unknown mode %1").arg(mode)
    }
    function ink(slot: int): color {
        return (colourBlind ? colourBlindInks : normalInks)[slot]
    }
    function word(slot: int): string {
        return (colourBlind ? colourBlindWords : normalWords)[slot]
    }
}
