if(NOT DEFINED ENV{OTSDAQ_CMSTRACKER_DIR})

    MESSAGE(STATUS " ")
    MESSAGE(STATUS "    ${BoldYellow}========================================================================================================${Reset}")
    MESSAGE(STATUS "    ${BoldYellow}PYTHON UTILS${Reset} [stand-alone]: [${BoldCyan}Ph2_ACF/pythonUtils/CMakeLists.txt${Reset}]. ${BoldRed}Begin...${Reset}")
    MESSAGE(STATUS " ")

    # pybind11
    if (NOT DEFINED ENV{PYBIND11}) # No external PYBIND11
        message(STATUS "    No pybind11 found. Installing.")
        message(STATUS "    Obtaining from github ref v$ENV{PYBIND11_REF}")

        FetchContent_Declare(ext_pybind                          # name of the content
        GIT_REPOSITORY https://github.com/pybind/pybind11.git  # the repository
        GIT_TAG        v$ENV{PYBIND11_REF}
        )
        FetchContent_MakeAvailable(ext_pybind)

        set(ENV{PYBIND11} ${CMAKE_BINARY_DIR}/_deps/ext_pybind-src/ )
        set(ENV{PYBIND11INCLUDE} $ENV{PYBIND11}/include/ )
    endif(NOT DEFINED ENV{PYBIND11})

    # Includes
    include_directories(${PROJECT_SOURCE_DIR}/MessageUtils/cpp)
    include_directories(${PROJECT_SOURCE_DIR}/miniDAQ)
    include_directories(${PROJECT_SOURCE_DIR}/Utils)
    include_directories($ENV{PYBIND11INCLUDE})
    include_directories($ENV{PYTHONINCLUDE})
    include_directories(${PROJECT_SOURCE_DIR})
    include_directories(${CMAKE_CURRENT_SOURCE_DIR})

    # Library dirs
    link_directories(${PROJECT_SOURCE_DIR/lib})

    # Initial set of libraries
    set(LIBS ${LIBS} Ph2_Description Ph2_Interface Ph2_Utils Ph2_System )

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

    # Find root and link against it
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

    #    find_package(Protobuf REQUIRED)

    # Boost also needs to be linked
    # include_directories(${Boost_INCLUDE_DIRS})
    # link_directories(${Boost_LIBRARY_DIRS})

    set(LIBS ${LIBS} boost_serialization ${Boost_IOSTREAMS_LIBRARY} ${Boost_DATE_TIME_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_PROGRAM_OPTIONS_LIBRARY}) # ${Protobuf_LIBRARIES})

    add_subdirectory($ENV{PYBIND11} subproject/pybind11)

    pybind11_add_module(Ph2_ACF_PythonInterface PythonMiddlewareMessageHandler.cc)
    target_link_libraries(Ph2_ACF_PythonInterface PRIVATE ${LIBS} Ph2_Tools Ph2_Parser Ph2_System Ph2_miniDAQ MessageUtils CACTUS::uhal)

    ###############
    # EXECUTABLES #
    ###############

    MESSAGE(STATUS " ")
    MESSAGE(STATUS "    ${BoldYellow}PYTHON UTILS${Reset} [stand-alone]: [${BoldCyan}Ph2_ACF/pythonUtils/CMakeLists.txt${Reset}]. ${BoldGreen}DONE!${Reset}")
    MESSAGE(STATUS "    ${BoldBlue}========================================================================================================${Reset}")
    MESSAGE(STATUS " ")

else() # ------------------------------- Compilation in the otsdaq environment ---------------------------------------------

    MESSAGE(STATUS " ")
    MESSAGE(STATUS "    ${BoldYellow}========================================================================================================${Reset}")
    MESSAGE(STATUS "    ${BoldYellow}PYTHON UTILS${Reset} [otsdaq]: [${BoldCyan}Ph2_ACF/pythonUtils/CMakeLists.txt${Reset}]. ${BoldRed}Begin...${Reset}")
    MESSAGE(STATUS " ")

    MESSAGE(STATUS " ")
    MESSAGE(STATUS "    ${BoldYellow}PYTHON UTILS${Reset} [otsdaq]: [${BoldCyan}Ph2_ACF/pythonUtils/CMakeLists.txt${Reset}]. ${BoldGreen}DONE!${Reset}")
    MESSAGE(STATUS "    ${BoldBlue}========================================================================================================${Reset}")
    MESSAGE(STATUS " ")

endif()
