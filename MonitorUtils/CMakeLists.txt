if(NOT DEFINED ENV{OTSDAQ_CMSTRACKER_DIR})

    MESSAGE(STATUS " ")
    MESSAGE(STATUS "    ${BoldYellow}========================================================================================================${Reset}")
    MESSAGE(STATUS "    ${BoldYellow}MONITOR UTILS${Reset} [stand-alone]: [${BoldCyan}Ph2_ACF/MonitorUtils/CMakeLists.txt${Reset}]. ${BoldRed}Begin...${Reset}")
    MESSAGE(STATUS " ")

    # Includes
    include_directories(${PROJECT_SOURCE_DIR})

    # Boost also needs to be linked
    # include_directories(${Boost_INCLUDE_DIRS})
    # link_directories(${Boost_LIBRARY_DIRS})
    set(LIBS ${LIBS} ${Boost_ITERATOR_LIBRARY})

    # Library dirs
    # none

    # Find source files
    file(GLOB HEADERS *.h)
    file(GLOB SOURCES *.cc)

    # Check for ZMQ installed
    if(ZMQ_FOUND)
        #here, now check for UsbInstLib
        if(PH2_USBINSTLIB_FOUND)

            #add include directoreis for ZMQ and USBINSTLIB
            include_directories(${PH2_USBINSTLIB_INCLUDE_DIRS})
            link_directories(${PH2_USBINSTLIB_LIBRARY_DIRS})
            include_directories(${ZMQ_INCLUDE_DIRS})

            #and link against the libs
            set(LIBS ${LIBS} ${ZMQ_LIBRARIES} ${PH2_USBINSTLIB_LIBRARIES})
            set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} $ENV{ZmqFlag} $ENV{USBINSTFlag}")
        endif()
    endif()

    # Check for AMC13 libraries
    if(${CACTUS_AMC13_FOUND})
        include_directories(${PROJECT_SOURCE_DIR}/AMC13)
        include_directories(${UHAL_AMC13_INCLUDE_PREFIX})
        link_directories(${UHAL_AMC13_LIB_PREFIX})
        set(LIBS ${LIBS} cactus_amc13_amc13 Ph2_Amc13)
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} $ENV{Amc13Flag}")
    endif()

    # Check for AntennaDriver
    if(${PH2_ANTENNA_FOUND})
        include_directories(${PH2_ANTENNA_INCLUDE_DIRS})
        link_directories(${PH2_ANTENNA_LIBRARY_DIRS})
        set(LIBS ${LIBS} usb ${PH2_ANTENNA_LIBRARIES})
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} $ENV{AntennaFlag}")
    endif()

    # Check for TestCard USBDriver
    if(${PH2_TCUSB_FOUND})
        include_directories(${PH2_TCUSB_INCLUDE_DIRS})
        link_directories(${PH2_TCUSB_LIBRARY_DIRS})
        set(LIBS ${LIBS} usb ${PH2_TCUSB_LIBRARIES})
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} $ENV{TCUSBFlag}")
    endif(${PH2_TCUSB_FOUND})
    
    #last but not least, find root and link against it
    if(NoDataShipping)
        if(${ROOT_FOUND})
            include_directories(${ROOT_INCLUDE_DIRS})
            set(LIBS ${LIBS} ${ROOT_LIBRARIES})
            if(NoDataShipping)
                set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} $ENV{UseRootFlag}")
            endif()

            #check for THttpServer
            if(${ROOT_HAS_HTTP})
                set(LIBS ${LIBS} ${ROOT_RHTTP_LIBRARY})
                set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} $ENV{HttpFlag}")
            endif()
        endif()
    endif()

    set(LIBS ${LIBS} Ph2_Description Ph2_Interface Ph2_Utils)
    if(NoDataShipping)
        set(LIBS ${LIBS} Ph2_MonitorDQM)
    endif()

    file(GLOB BINARIES RELATIVE ${PROJECT_SOURCE_DIR}/tools *.cc)

    # Build eudaq producer
    if(${USE_EUDAQ})
        set(LIBS ${LIBS} ${EUDAQ_LIB})
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} $ENV{EuDaqFlag}")
    endif(${USE_EUDAQ})

    ###############
    # EXECUTABLES #
    ###############

    message("--     ${BoldCyan}#### Building the following executables: ####${Reset}")

    # Add the library
    add_library(Ph2_MonitorUtils STATIC ${SOURCES} ${HEADERS})

    foreach(sourcefile ${BINARIES})
        string(REPLACE ".cc" "" name ${sourcefile})
        message(STATUS "    ${name}")
    endforeach(sourcefile ${BINARIES})

    TARGET_LINK_LIBRARIES(Ph2_MonitorUtils ${LIBS})

    message("--     ${BoldCyan}#### End ####${Reset}")

    MESSAGE(STATUS " ")
    MESSAGE(STATUS "    ${BoldYellow}MONITOR UTILS${Reset} [stand-alone]: [${BoldCyan}Ph2_ACF/MonitorUtils/CMakeLists.txt${Reset}]. ${BoldGreen}DONE!${Reset}")
    MESSAGE(STATUS "    ${BoldBlue}========================================================================================================${Reset}")
    MESSAGE(STATUS " ")

else()

    MESSAGE(STATUS " ")
    MESSAGE(STATUS "    ${BoldBlue}========================================================================================================${Reset}")
    MESSAGE(STATUS "    ${BoldYellow}MONITOR UTILS${Reset} [otsdaq]: [${BoldCyan}Ph2_ACF/MonitorUtils/CMakeLists.txt${Reset}]. ${BoldRed}Begin...${Reset}")
    MESSAGE(STATUS " ")

    cet_make(LIBRARY_NAME Ph2_MonitorUtils
             LIBRARIES
             pthread
             ${Boost_SYSTEM_LIBRARY}
    )

    install_headers()
    install_source()

    MESSAGE(STATUS " ")
    MESSAGE(STATUS "    ${BoldYellow}MONITOR UTILS${Reset} [otsdaq]: [${BoldCyan}Ph2_ACF/MonitorUtils/CMakeLists.txt${Reset}]. ${BoldGreen}DONE!${Reset}")
    MESSAGE(STATUS "    ${BoldBlue}========================================================================================================${Reset}")
    MESSAGE(STATUS " ")

endif()
