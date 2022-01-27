#includes
include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/utils)

#library dirs
link_directories(${PROJECT_SOURCE_DIR/lib})

#initial set of libraries
#set(LIBS ${LIBS} m boost_filesystem  pthread  dl)
set(LIBS ${LIBS} PowerSupply pugixml boost_system boost_program_options boost_filesystem)

if (CAENHVWRAPPER_FOUND)
    set(LIBS ${LIBS} caenhvwrapper)
endif (CAENHVWRAPPER_FOUND)


####################################
## EXECUTABLES
####################################

#PowerSupplyController
add_executable(PowerSupplyController PowerSupplyController.cc)
target_link_libraries(PowerSupplyController ${LIBS})

#PowerSupplyController
add_executable(ITPowerSupplyController ITPowerSupplyController.cc)
target_link_libraries(ITPowerSupplyController ${LIBS})

#powersupply
add_executable(powersupply powersupply.cc)
target_link_libraries(powersupply ${LIBS})

#RunPowerSupply
add_executable(RunPowerSupply RunPowerSupply.cc)
target_link_libraries(RunPowerSupply ${LIBS})

#TestTTi
add_executable(TestTTi TestTTi.cc)
target_link_libraries(TestTTi ${LIBS})

#TestRohdeSchwarz
add_executable(TestRohdeSchwarz TestRohdeSchwarz.cc)
target_link_libraries(TestRohdeSchwarz ${LIBS})

#TestHameg7044
add_executable(TestHameg TestHameg.cc)
target_link_libraries(TestHameg ${LIBS})

#TestHameg7044
add_executable(TestKeithley TestKeithley.cc)
target_link_libraries(TestKeithley ${LIBS})

#TestArduino
add_executable(TestArduino TestArduino.cc)
target_link_libraries(TestArduino ${LIBS})

#IV_keithley_2410
add_executable(IV_keithley_2410 IV_keithley_2410.cc)
target_link_libraries(IV_keithley_2410 ${LIBS})

#TestIsegSHR
add_executable(TestIsegSHR TestIsegSHR.cc)
target_link_libraries(TestIsegSHR ${LIBS})

#ResetITLVPower
add_executable(ResetITLVPower ResetITLVPower.cc)
target_link_libraries(ResetITLVPower ${LIBS})

#ITIVCurve
add_executable(ITIVCurve ITIVCurve.cc)
target_link_libraries(ITIVCurve ${LIBS})

#TestAgilent
add_executable(TestAgilent TestAgilent.cc)
target_link_libraries(TestAgilent ${LIBS})


#TestCAEN
if (CAENHVWRAPPER_FOUND)
add_executable(TestCAEN_SY4527 TestCAEN_SY4527.cc)
target_link_libraries(TestCAEN_SY4527 ${LIBS})
endif (CAENHVWRAPPER_FOUND)
