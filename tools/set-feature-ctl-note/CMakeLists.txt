# Built for the build host (see ../CMakeLists.txt), not the CHERI target.
cmake_minimum_required(VERSION 3.20)
project(set-feature-ctl-note CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(set-feature-ctl-note set-feature-ctl-note.cpp elf_defs.h)

enable_testing()
add_executable(test_set_feature_ctl_note test_set_feature_ctl_note.cpp elf_defs.h)
add_test(NAME set-feature-ctl-note
    COMMAND test_set_feature_ctl_note $<TARGET_FILE:set-feature-ctl-note>)
