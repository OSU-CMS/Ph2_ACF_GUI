if(NOT DEFINED ENV{OTSDAQ_CMSOUTERTRACKER_DIR})
   #includes
   #initial set of libraries
   #find source files
   file(GLOB HEADERS *.h)
   file(GLOB SOURCES *.cc)

   #message(STATUS ${SOURCES})
   #add the library
   add_library(PowerSupply_utils SHARED ${SOURCES} ${HEADERS})
   TARGET_LINK_LIBRARIES(PowerSupply_utils ${LIBS})

else() # ------------------------------- Compilation in the otsdaq environment ---------------------------------------------

   #initial set of libraries
   set(LIBS pugixml boost_system boost_program_options)
    
   cet_set_compiler_flags(
      EXTRA_FLAGS -Wno-reorder
   )

   cet_make(LIBRARY_NAME Ph2_PowerSupply_utils
            LIBRARIES
            pthread
            ${LIBS}
            #${Boost_SYSTEM_LIBRARY}
            EXCLUDE ${EXCLUDE_LIST}
   )

   install_headers()
   install_source()
endif()