if(NOT DEFINED ENV{OTSDAQ_CMSTRACKER_DIR})

    MESSAGE(STATUS " ")
    MESSAGE(STATUS "    ${BoldYellow}========================================================================================================${Reset}")
    MESSAGE(STATUS "    ${BoldYellow}PRODUCTION TOOLS IT${Reset} [stand-alone]: [${BoldCyan}Ph2_ACF/ProductionToolsIT/CMakeLists.txt${Reset}]. ${BoldRed}Begin...${Reset}")
    MESSAGE(STATUS " ")

    # Includes
    include_directories(${PROJECT_SOURCE_DIR})

    # Boost also needs to be linked
    #include_directories(${Boost_INCLUDE_DIRS})
    #link_directories(${Boost_LIBRARY_DIRS})
    set(LIBS ${LIBS} ${Boost_DATE_TIME_LIBRARY} ${Boost_IOSTREAMS_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_PROGRAM_OPTIONS_LIBRARY_RELEASE})

    # Library dirs
    # none
    link_directories(${PROJECT_SOURCE_DIR/lib})

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

    # Power supplt
    if(${PH2_POWERSUPPLY_FOUND})
      include_directories(${PH2_POWERSUPPLY_INCLUDE_DIRS})
      link_directories(${PH2_POWERSUPPLY_LIBRARY_DIRS})
      set(LIBS ${LIBS} ${PH2_POWERSUPPLY_LIBRARIES})
      set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} $ENV{PowerSupplyFlag}")
    endif()

    # Check for TestCard USBDriver
    if(${PH2_TCUSB_FOUND})
        include_directories(${PH2_TCUSB_INCLUDE_DIRS})
        link_directories(${PH2_TCUSB_LIBRARY_DIRS})
        set(LIBS ${LIBS} usb ${PH2_TCUSB_LIBRARIES})
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} $ENV{TCUSBFlag}")
    endif(${PH2_TCUSB_FOUND})

    #last but not least, find root and link against it
    if(${ROOT_FOUND})
        include_directories(${ROOT_INCLUDE_DIRS})
        set(LIBS ${LIBS} ${ROOT_LIBRARIES})
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} $ENV{UseRootFlag}")

        #check for THttpServer
        if(${ROOT_HAS_HTTP})
            set(LIBS ${LIBS} ${ROOT_RHTTP_LIBRARY})
            set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} $ENV{HttpFlag}")
        endif()
    endif()

    set(LIBS ${LIBS} CACTUS::uhal)

    file(GLOB BINARIES RELATIVE ${PROJECT_SOURCE_DIR}/ProductionToolsIT TEPXTools.cc)

    ####################################
    ## EXECUTABLES
    ####################################

    message("--     ${BoldCyan}#### Building the following executables: ####${Reset}")

    # Add the library
    add_library(Ph2_ProductionToolsIT STATIC ${SOURCES} ${HEADERS})

    foreach(sourcefile ${BINARIES})
        string(REPLACE ".cc" "" name ${sourcefile})
        message(STATUS "    ${name}")
        add_executable(${name} ${sourcefile})
        target_link_libraries(${name} ${LIBS} Ph2_ProductionToolsIT Ph2_Interface Ph2_System Ph2_Tools Ph2_Utils Ph2_Parser)
    endforeach(sourcefile ${BINARIES})

    include_directories(${CACTUS_INCLUDEDIR})
    target_link_libraries(Ph2_ProductionToolsIT ${LIBS})

    message("--     ${BoldCyan}#### End ####${Reset}")

    MESSAGE(STATUS " ")
    MESSAGE(STATUS "    ${BoldYellow}PRODUCTION TOOLS IT${Reset} [stand-alone]: [${BoldCyan}Ph2_ACF/ProductionToolsIT/CMakeLists.txt${Reset}]. ${BoldGreen}DONE!${Reset}")
    MESSAGE(STATUS "    ${BoldBlue}========================================================================================================${Reset}")
    MESSAGE(STATUS " ")

else()

    MESSAGE(STATUS " ")
    MESSAGE(STATUS "    ${BoldBlue}========================================================================================================${Reset}")
    MESSAGE(STATUS "    ${BoldYellow}PRODUCTION TOOLS IT${Reset} [otsdaq]: [${BoldCyan}Ph2_ACF/ProductionToolsIT/CMakeLists.txt${Reset}]. ${BoldRed}Begin...${Reset}")
    MESSAGE(STATUS " ")

    include_directories(${Protobuf_INCLUDE_DIRS})

#    include_directories(${CMAKE_CURRENT_SOURCE_DIR})
#    include_directories($ENV{PH2ACF_BASE_DIR}/HWDescription)
#    include_directories($ENV{PH2ACF_BASE_DIR}/HWInterface)
#    include_directories($ENV{PH2ACF_BASE_DIR}/Utils)
#    include_directories($ENV{PH2ACF_BASE_DIR}/tools)
#    include_directories($ENV{PH2ACF_BASE_DIR}/ProductionToolsIT)
#    include_directories($ENV{PH2ACF_BASE_DIR}/DQMUtils)
#    include_directories($ENV{PH2ACF_BASE_DIR}/System)

    cet_set_compiler_flags(
        EXTRA_FLAGS -Wno-reorder -Wl,--undefined
    )

    cet_make(LIBRARY_NAME Ph2_ProductionToolsIT_${Ph2_ACF_Master}
            LIBRARIES
            pthread
            ${Boost_SYSTEM_LIBRARY}
            )

    install_headers()
    install_source()

    MESSAGE(STATUS " ")
    MESSAGE(STATUS "    ${BoldYellow}PRODUCTION TOOLS IT${Reset} [otsdaq]: [${BoldCyan}Ph2_ACF/ProductionToolsIT/CMakeLists.txt${Reset}]. ${BoldGreen}DONE!${Reset}")
    MESSAGE(STATUS "    ${BoldBlue}========================================================================================================${Reset}")
    MESSAGE(STATUS " ")

endif()
