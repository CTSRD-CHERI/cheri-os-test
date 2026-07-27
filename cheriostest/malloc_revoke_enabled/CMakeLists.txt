# malloc_revoke_enabled: a statically-linked test binary, installed 6 times
# with different combinations of the CHERI revocation ELF feature-control
# note (unset/force-disabled/force-enabled) and setuid bit.
# FreeBSD/CheriBSD only: malloc revocation is not supported on Linux, and ELF
# feature-control notes are a FreeBSD-specific mechanism.
#
# NOTE: the setuid variants are installed with the SETUID permission bit set,
# so the test will not work on 9pfs

add_executable(malloc_revoke_enabled malloc_revoke_enabled.c)
target_link_options(malloc_revoke_enabled PRIVATE -static)
set_target_properties(malloc_revoke_enabled PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CHERIOSTEST_LIBEXEC_OUTPUT_DIRECTORY}")
install(TARGETS malloc_revoke_enabled RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR})

# The elfnote_* variants need the ELF feature-control. elfctl(1) does that,
# but it is only part of the CheriBSD build so not always available.
# Since we are just flipping a bit in the note header, we build our own
# minimal tool for the build host instead.
include(ExternalProject)
ExternalProject_Add(host-set-feature-ctl-note
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/tools/set-feature-ctl-note"
    BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/set-feature-ctl-note-build"
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
    INSTALL_COMMAND ""
    BUILD_BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/set-feature-ctl-note-build/set-feature-ctl-note"
)
set(ELFCTL_EXECUTABLE "${CMAKE_CURRENT_BINARY_DIR}/set-feature-ctl-note-build/set-feature-ctl-note")

function(add_malloc_revoke_enabled_variant name features)
    # `out` is set to where the cheriostest binaries that exec() these look for them.
    set(out "${CHERIOSTEST_LIBEXEC_OUTPUT_DIRECTORY}/${name}")
    if(features STREQUAL "")
        add_custom_command(OUTPUT "${out}"
            COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:malloc_revoke_enabled>" "${out}"
            DEPENDS malloc_revoke_enabled
            VERBATIM)
    else()
        add_custom_command(OUTPUT "${out}"
            COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:malloc_revoke_enabled>" "${out}"
            COMMAND ${ELFCTL_EXECUTABLE} "${features}" "${out}"
            DEPENDS malloc_revoke_enabled ${ELFCTL_EXECUTABLE}
            VERBATIM)
    endif()
    add_custom_target(malloc_revoke_enabled-${name} ALL DEPENDS "${out}")
    add_dependencies(malloc_revoke_enabled-${name} host-set-feature-ctl-note)

    if(name MATCHES "_suid")
        install(PROGRAMS "${out}" DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}
            PERMISSIONS
                OWNER_READ OWNER_WRITE OWNER_EXECUTE
                GROUP_READ GROUP_EXECUTE
                WORLD_READ WORLD_EXECUTE
                SETUID)
    else()
        install(PROGRAMS "${out}" DESTINATION ${CMAKE_INSTALL_LIBEXECDIR})
    endif()
endfunction()

add_malloc_revoke_enabled_variant(malloc_revoke_enabled_elfnote_disable "+nocherirevoke")
add_malloc_revoke_enabled_variant(malloc_revoke_enabled_elfnote_enable "+cherirevoke")
add_malloc_revoke_enabled_variant(malloc_revoke_enabled_suid "")
add_malloc_revoke_enabled_variant(malloc_revoke_enabled_suid_elfnote_disable "+nocherirevoke")
add_malloc_revoke_enabled_variant(malloc_revoke_enabled_suid_elfnote_enable "+cherirevoke")
