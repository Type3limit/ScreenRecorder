#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "qrcodegencpp::qrcodegencpp" for configuration "Debug"
set_property(TARGET qrcodegencpp::qrcodegencpp APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(qrcodegencpp::qrcodegencpp PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/qrcodegencppd.lib"
  )

list(APPEND _cmake_import_check_targets qrcodegencpp::qrcodegencpp )
list(APPEND _cmake_import_check_files_for_qrcodegencpp::qrcodegencpp "${_IMPORT_PREFIX}/lib/qrcodegencppd.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
