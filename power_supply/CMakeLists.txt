if(NOT DEFINED ENV{OTSDAQ_CMSOUTERTRACKER_DIR})
   cmake_minimum_required(VERSION 2.8)
   project(PowerSupplies)

   #set the output directory
   if ( ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR} )
      message( FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt." )
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

   #set the cmakeL module path
   set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules")

   # compiler flags
   set (CMAKE_CXX_FLAGS "-std=c++11 -fPIC -O3 -Wcpp -pthread -pedantic -Wall -DBOOST_NO_CXX11_SCOPED_ENUMS -ggdb3")

   find_package (CAENHVWrapper)
   if (CAENHVWRAPPER_FOUND)
      add_compile_options(-DLINUX -DUNIX -D__CAEN__)
   endif (CAENHVWRAPPER_FOUND)

   add_subdirectory(NetworkUtils)
   add_subdirectory(utils)
   add_subdirectory(src)
   add_subdirectory(bin)

else() # ------------------------------- Compilation in the otsdaq environment ---------------------------------------------
   MESSAGE(STATUS " ")
   MESSAGE(STATUS "${BoldBlue}========================================================================================================${Reset}")
   MESSAGE(STATUS "${BoldYellow}MIDDLEWARE${Reset} [otsdaq]: Compilation of the power supply package for OTSDAQ")
   MESSAGE(STATUS "${BoldYellow}MIDDLEWARE${Reset} [otsdaq]: [${BoldCyan}${BASE_DIR}/CMakeLists.txt${Reset}]")
   MESSAGE(STATUS " ")   

   set(CMAKE_MODULE_PATH 
        $ENV{PH2ACF_BASE_DIR}/../power_supply/cmake/Modules
        ${CMAKE_MODULE_PATH})
   message(STATUS "CMAKE_MODULE_PATH is ${CMAKE_MODULE_PATH}")

   find_package (CAENHVWrapper)
   if (CAENHVWRAPPER_FOUND)
      add_compile_options(-DLINUX -DUNIX -D__CAEN__)
   endif (CAENHVWRAPPER_FOUND)

   add_subdirectory(src)
   #add_subdirectory(NetworkUtils)

   MESSAGE(STATUS " ")
   MESSAGE(STATUS "${BoldYellow}MIDDLEWARE${Reset} [otsdaq]: [${BoldCyan}${BASE_DIR}/CMakeLists.txt${Reset}]")
   MESSAGE(STATUS "${BoldYellow}MIDDLEWARE${Reset} [otsdaq]: Compilation of the middleware package done.")
   MESSAGE(STATUS "${BoldBlue}========================================================================================================${Reset}")

endif()
