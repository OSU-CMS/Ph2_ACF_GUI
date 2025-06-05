# - Try to find Ph2_UsbInstLib
# Once done this will define
#
#  PH2_USBINSTLIB_FOUND - system has Ph2 USbInstLib
#  PH2_USBINSTLIB_INCLUDE_DIRS
#  PH2_USBINSTLIB_LIBRARY_DIRS
#  PH2_USBINSTLIB_LIBRARIES 
#
#  Copyright (c) 2017 Georg Auzinger <georg.auzinger@SPAMNOTcern.ch>
#
file(GLOB_RECURSE PH2_USBINSTLIB ${PROJECT_SOURCE_DIR}/../Ph2_USBInstDriver/init_serial.sh)
if(PH2_USBINSTLIB)
      set(PH2_USBINSTLIB_FOUND TRUE)
      #strip to the blank path
      get_filename_component(PH2_USBINSTLIB_DIR "${PH2_USBINSTLIB}" PATH)

      find_library(PH2_USBINSTLIB_LIBRARY_DIRS
      NAMES
        lib/libPh2_Drivers.so
        lib/libPh2_HMP4040.so
        lib/libPh2_Ke2110.so
        lib/libPh2_ArdNano.so
        lib/libPh2_USBUtils.so
      PATHS
        ${PH2_USBINSTLIB_DIR}
        )
        #strip away the path
        get_filename_component(PH2_USBINSTLIB_LIBRARY_DIRS "${PH2_USBINSTLIB_LIBRARY_DIRS}" PATH)
        #strip again to get the path to the Ph2_USBInstDriver directory from the root of the FS
        get_filename_component(PH2_USBINSTLIB_DIR "${PH2_USBINSTLIB_LIBRARY_DIRS}" PATH)

        set(PH2_USBINSTLIB_INCLUDE_DIRS
            ${PH2_USBINSTLIB_DIR}/Utils
            ${PH2_USBINSTLIB_DIR}/Ke2110
            ${PH2_USBINSTLIB_DIR}/HMP4040
            ${PH2_USBINSTLIB_DIR}/Drivers
            ${PH2_USBINSTLIB_DIR}/ArdNano
        )

        file(GLOB_RECURSE PH2_USBINSTLIB_LIBRARIES ${PH2_USBINSTLIB_LIBRARY_DIRS}/*.so)
else(PH2_USBINSTLIB)
        set(PH2_USBINSTLIB_FOUND FALSE)
endif(PH2_USBINSTLIB)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PH2_USBInstLib DEFAULT_MSG PH2_USBINSTLIB_DIR)

# show the PH2_USBINSTLIB_INCLUDE_DIRS and PH2_USBINSTLIB_LIBRARIES variables only in the advanced view
mark_as_advanced(PH2_USBINSTLIB_INCLUDE_DIRS PH2_USBINSTLIB_LIBRARY_DIRS)

