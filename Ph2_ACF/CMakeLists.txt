string(ASCII 27 Esc)
set(Reset       "${Esc}[m"    )
set(Bold        "${Esc}[1m"   )
set(Red         "${Esc}[31m"  )
set(Green       "${Esc}[32m"  )
set(Yellow      "${Esc}[33m"  )
set(Blue        "${Esc}[34m"  )
set(Magenta     "${Esc}[35m"  )
set(Cyan        "${Esc}[36m"  )
set(White       "${Esc}[37m"  )
set(BoldRed     "${Esc}[1;31m")
set(BoldGreen   "${Esc}[1;32m")
set(BoldYellow  "${Esc}[1;33m")
set(BoldBlue    "${Esc}[1;34m")
set(BoldMagenta "${Esc}[1;35m")
set(BoldCyan    "${Esc}[1;36m")
set(BoldWhite   "${Esc}[1;37m")

include(ExternalProject)
include(FetchContent)

set(EXTERNAL_INSTALL_LOCATION ${CMAKE_BINARY_DIR}/external)

##############################
# Get the latest commit hash #
##############################
execute_process(
    COMMAND git rev-parse HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_STRIP_TRAILING_WHITESPACE)

######################################################
# Run the git command to get the current branch name #
######################################################
execute_process(
    COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(NOT DEFINED ENV{OTSDAQ_CMSTRACKER_DIR})

    MESSAGE(STATUS " ")
    MESSAGE(STATUS "${BoldBlue}========================================================================================================${Reset}")
    MESSAGE(STATUS "    ${BoldYellow}PH2_ACF${Reset} [stand-alone]: ${BoldRed}Compilation of the PH2_ACF package${Reset}")
    MESSAGE(STATUS "    ${BoldYellow}PH2_ACF${Reset} [stand-alone]: [${BoldCyan}${PH2ACF_BASE_DIR}/CMakeLists.txt${Reset}]")
    MESSAGE(STATUS " ")

    if( NOT DEFINED ENV{PH2ACF_BASE_DIR} )
       MESSAGE(FATAL_ERROR "${BoldRed}PH2ACF_BASE_DIR not set. Did you source setup.sh${Reset}")
    endif()

    cmake_minimum_required(VERSION 3.17)
    project(Ph2_ACF)

    ############################
    # Set the output directory #
    ############################
    if ( ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR} )
       MESSAGE( STATUS "${BoldRed}" )
       MESSAGE( FATAL_ERROR "${Reset}In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt." )
    endif()

    # ---------- Setup output Directories -------------------------
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY
        ${PROJECT_SOURCE_DIR}/lib
        CACHE PATH
        "Single Directory for all Libraries"
       )

    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
        ${PROJECT_SOURCE_DIR}/bin
        CACHE PATH
        "Single Directory for all Executables."
       )

    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY
        ${PROJECT_SOURCE_DIR}/bin
        CACHE PATH
        "Single Directory for all static libraries."
       )
    # ---------- Setup output Directories -------------------------

    #############################
    # Set the cmake module path #
    #############################
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

    set(CompileForShep "$ENV{CompileForShep}")
    set(CompileForHerd "$ENV{CompileForHerd}")
    set(CompileMinExecutable "$ENV{CompileMinExecutable}")
    set(EnableSanitizer "$ENV{EnableSanitizer}")
    if ((NOT CompileForShep) AND (NOT CompileForHerd))
        set(NoDataShipping true)
    else()
        set(NoDataShipping false)
    endif()

    MESSAGE(STATUS "    ${BoldYellow}COMPILATION FLAGS:${Reset}")
    MESSAGE(STATUS "    CompileForShep = ${BoldRed}${CompileForShep}${Reset}")
    MESSAGE(STATUS "    CompileForHerd = ${BoldRed}${CompileForHerd}${Reset}")
    MESSAGE(STATUS "    NoDataShipping = ${BoldRed}${NoDataShipping}${Reset}")

    set (GCC_NOERROR_FLAGS " -Wno-error=unused-result -Wno-error=deprecated-declarations -Wno-error=parentheses -Wno-error=misleading-indentation -Wno-error=unused-function -Wno-error=class-memaccess -Wno-error=implicit-fallthrough")
    set (GCC_12_NOERROR_FLAGS "-Wno-error=range-loop-construct -Wno-error=int-in-bool-context -Wno-error=nonnull")

    set(CMAKE_CXX_STANDARD $ENV{STDCXX} CACHE STRING "C++ version selection")
    set(CMAKE_CXX_STANDARD_REQUIRED ON)  # optional, ensure standard is supported
    set(CMAKE_CXX_EXTENSIONS OFF)  # optional, keep compiler extensions off

    if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
      EXECUTE_PROCESS( COMMAND gcc -dumpfullversion OUTPUT_VARIABLE GCC_VERSION )
      if(GCC_VERSION VERSION_LESS 14)
        set (GCC_NOERROR_FLAGS "${GCC_NOERROR_FLAGS} ${GCC_12_NOERROR_FLAGS}")
      endif()
    endif ()

    set (CMAKE_CXX_FLAGS "-O3 -fopenmp -fno-omit-frame-pointer -Werror -pedantic -Wno-unused-parameter -Wno-ignored-qualifiers -Wno-missing-field-initializers -Wall -Werror=return-type -Wextra -Winit-self -Wno-unused-local-typedefs -Woverloaded-virtual -Wnon-virtual-dtor -Wdelete-non-virtual-dtor ${GCC_NOERROR_FLAGS} -pthread -Wcpp -fPIC ${CMAKE_CXX_FLAGS} -DELPP_THREAD_SAFE")
    if(NoDataShipping)
      set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} $ENV{UseRootFlag}")
    endif()
    if(EnableSanitizer)
      set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --param=max-vartrack-size=150000000 -fsanitize=undefined -Wno-array-bounds ")
    endif()
    

    ##################################
    # Check for external dependences #
    ##################################
    message("--")
    message("--     ${BoldCyan}#### Checking for external Dependencies ####${Reset}")
    if(CompileForShep OR NoDataShipping)
      ########
      # ROOT #
      ########
      find_package(ROOT COMPONENTS RHTTP)
      if(ROOT_FOUND)
        # Check for THttpServer
        if(EXISTS ${ROOT_RHTTP_LIBRARY})
          message(STATUS "    Found THttp Server support - enabling compiler flags")
          set(ROOT_HAS_HTTP TRUE)
        elseif()
          message(STATUS "    ROOT built without THttp Server - disabling compiler flags")
        endif()
        message(STATUS "    Using ROOT version ${ROOT_VERSION}")
      endif(ROOT_FOUND)
    else()
      message(STATUS "Building without ROOT")
    endif()

    # AMC13
    find_package(CACTUS)
    if(${CACTUS_AMC13_FOUND})
        message(STATUS "    Enabling AMC13 component")
    endif(${CACTUS_AMC13_FOUND})

    # Antenna
    find_package(PH2_ANTENNA)
    if(PH2_ANTENNA_FOUND)
        message(STATUS "    Building the Antenna components")
    endif(PH2_ANTENNA_FOUND)

    # Ph2_TCUSB [USB control of hybrids]
    if ($ENV{CompileWithTCUSB})
        find_package(PH2_TCUSB)
        message(STATUS "    Building with TCUSB")

        if(NOT PH2_TCUSB_FOUND)
            message(WARNING "    Couldn't find PH2_TCUSB. Obtaining from gitlab ref $ENV{PH2_TCUSB_REF}")
            ExternalProject_Add( ext_tcusb
                    GIT_REPOSITORY https://gitlab.cern.ch/cms_tk_ph2/cmsph2_tcusb.git
                    GIT_TAG $ENV{PH2_TCUSB_REF}
                    PREFIX ${EXTERNAL_INSTALL_LOCATION}
                    INSTALL_COMMAND : # This will skip the install step
                )
            set(PH2_TCUSB_FOUND TRUE)
            set(PH2_TCUSB_INCLUDE_DIRS ${EXTERNAL_INSTALL_LOCATION}/src/ext_tcusb/src)
            set(PH2_TCUSB_LIBRARY_DIRS ${EXTERNAL_INSTALL_LOCATION}/src/ext_tcusb/lib)
            set(PH2_TCUSB_LIBRARIES ${EXTERNAL_INSTALL_LOCATION}/src/ext_tcusb/lib/libcmsph2_tcusb.so  ${EXTERNAL_INSTALL_LOCATION}/src/ext_tcusb/lib/libcmsph2_tcusb_utils.so)
        endif(NOT PH2_TCUSB_FOUND)
    endif($ENV{CompileWithTCUSB})

    #########
    # EUDAQ #
    #########
    if ($ENV{CompileWithEUDAQ})
        message(STATUS "Building with EUDAQ")
        set(USE_EUDAQ ON)
        if(NOT DEFINED ENV{EUDAQDIR})
            message(WARNING "Couldn't find EUDAQ. Obtaining from gitlab ref $ENV{EUDAQ_REF}")
            ExternalProject_Add( ext_eudaq
                    GIT_REPOSITORY https://gitlab.cern.ch/cms_tk_ph2/eudaq.git
                    GIT_TAG $ENV{EUDAQ_REF}
                    PREFIX ${EXTERNAL_INSTALL_LOCATION}
                    CMAKE_ARGS -DUSER_EUDET_BUILD=OFF
                               -DUSER_EXAMPLE_BUILD=OFF
                               -DEUDAQ_BUILD_EXECUTABLE=OFF
                )
            set(ENV{EUDAQDIR} ${EXTERNAL_INSTALL_LOCATION}/src/ext_eudaq/ )
        endif(NOT DEFINED ENV{EUDAQDIR})

        include_directories($ENV{EUDAQDIR}/include)
        set(EUDAQ_LIB -L$ENV{EUDAQDIR}/lib -Wl,-rpath=$ENV{EUDAQDIR}/lib eudaq_core)
        message(STATUS "EUDAQ variable is defined. EUDAQ directory: ${BoldCyan}$ENV{EUDAQDIR}${Reset}")
    else($ENV{CompileWithEUDAQ})
        message(STATUS "EUDAQ variable is not defined. Building without EUDAQ producer")
    endif($ENV{CompileWithEUDAQ})

    ##########
    # ZeroMQ #
    ##########
    find_package(ZMQ)
    if(ZMQ_FOUND)
        find_package(PH2_USBINSTLIB)
        if(PH2_USBINSTLIB_FOUND)
        endif(PH2_USBINSTLIB_FOUND)
    endif(ZMQ_FOUND)

    #########
    # Boost #
    #########
    # To use a local installtion of BOOST
    #set(BOOST_ROOT "" CACHE PATH "Boost library path")
    #set(Boost_NO_SYSTEM_PATHS on CACHE BOOL "Do not search system for Boost")
    find_package(Boost 1.53 REQUIRED COMPONENTS system filesystem serialization iostreams program_options regex)

    message("--     ${BoldCyan}#### End ####${Reset}")

    # All the subdirs
    if(${CACTUS_AMC13_FOUND})
        add_subdirectory(AMC13)
    endif()

    MESSAGE(STATUS " ")
    if(CompileForHerd OR NoDataShipping)
        add_subdirectory(tools)
        add_subdirectory(ProductionToolsIT)
        add_subdirectory(HWInterface)
        add_subdirectory(System)
        add_subdirectory(MonitorUtils)
        add_subdirectory(miniDAQ)
    endif()

    if(CompileForShep OR NoDataShipping)
        add_subdirectory(DQMUtils)
        add_subdirectory(RootUtils)
        add_subdirectory(MonitorDQM)
        add_subdirectory(src)
    endif()

    if(NoDataShipping)
        add_subdirectory(RootWeb)
        add_subdirectory(pythonUtils)
    endif()

    add_subdirectory(HWDescription)
    add_subdirectory(Utils)
    add_subdirectory(NetworkUtils)
    add_subdirectory(MessageUtils/cpp)
    add_subdirectory(Parser)

    message("--     ${BoldRed}#### Don't forget to set the path and ld_library_path by sourcing setup.sh before running ###${Reset}")

    MESSAGE(STATUS " ")
    MESSAGE(STATUS "${BoldYellow}PH2_ACF${Reset} [stand-alone]: [${BoldCyan}${PH2ACF_BASE_DIR}/CMakeLists.txt${Reset}]")
    MESSAGE(STATUS "${BoldYellow}PH2_ACF${Reset} [stand-alone]: ${BoldGreen}Compilation of the PH2_ACF package done.${Reset}")
    MESSAGE(STATUS "${BoldBlue}========================================================================================================${Reset}")
    MESSAGE(STATUS " ")

else() # ------------------------------- Compilation in the otsdaq environment ---------------------------------------------

    MESSAGE(STATUS " ")
    MESSAGE(STATUS "${BoldBlue}========================================================================================================${Reset}")
    MESSAGE(STATUS "    ${BoldYellow}PH2_ACF${Reset} [otsdaq]: ${BoldRed}Compilation of the PH2_ACF package${Reset}")
    MESSAGE(STATUS "    ${BoldYellow}PH2_ACF${Reset} [otsdaq]: [${BoldCyan}$ENV{PH2ACF_BASE_DIR}/CMakeLists.txt${Reset}]")
    MESSAGE(STATUS " ")

    set(CompileForShep true)
    set(CompileForHerd false)
    set(NoDataShipping false)

    set (CMAKE_CXX_STANDARD 17)
    cet_set_compiler_flags(
        EXTRA_FLAGS -DELPP_THREAD_SAFE
    )

    include_directories(${CMAKE_CURRENT_SOURCE_DIR})
    include_directories(${CACTUS_INCLUDEDIR})

    add_subdirectory(tools)
    add_subdirectory(DQMUtils)
    add_subdirectory(Utils)
    add_subdirectory(RootUtils)
    add_subdirectory(NetworkUtils)
    add_subdirectory(HWDescription)
    add_subdirectory(HWInterface)
    add_subdirectory(System)
    add_subdirectory(MonitorUtils)
    add_subdirectory(MonitorDQM)
    add_subdirectory(Parser)
    add_subdirectory(MessageUtils/cpp)
    add_subdirectory(miniDAQ)
    add_subdirectory(src)

    MESSAGE(STATUS " ")
    MESSAGE(STATUS "    ${BoldYellow}PH2_ACF${Reset} [otsdaq]: [${BoldCyan}Ph2_ACF/CMakeLists.txt${Reset}]")
    MESSAGE(STATUS "    ${BoldYellow}PH2_ACF${Reset} [otsdaq]: ${BoldGreen}Compilation of the PH2_ACF package done.${Reset}")
    MESSAGE(STATUS "${BoldBlue}========================================================================================================${Reset}")
    MESSAGE(STATUS " ")

endif()
