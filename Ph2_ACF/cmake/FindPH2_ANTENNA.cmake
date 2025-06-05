file(GLOB_RECURSE PH2_ANTENNA_SOURCE_DIR ${PROJECT_SOURCE_DIR}/../CMSPh2_AntennaDriver/*Antenna.h)
if(PH2_ANTENNA_SOURCE_DIR)
         set(PH2_ANTENNA_FOUND TRUE)
         #strip to the blank path
         get_filename_component(PH2_ANTENNA_SOURCE_DIR "${PH2_ANTENNA_SOURCE_DIR}" PATH)

         find_library(PH2_ANTENNA_LIBRARY_DIRS
         NAMES
           lib/libPh2_Antenna.so
         PATHS
         ${PH2_ANTENNA_SOURCE_DIR}
         )

         #strip away the path
         get_filename_component(PH2_ANTENNA_LIBRARY_DIRS "${PH2_ANTENNA_LIBRARY_DIRS}" PATH)
         #strip again to get the path to the Ph2_USBInstDriver directory from the root of the FS
         get_filename_component(PH2_ANTENNA_SOURCE_DIR "${PH2_ANTENNA_LIBRARY_DIRS}" PATH)

         set(PH2_ANTENNA_INCLUDE_DIRS
             ${PH2_ANTENNA_SOURCE_DIR})

         file(GLOB_RECURSE PH2_ANTENNA_LIBRARIES ${PH2_ANTENNA_LIBRARY_DIRS}/*.so)
else(PH2_ANTENNA_SOURCE_DIR)
         set(PH2_ANTENNA_FOUND FALSE)
endif(PH2_ANTENNA_SOURCE_DIR)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(PH2_ANTENNA DEFAULT_MSG PH2_ANTENNA_SOURCE_DIR)

  # show the PH2_USBINSTLIB_INCLUDE_DIRS and PH2_USBINSTLIB_LIBRARIES variables only in the advanced view
  mark_as_advanced(PH2_ANTENNA_INCLUDE_DIRS PH2_ANTENNA_LIBRARY_DIRS)
