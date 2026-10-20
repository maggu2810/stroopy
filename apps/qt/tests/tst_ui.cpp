// SPDX-License-Identifier: GPL-3.0-or-later
// Headless UI test: drives the real QML app with key presses (docs/design/product.md, "Input").
// Set STROOPY_SCREENSHOT_DIR to also save screenshots of the visited screens.
#include "backup.hpp"
#include "gamecontroller.hpp"
#include "highscores.hpp"

#include <QDir>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QTest>
#include <QtQml/qqmlextensionplugin.h>

Q_IMPORT_QML_PLUGIN(StroopyPlugin)

using namespace Qt::StringLiterals;

class TestUi : public QObject {
    Q_OBJECT

    QQmlApplicationEngine *m_engine = nullptr;
    QQuickWindow *m_window = nullptr;
    GameController *m_controller = nullptr;
    QString m_shots;

    void key(Qt::Key k, Qt::KeyboardModifiers modifiers = Qt::NoModifier)
    {
        QTest::keyClick(m_window, k, modifiers);
    }

    void shot(const QString &name)
    {
        QTest::qWait(400); // let transitions and animations settle
        if (!m_shots.isEmpty())
            QVERIFY(m_window->grabWindow().save(QDir{m_shots}.filePath(name + u".png"_s)));
    }

    void resize(int w, int h)
    {
        m_window->resize(w, h);
        QTest::qWait(100);
    }

    // Home has focus on the first mode button; Down moves along the focus chain.
    void startModeFromHome(int index)
    {
        QTest::qWait(600); // let the page transition finish so focus is on the first mode button
        for (int i = 0; i < index; ++i)
            key(Qt::Key_Down);
        key(Qt::Key_Return);
        QTRY_COMPARE_WITH_TIMEOUT(m_controller->phase(), GameController::Phase::Running, 6000);
    }

    static Qt::Key directionKey(int direction)
    {
        static constexpr Qt::Key keys[]{Qt::Key_Up, Qt::Key_Right, Qt::Key_Down, Qt::Key_Left};
        return keys[direction];
    }

private slots:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        QDir{QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)}.removeRecursively();
        QDir{QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)}.removeRecursively();
        m_shots = qEnvironmentVariable("STROOPY_SCREENSHOT_DIR");
        if (!m_shots.isEmpty())
            QDir{}.mkpath(m_shots);

        QQuickStyle::setStyle(u"Material"_s);
        m_engine = new QQmlApplicationEngine{this};
        m_engine->loadFromModule(u"Stroopy"_s, u"Main"_s);
        QVERIFY(!m_engine->rootObjects().isEmpty());
        m_window = qobject_cast<QQuickWindow *>(m_engine->rootObjects().first());
        QVERIFY(m_window);
        m_controller = m_window->findChild<GameController *>();
        QVERIFY(m_controller);
        resize(420, 820);
        QVERIFY(QTest::qWaitForWindowExposed(m_window));
    }

    void homeScreens()
    {
        shot(u"home-portrait"_s);
        resize(1280, 720);
        shot(u"home-landscape"_s);
    }

    void classicByKeyboard()
    {
        resize(420, 820);
        startModeFromHome(0);
        QCOMPARE(m_controller->mode(), GameController::Mode::Classic);
        shot(u"classic-portrait"_s);
        for (int i = 0; i < 3; ++i) {
            const bool isTrue = m_controller->ink() == m_controller->word();
            key(isTrue ? Qt::Key_J : Qt::Key_Left);
            QCOMPARE(m_controller->score(), i + 1);
        }
        resize(1280, 720);
        shot(u"classic-landscape"_s);

        // A wrong answer ends the game and shows the result page; N starts a new one.
        const bool isTrue = m_controller->ink() == m_controller->word();
        key(isTrue ? Qt::Key_Left : Qt::Key_Right);
        QCOMPARE(m_controller->phase(), GameController::Phase::Over);
        QCOMPARE(m_controller->score(), 3);
        shot(u"result-landscape"_s);
        resize(420, 820);
        shot(u"result-portrait"_s);
        key(Qt::Key_N);
        QCOMPARE(m_controller->phase(), GameController::Phase::Countdown);
        key(Qt::Key_Escape);
        QCOMPARE(m_controller->phase(), GameController::Phase::Idle);
    }

    void findByKeyboard()
    {
        resize(1280, 720);
        startModeFromHome(2);
        QCOMPARE(m_controller->mode(), GameController::Mode::Find);
        shot(u"find-landscape"_s);
        for (int i = 0; i < 3; ++i) {
            const int slot = m_controller->word(); // pool is 4 at the start: inner diamond
            key(directionKey(slot ^ (slot >> 1)));
            QCOMPARE(m_controller->score(), i + 1);
        }
        // Number keys select by slot as well.
        key(static_cast<Qt::Key>(Qt::Key_1 + m_controller->word()));
        QCOMPARE(m_controller->score(), 4);
        resize(420, 820);
        shot(u"find-portrait"_s);
        key(Qt::Key_Escape);
    }

    void tapByKeyboard()
    {
        resize(420, 820);
        startModeFromHome(3);
        QCOMPARE(m_controller->mode(), GameController::Mode::Tap);
        shot(u"tap-portrait"_s);
        for (int i = 0; i < 3; ++i) {
            const auto inks = m_controller->cardInks();
            const auto words = m_controller->cardWords();
            int trueCard = -1;
            for (int c = 0; c < 4; ++c) {
                if (inks[c] == words[c])
                    trueCard = c;
            }
            QVERIFY(trueCard >= 0);
            key(directionKey(trueCard));
            QCOMPARE(m_controller->score(), i + 1);
        }
        resize(1280, 720);
        shot(u"tap-landscape"_s);
        key(Qt::Key_Escape);
    }

    void helpPage()
    {
        // Focus chain on home: 4 modes, then Highscores, Enter code, Help, Settings.
        resize(420, 820);
        QTest::qWait(600);
        for (int i = 0; i < 6; ++i)
            key(Qt::Key_Down);
        key(Qt::Key_Return);
        shot(u"help-portrait"_s);
        resize(1280, 720);
        shot(u"help-landscape"_s);
        key(Qt::Key_Escape);
        shot(u"home-after-help"_s);
    }

    void backupRoundTrip()
    {
        // Home: 7 x Down reaches Settings; there, Tab x 5 reaches the backup button.
        resize(420, 820);
        QTest::qWait(600);
        for (int i = 0; i < 7; ++i)
            key(Qt::Key_Down);
        key(Qt::Key_Return);
        QTest::qWait(600);
        shot(u"settings-portrait"_s);
        for (int i = 0; i < 5; ++i)
            key(Qt::Key_Tab);
        key(Qt::Key_Return);
        shot(u"backup-portrait"_s);
        resize(1280, 720);
        shot(u"backup-landscape"_s);

        auto *highscores = m_engine->singletonInstance<Highscores *>("Stroopy", "Highscores");
        auto *backup = m_engine->singletonInstance<Backup *>("Stroopy", "Backup");
        QVERIFY(highscores && backup);
        const auto before = highscores->all().size();
        QVERIFY(before > 0); // the classic game above scored 3
        const auto code = backup->exportCode();
        QVERIFY(code.startsWith(u"SYB1-"_s));
        QCOMPARE(backup->importCode(code, true), 0); // everything already there
        highscores->clear();
        QCOMPARE(Backup::inspect(u"Here it is:\n"_s + code)[u"entries"_s].toInt(), before);
        QCOMPARE(backup->importCode(code, true), before);
        QCOMPARE(highscores->all().size(), before);
        QCOMPARE(backup->exportCode(), code);
        QCOMPARE(backup->importCode(u"SYB1-XXXX"_s, true), -1);
        key(Qt::Key_Escape);
        key(Qt::Key_Escape);
    }
};

QTEST_MAIN(TestUi)
#include "tst_ui.moc"
