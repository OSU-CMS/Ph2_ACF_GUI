if(NOT DEFINED ENV{OTSDAQ_CMSTRACKER_DIR})

    MESSAGE(STATUS " ")
    MESSAGE(STATUS "    ${BoldYellow}========================================================================================================${Reset}")
    MESSAGE(STATUS "    ${BoldYellow}SRC${Reset} [stand-alone]: [${BoldCyan}Ph2_ACF/src/CMakeLists.txt${Reset}]. ${BoldRed}Begin...${Reset}")
    MESSAGE(STATUS " ")

    ############
    # Includes #
    ############
    include_directories(${PROJECT_SOURCE_DIR})

    ################
    # Library dirs #
    ################
    link_directories(${PROJECT_SOURCE_DIR/lib})

    find_package(Protobuf REQUIRED)

    # Check for ZMQ installed
    if(ZMQ_FOUND)
        # Now check for UsbInstLib
        if(PH2_USBINSTLIB_FOUND)

            # Add include directoreis for ZMQ and USBINSTLIB
            include_directories(${PH2_USBINSTLIB_INCLUDE_DIRS})
            link_directories(${PH2_USBINSTLIB_LIBRARY_DIRS})
            include_directories(${ZMQ_INCLUDE_DIRS})

            # And link against the libs
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
        set(LIBS ${LIBS} ${PH2_ANTENNA_LIBRARIES} usb)
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} $ENV{AntennaFlag}")
    endif()

    # Check for PowerSupply
    if(${PH2_POWERSUPPLY_FOUND})
      include_directories(${PH2_POWERSUPPLY_INCLUDE_DIRS})
      link_directories(${PH2_POWERSUPPLY_LIBRARY_DIRS})
      #set(LIBS ${LIBS} ${PH2_POWERSUPPLY_LIBRARIES})
      set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} $ENV{PowerSupplyFlag}")
    endif()

    # Check for TestCard USBDriver
    if(${PH2_TCUSB_FOUND})
      include_directories(${PH2_TCUSB_INCLUDE_DIRS})
      link_directories(${PH2_TCUSB_LIBRARY_DIRS})
      set(LIBS ${LIBS} ${PH2_TCUSB_LIBRARIES} usb)
      set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} $ENV{TCUSBFlag}")
    endif(${PH2_TCUSB_FOUND})

    # Boost also needs to be linked
    # include_directories(${Boost_INCLUDE_DIRS})
    # link_directories(${Boost_LIBRARY_DIRS})
    set(LIBS ${LIBS} ${Boost_DATE_TIME_LIBRARY} ${Boost_IOSTREAMS_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_PROGRAM_OPTIONS_LIBRARY_RELEASE})

    #################################
    # Find ROOT and link against it #
    #################################
    if(${ROOT_FOUND})
        include_directories(${ROOT_INCLUDE_DIRS})
        set(LIBS ${LIBS} ${ROOT_LIBRARIES})
        if(NoDataShipping)
            set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} $ENV{UseRootFlag}")
        endif()

        # Check for THttpServer
        if(${ROOT_HAS_HTTP})
            set(LIBS ${LIBS} ${ROOT_RHTTP_LIBRARY})
            set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} $ENV{HttpFlag}")
        endif()
    endif()

    # Check if the setup is based on the multiplexing backplane
    if(DEFINED ENV{MultiplexingFlag})
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} $ENV{MultiplexingFlag}")
    endif(DEFINED ENV{MultiplexingFlag})

    ########################
    # Build eudaq producer #
    ########################
    if(${USE_EUDAQ})
        set(LIBS ${LIBS} ${EUDAQ_LIB})
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} $ENV{EuDaqFlag}")
    endif(${USE_EUDAQ})

    ###############
    # EXECUTABLES #
    ###############

    file(GLOB BINARIES RELATIVE ${PROJECT_SOURCE_DIR}/src *.cc)

    # Initial set of libraries
    set(LIBS ${LIBS} Ph2_Description Ph2_Utils Ph2_Parser Ph2_RootUtils Ph2_DQMUtils Ph2_MonitorDQM NetworkUtils MessageUtils CACTUS::uhal)

    message("--     ${BoldCyan}#### Building the following executables: ####${Reset}")

    # Remove binaries
    if(CompileForShep AND CompileForHerd)
        list(REMOVE_ITEM BINARIES *.cc)
        file(GLOB BINARIES RELATIVE ${PROJECT_SOURCE_DIR}/src supervisor.cc CMSITminiDAQ.cc fpgaconfig.cc testShep.cc)
        set(LIBS ${LIBS} Ph2_Interface Ph2_System Ph2_Tools Ph2_MonitorUtils Ph2_ProductionToolsIT Ph2_miniDAQ)
    elseif(CompileForShep)
        list(REMOVE_ITEM BINARIES *.cc)
        file(GLOB BINARIES RELATIVE ${PROJECT_SOURCE_DIR}/src testShep.cc)
        set(LIBS ${LIBS} Ph2_Parser boost_regex)
    else()
        list(REMOVE_ITEM BINARIES supervisor.cc testShep.cc)
        set(LIBS ${LIBS} Ph2_Interface Ph2_System Ph2_Tools Ph2_MonitorUtils Ph2_ProductionToolsIT Ph2_miniDAQ)
    endif()

    foreach(sourcefile ${BINARIES})
        string(REPLACE ".cc" "" name ${sourcefile})
        message(STATUS "    ${name}")
        add_executable(${name} ${sourcefile})
        if (("${name}" STREQUAL "miniDAQ") OR ("${name}" STREQUAL "miniSLinkDQM"))
          target_link_libraries(${name} ${LIBS} Ph2_RootUtils RootWeb)
        else()
          target_link_libraries(${name} ${LIBS})
        endif()

        # Check for PowerSupply
        if ("${name}" STREQUAL "SEHTest")
            if(${PH2_POWERSUPPLY_FOUND})
                target_link_libraries(${name} ${LIBS} "${PH2_POWERSUPPLY_LIBRARIES}")
            endif()
          endif()

        if ("${name}" STREQUAL "powersupply")
            if(${PH2_POWERSUPPLY_FOUND})
                target_link_libraries(${name} ${LIBS} "${PH2_POWERSUPPLY_LIBRARIES}")
            endif()
        endif()

        if ("${name}" STREQUAL "testShep")
            if(${PH2_POWERSUPPLY_FOUND})
               target_link_libraries(${name} ${LIBS} protobuf::libprotobuf)
            endif()
        endif()
    endforeach(sourcefile ${BINARIES})

    message("--     ${BoldCyan}#### End ####${Reset}")

    MESSAGE(STATUS " ")
    MESSAGE(STATUS "    ${BoldYellow}SRC${Reset} [stand-alone]: [${BoldCyan}Ph2_ACF/src/CMakeLists.txt${Reset}]. ${BoldGreen}DONE!${Reset}")
    MESSAGE(STATUS "    ${BoldBlue}========================================================================================================${Reset}")
    MESSAGE(STATUS " ")

else()

    MESSAGE(STATUS " ")
    MESSAGE(STATUS "    ${BoldBlue}========================================================================================================${Reset}")
    MESSAGE(STATUS "    ${BoldYellow}SRC${Reset} [otsdaq]: [${BoldCyan}Ph2_ACF/src/CMakeLists.txt${Reset}]. ${BoldRed}Begin...${Reset}")
    MESSAGE(STATUS " ")

    #Include dirs
    include_directories(${Protobuf_INCLUDE_DIRS})

    #Library dirs
    link_directories(${Protobuf_LIBRARIES})

    cet_make_exec(NAME fpgaconfig
        SOURCE
        fpgaconfig.cc
        LIBRARIES
        Ph2_Utils
        NetworkUtils
        Ph2_MessageUtils
        protobuf
        Boost::iostreams
        Boost::filesystem
        Boost::regex
        Boost::program_options
        Boost::headers
        CACTUS::uhal
        Ph2_System
        Ph2_Interface
        Ph2_Tools
        Ph2_MonitorUtils
        Ph2_MiniDAQ
    )

    MESSAGE(STATUS " ")
    MESSAGE(STATUS "    ${BoldYellow}SRC${Reset} [otsdaq]: [${BoldCyan}Ph2_ACF/src/CMakeLists.txt${Reset}]. ${BoldGreen}DONE!${Reset}")
    MESSAGE(STATUS "    ${BoldBlue}========================================================================================================${Reset}")
    MESSAGE(STATUS " ")

endif()
