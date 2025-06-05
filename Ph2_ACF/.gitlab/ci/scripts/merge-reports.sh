#!/bin/sh

# Originally by J Sturdy
# Merges the JSON reports produced by generate-report.sh

if [ ! -n "${REPORTS_PATH}" ]
then
    echo "REPORTS_PATH not set, setting to 'reports'"
    REPORTS_PATH=reports
fi

## gl-code-quality-report.json
outputfile=${REPORTS_PATH}/gl-code-quality-report.json

## Merge all JSON files together
jq -s '[.[][]]' ${REPORTS_PATH}/*.json > ${outputfile}

## Remove leading ./ in path attribute
perl -pi -e 's|"./|"|g' ${outputfile}

