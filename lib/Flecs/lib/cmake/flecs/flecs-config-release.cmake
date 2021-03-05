#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "flecs::flecs" for configuration "Release"
set_property(TARGET flecs::flecs APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(flecs::flecs PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/flecs.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/flecs.dll"
  )

list(APPEND _IMPORT_CHECK_TARGETS flecs::flecs )
list(APPEND _IMPORT_CHECK_FILES_FOR_flecs::flecs "${_IMPORT_PREFIX}/lib/flecs.lib" "${_IMPORT_PREFIX}/bin/flecs.dll" )

# Import target "flecs::flecs_static" for configuration "Release"
set_property(TARGET flecs::flecs_static APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(flecs::flecs_static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/flecs_static.lib"
  )

list(APPEND _IMPORT_CHECK_TARGETS flecs::flecs_static )
list(APPEND _IMPORT_CHECK_FILES_FOR_flecs::flecs_static "${_IMPORT_PREFIX}/lib/flecs_static.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
