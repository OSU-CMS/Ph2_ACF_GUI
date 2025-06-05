if(NOT DEFINED ENV{OTSDAQ_CMSTRACKER_DIR})

    MESSAGE(STATUS " ")
    MESSAGE(STATUS "    ${BoldBlue}========================================================================================================${Reset}")
    MESSAGE(STATUS "    ${BoldYellow}HW INTERFACE${Reset} [stand-alone]: [${BoldCyan}Ph2_ACF/HWInterface/CMakeLists.txt${Reset}]. ${BoldRed}Begin...${Reset}")
    MESSAGE(STATUS " ")

    ############
    # Includes #
    ############
    include_directories(${PROJECT_SOURCE_DIR})

    # Boost also needs to be linked
    # include_directories(${Boost_INCLUDE_DIRS})
    # link_directories(${Boost_LIBRARY_DIRS})
    set(LIBS ${LIBS} ${Boost_ITERATOR_LIBRARY} ${Boost_IOSTREAMS_LIBRARY})

    # Find root and link against it
    if(${ROOT_FOUND})
        include_directories(${ROOT_INCLUDE_DIRS})
        set(LIBS ${LIBS} ${ROOT_LIBRARIES})
    endif()

    # Find source files
    file(GLOB HEADERS *.h)
    file(GLOB SOURCES *.cc)

    # Add the library
    add_library(Ph2_Interface STATIC ${SOURCES} ${HEADERS})

    set(LIBS ${LIBS} Ph2_Description NetworkUtils CACTUS::uhal CACTUS::uhal_log boost_regex)

    # Check for TestCard USBDriver
    TARGET_LINK_LIBRARIES(Ph2_Interface ${LIBS})
    #check for TestCard USBDriver
    if(${PH2_TCUSB_FOUND})
      include_directories(${PH2_TCUSB_INCLUDE_DIRS})
      link_directories(${PH2_TCUSB_LIBRARY_DIRS})
      set(LIBS ${LIBS} ${PH2_TCUSB_LIBRARIES} usb)
      set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} $ENV{TCUSBFlag}")
    endif(${PH2_TCUSB_FOUND})

    #################
    ## EXECUTABLES ##
    #################

    file(GLOB BINARIES RELATIVE ${PROJECT_SOURCE_DIR}/HWInterface *.cc)

    message("--     ${BoldCyan}#### Building the following executables: ####${Reset}")
    foreach( sourcefile ${BINARIES} )
        string(REPLACE ".cc" "" name ${sourcefile})
        message(STATUS "    ${name}")
    endforeach(sourcefile ${BINARIES})
    message("--     ${BoldCyan}#### End ####${Reset}")

    MESSAGE(STATUS " ")
    MESSAGE(STATUS "    ${BoldYellow}HW INTERFACE${Reset} [stand-alone]: [${BoldCyan}Ph2_ACF/HWInterface/CMakeLists.txt${Reset}]. ${BoldGreen}DONE!${Reset}")
    MESSAGE(STATUS "    ${BoldBlue}========================================================================================================${Reset}")
    MESSAGE(STATUS " ")

else()

    MESSAGE(STATUS " ")
    MESSAGE(STATUS "    ${BoldBlue}========================================================================================================${Reset}")
    MESSAGE(STATUS "    ${BoldYellow}HW INTERFACE${Reset} [otsdaq]: [${BoldCyan}Ph2_ACF/HWInterface/CMakeLists.txt${Reset}]. ${BoldRed}Begin...${Reset}")
    MESSAGE(STATUS " ")

    cet_make(LIBRARY_NAME Ph2_Interface
            LIBRARIES
            Ph2_Description
    )

    install_headers()
    install_source()

    MESSAGE(STATUS " ")
    MESSAGE(STATUS "    ${BoldYellow}HW INTERFACE${Reset} [otsdaq]: [${BoldCyan}Ph2_ACF/HWInterface/CMakeLists.txt${Reset}]. ${BoldGreen}DONE!${Reset}")
    MESSAGE(STATUS "    ${BoldBlue}========================================================================================================${Reset}")
    MESSAGE(STATUS " ")

endif()
