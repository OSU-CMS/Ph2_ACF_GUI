# - Try to find the CAENHVWrapper library
# Once done this will define
#
#  CAENHVWRAPPER_FOUND - System has CAENHVWrapper
#  CAENHVWRAPPER_INCLUDE_DIR - The CAENHVWrapper include directory
#  CAENHVWRAPPER_LIBRARY - The libraries needed to use CAENHVWrapper

# use pkg-config to get the directories and then use these values
# in the find_path() and find_library() calls
find_package(PkgConfig QUIET)
PKG_CHECK_MODULES(PC_CAENHVWRAPPER QUIET libcaenhvwrapper)

find_path(CAENHVWRAPPER_INCLUDE_DIR NAMES CAENHVWrapper.h)

find_library(CAENHVWRAPPER_LIBRARY NAMES caenhvwrapper libcaenhvwrapper)

# handle the QUIETLY and REQUIRED arguments and set CAENHVWRAPPER_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(CAENHVWrapper
                                  REQUIRED_VARS CAENHVWRAPPER_LIBRARY CAENHVWRAPPER_INCLUDE_DIR
                                  )

mark_as_advanced(CAENHVWRAPPER_INCLUDE_DIR CAENHVWRAPPER_LIBRARY)
