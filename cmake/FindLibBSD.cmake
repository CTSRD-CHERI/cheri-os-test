# Finds libbsd (https://libbsd.freedesktop.org/), which provides BSD
# compatibility headers/functions on non-BSD systems.

find_path(LibBSD_INCLUDE_DIR NAMES bsd/bsd.h)
find_library(LibBSD_LIBRARY NAMES bsd)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibBSD REQUIRED_VARS LibBSD_LIBRARY LibBSD_INCLUDE_DIR)

if(LibBSD_FOUND AND NOT TARGET LibBSD::bsd)
    get_filename_component(LibBSD_LIBRARY_DIR "${LibBSD_LIBRARY}" DIRECTORY)
    # Link by  name rather than the detected absolute path: we need this to be
    # usable for both static/dynamic linking to select libbsd.a/libbsd.so.
    add_library(LibBSD::bsd INTERFACE IMPORTED)
    set_target_properties(LibBSD::bsd PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${LibBSD_INCLUDE_DIR}"
        INTERFACE_LINK_DIRECTORIES "${LibBSD_LIBRARY_DIR}"
        INTERFACE_LINK_LIBRARIES "bsd")
endif()

mark_as_advanced(LibBSD_INCLUDE_DIR LibBSD_LIBRARY)
