// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Shapes

// Circular timer that drains from full to empty over `duration` ms.
Item {
    id: ring

    property color color: "white"
    property real strokeWidth: 16
    property real progress: 1.0

    function restart(duration: int) {
        drain.stop()
        progress = 1.0
        drain.duration = duration
        drain.start()
    }
    function stop() {
        drain.stop()
    }

    NumberAnimation {
        id: drain

        target: ring
        property: "progress"
        from: 1.0
        to: 0.0
    }

    Shape {
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            strokeColor: Qt.alpha(ring.color, 0.15)
            strokeWidth: ring.strokeWidth
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap

            PathAngleArc {
                centerX: ring.width / 2
                centerY: ring.height / 2
                radiusX: (Math.min(ring.width, ring.height) - ring.strokeWidth) / 2
                radiusY: radiusX
                startAngle: 0
                sweepAngle: 360
            }
        }

        ShapePath {
            strokeColor: ring.color
            strokeWidth: ring.strokeWidth
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap

            PathAngleArc {
                centerX: ring.width / 2
                centerY: ring.height / 2
                radiusX: (Math.min(ring.width, ring.height) - ring.strokeWidth) / 2
                radiusY: radiusX
                startAngle: -90
                sweepAngle: 360 * ring.progress
            }
        }
    }
}
