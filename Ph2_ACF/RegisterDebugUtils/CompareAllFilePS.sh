#!/bin/bash

# Check if the correct number of arguments is provided
if [ "$#" -ne 1 ] && [ "$#" -ne 2 ]; then
    echo "Usage: $0 <RunNumber> ->  Compare files with Default"
    echo "Usage: $0 <RunNumber_1> <RunNumber_2>->  Compare files between run numbers"
    exit 1
fi

g++ -o ${PH2ACF_BASE_DIR}/RegisterDebugUtils/RegisterDifference ${PH2ACF_BASE_DIR}/RegisterDebugUtils/RegisterDifference.cc
g++ -o ${PH2ACF_BASE_DIR}/RegisterDebugUtils/BoardRegisterDifference ${PH2ACF_BASE_DIR}/RegisterDebugUtils/BoardRegisterDifference.cc -lpugixml

if [ "$#" -eq 1 ]; then
    run_number="$1"

    echo "Comparing Board files"
    ${PH2ACF_BASE_DIR}/RegisterDebugUtils/CompareBoardFilesWithDefault.sh ${PH2ACF_BASE_DIR}/settings/BeBoardFiles/uDTC_registers_PS.xml ${run_number}
    echo
    echo
    echo

    echo "Comparing LpGBT files"
    ${PH2ACF_BASE_DIR}/RegisterDebugUtils/CompareFilesWithDefault.sh ${PH2ACF_BASE_DIR}/settings/lpGBTFiles/lpGBT_v1_PS.txt ${run_number} "BE*_OG*_lpGBT*.txt" LpGBT
    echo
    echo
    echo

    echo "Comparing VTRx files"
    ${PH2ACF_BASE_DIR}/RegisterDebugUtils/CompareFilesWithDefault.sh ${PH2ACF_BASE_DIR}/settings/VTRxFiles/VTRx.txt ${run_number} "BE*_OG*_VTRx*.txt" VTRx
    echo
    echo
    echo

    echo "Comparing CIC files"
    ${PH2ACF_BASE_DIR}/RegisterDebugUtils/CompareFilesWithDefault.sh ${PH2ACF_BASE_DIR}/settings/CicFiles/CIC2_PS.txt ${run_number} "BE*_OG*_FE*_CIC.txt" CIC
    echo
    echo
    echo

    echo "Comparing SSA files"
    ${PH2ACF_BASE_DIR}/RegisterDebugUtils/CompareFilesWithDefault.sh ${PH2ACF_BASE_DIR}/settings/SSAFiles/SSA2.txt ${run_number} "BE*_OG*_FE*_Chip*SSA.txt" SSA
    echo
    echo
    echo


    echo "Comparing MPA files"
    ${PH2ACF_BASE_DIR}/RegisterDebugUtils/CompareFilesWithDefault.sh ${PH2ACF_BASE_DIR}/settings/MPAFiles/MPA2.txt ${run_number} "BE*_OG*_FE*_Chip*MPA.txt" MPA
    echo
    echo
    echo

else

    run_number_1="$1"
    run_number_2="$2"

    echo "Comparing Board files"
    ${PH2ACF_BASE_DIR}/RegisterDebugUtils/CompareBoardFilesFromTwoRuns.sh ${run_number_1} ${run_number_2}
    echo
    echo
    echo

    echo "Comparing LpGBT files"
    ${PH2ACF_BASE_DIR}/RegisterDebugUtils/CompareFilesFromTwoRuns.sh ${run_number_1} ${run_number_2} "BE*_OG*_lpGBT*.txt" LpGBT
    echo
    echo
    echo

    echo "Comparing VTRx files"
    ${PH2ACF_BASE_DIR}/RegisterDebugUtils/CompareFilesFromTwoRuns.sh ${run_number_1} ${run_number_2} "BE*_OG*_VTRx*.txt" VTRx
    echo
    echo
    echo

    echo "Comparing CIC files"
    ${PH2ACF_BASE_DIR}/RegisterDebugUtils/CompareFilesFromTwoRuns.sh ${run_number_1} ${run_number_2} "BE*_OG*_FE*_CIC.txt" CIC
    echo
    echo
    echo

    echo "Comparing SSA files"
    ${PH2ACF_BASE_DIR}/RegisterDebugUtils/CompareFilesFromTwoRuns.sh ${run_number_1} ${run_number_2} "BE*_OG*_FE*_Chip*SSA.txt" SSA
    echo
    echo
    echo


    echo "Comparing MPA files"
    ${PH2ACF_BASE_DIR}/RegisterDebugUtils/CompareFilesFromTwoRuns.sh ${run_number_1} ${run_number_2} "BE*_OG*_FE*_Chip*MPA.txt" MPA
    echo
    echo
    echo
fi