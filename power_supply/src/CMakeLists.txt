if(NOT DEFINED ENV{OTSDAQ_CMSOUTERTRACKER_DIR})
   #includes
   #include_directories(${PROJECT_SOURCE_DIR}/include)
   include_directories(${PROJECT_SOURCE_DIR}/NetworkUtils)
   include_directories(${PROJECT_SOURCE_DIR}/utils)
   set(LIBS ${LIBS} NetworkUtils PowerSupply_utils)

   #initial set of libraries
   #find source files
   file(GLOB HEADERS *.h)
   file(GLOB SOURCES *.cc)

   if (NOT CAENHVWRAPPER_FOUND)
      list(REMOVE_ITEM HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/CAEN.h)
      list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/CAEN.cc)
      list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/CAENErrors.h)
   endif()

   #message(STATUS ${SOURCES})
   #add the library
   add_library(PowerSupply SHARED ${SOURCES} ${HEADERS})
   TARGET_LINK_LIBRARIES(PowerSupply ${LIBS})

else() # ------------------------------- Compilation in the otsdaq environment ---------------------------------------------

   #initial set of libraries
   set(LIBS pugixml boost_system boost_program_options)

   set(EXCLUDE_LIST readxml.cc)

   if (CAENHVWRAPPER_FOUND)
      set(LIBS ${LIBS} caenhvwrapper)
   else()
      set(EXCLUDE_LIST ${EXCLUDE_LIST} CAEN.h CAEN.cc CAENErrors.h)
   endif (CAENHVWRAPPER_FOUND)
    
   cet_set_compiler_flags(
      EXTRA_FLAGS -Wno-reorder
   )

   cet_make(LIBRARY_NAME Ph2_PowerSupplies
            LIBRARIES
            pthread
            ${LIBS}
            NetworkUtils_${Ph2_ACF_Master}
            #${Boost_SYSTEM_LIBRARY}
            EXCLUDE ${EXCLUDE_LIST}
   )

   install_headers()
   install_source()
endif()