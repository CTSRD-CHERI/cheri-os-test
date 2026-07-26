# Finds libxo (https://github.com/Juniper/libxo). Respects
# CMAKE_FIND_ROOT_PATH like any other find_path/find_library call, so this
# picks up the version installed into the target sysroot when cross-compiling.

find_path(LibXo_INCLUDE_DIR NAMES libxo/xo.h)
find_library(LibXo_LIBRARY NAMES xo)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibXo REQUIRED_VARS LibXo_LIBRARY LibXo_INCLUDE_DIR)

if(LibXo_FOUND AND NOT TARGET LibXo::xo)
    get_filename_component(LibXo_LIBRARY_DIR "${LibXo_LIBRARY}" DIRECTORY)
    # Link by  name rather than the detected absolute path: we need this to be
    # usable for both static/dynamic linking to select libxo.a/libxo.so.
    add_library(LibXo::xo INTERFACE IMPORTED)
    set_target_properties(LibXo::xo PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${LibXo_INCLUDE_DIR}"
        INTERFACE_LINK_DIRECTORIES "${LibXo_LIBRARY_DIR}"
        INTERFACE_LINK_LIBRARIES "xo")
endif()

mark_as_advanced(LibXo_INCLUDE_DIR LibXo_LIBRARY)
