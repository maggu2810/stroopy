# SPDX-License-Identifier: GPL-3.0-or-later
find_package(Qt6 REQUIRED COMPONENTS Test)

qt_add_executable(tst_rules_v1 tst_rules_v1.cpp)
target_link_libraries(tst_rules_v1 PRIVATE Qt6::Test stroopy_core)
target_compile_definitions(tst_rules_v1 PRIVATE
    STROOPY_VECTORS_DIR="${STROOPY_SPEC_DIR}/test-vectors"
)
add_test(NAME rules_v1_conformance COMMAND tst_rules_v1)

qt_add_executable(tst_game_v1 tst_game_v1.cpp)
target_link_libraries(tst_game_v1 PRIVATE Qt6::Test stroopy_core)
target_compile_definitions(tst_game_v1 PRIVATE
    STROOPY_VECTORS_DIR="${STROOPY_SPEC_DIR}/test-vectors"
)
add_test(NAME game_v1_conformance COMMAND tst_game_v1)

qt_add_executable(tst_backup_v1 tst_backup_v1.cpp)
target_link_libraries(tst_backup_v1 PRIVATE Qt6::Test stroopy_core)
target_compile_definitions(tst_backup_v1 PRIVATE
    STROOPY_VECTORS_DIR="${STROOPY_SPEC_DIR}/test-vectors"
)
add_test(NAME backup_v1_conformance COMMAND tst_backup_v1)

qt_add_executable(tst_multiplayer_v1 tst_multiplayer_v1.cpp)
target_link_libraries(tst_multiplayer_v1 PRIVATE Qt6::Test stroopy_net)
target_compile_definitions(tst_multiplayer_v1 PRIVATE
    STROOPY_VECTORS_DIR="${STROOPY_SPEC_DIR}/test-vectors"
)
add_test(NAME multiplayer_v1_conformance COMMAND tst_multiplayer_v1)

find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2)
qt_add_executable(tst_ui tst_ui.cpp)
target_link_libraries(tst_ui PRIVATE Qt6::Test Qt6::Quick Qt6::QuickControls2 stroopy_ui stroopy_uiplugin)
add_test(NAME ui_keyboard COMMAND tst_ui)
set_tests_properties(ui_keyboard PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Live Nostr interop driver (needs internet), used by apps/web/tests/e2e/interop-qt.ts; not a ctest.
qt_add_executable(interop_nostr interop_nostr.cpp)
target_link_libraries(interop_nostr PRIVATE Qt6::Gui Qt6::Qml stroopy_ui stroopy_uiplugin)
