# SPDX-License-Identifier: GPL-3.0-or-later
find_package(Qt6 REQUIRED COMPONENTS Test)

qt_add_executable(tst_rules_v1 tst_rules_v1.cpp)
target_link_libraries(tst_rules_v1 PRIVATE Qt6::Test stroopy_core)
target_compile_definitions(tst_rules_v1 PRIVATE
    STROOPY_VECTORS_DIR="${STROOPY_SPEC_DIR}/test-vectors"
)
add_test(NAME rules_v1_conformance COMMAND tst_rules_v1)
