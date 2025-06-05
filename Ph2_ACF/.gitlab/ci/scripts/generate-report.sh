#!/bin/sh

# Originally by J Sturdy

usage() {
    cat <<EOF
Usage: $0 <filename>

Parse the output of a compilation into a JSON file compatible with
GitLab CI Code Quality reporting back end (CodeClimate).
EOF

    exit 1
}

if [ ! -f $1 ] || [ ! -n "$1" ]
then
    echo "Invalid file specified"
    usage
fi

inputfile=$1

if [ ! -n "${REPORTS_PATH}" ]
then
    echo "REPORTS_PATH not set, setting to 'reports'"
    REPORTS_PATH=reports
    mkdir -p ${REPORTS_PATH}
fi

outputfile=${inputfile%*.txt}.json
if [ ! -n "${outputfile}" ]
then
    outputfile=${inputfile}.json
fi

perl -pi -e 's|\\.|\\\\.|g' ${inputfile}
perl -pi -e "s|${CI_PROJECT_DIR}/${QUALITY_CHECK_DIR}/../${CI_PROJECT_NAME}/|../${CI_PROJECT_NAME}/|g" ${inputfile}

cat ${inputfile} | \
    grep -Ev "(^ |/cvmfs/|_deps|extern/|DAL_${CI_PROJECT_NAME}.tmp.cpp|${CI_PROJECT_NAME}Dal)" |
    grep -Ev '(^ |cc1: warning: command-line|clang-diagnostic-error|note:|fatal error|No such file or directory)'| \
    grep -E '(error|note|warning):' | \
    sed -E 's/^([^:]+):([^:]+):([^:]+): ([^:]+): (.*) (\[.*\])$/{\n\t"description":"\6 \5",\n\t"severity":"\4",\n\t"fingerprint":\4:\6:\1:\2:\3\n\t"location": {\n\t\t"path":"\1",\n\t\t"lines": {\n\t\t\t"begin":\2\n\t\t}\n\t},\n\t"type":"\4",\n\t"check_name": "\6"\n},/' | \
    awk -F':' '{ if ( match($1, "fingerprint") == 0) {print $0 } else {cmd="echo "$2":"$3":"$4":"$5":"$6":$(sed -n "$5"p "$4")|md5sum"; cmd|getline myhash; close(cmd); split(myhash,a," "); {print $1 ":\""a[1]"\","}}}' \
        > ${outputfile}

# The path to the source file must be relative to the repository root
# ## In-source build in the CI
# perl -pi -e "s|${PWD}/||g" ${outputfile}

## Out-of-source build in the CI
perl -pi -e "s|${CI_PROJECT_DIR}/${QUALITY_CHECK_DIR}/${CI_PROJECT_NAME}/||g" ${outputfile}
# perl -pi -e "s|${CI_PROJECT_DIR}/${QUALITY_CHECK_DIR}/../${CI_PROJECT_NAME}/||g" ${outputfile}
perl -pi -e "s|${CI_PROJECT_DIR}/${CI_PROJECT_NAME}/||g" ${outputfile}
perl -pi -e "s|..//${CI_PROJECT_NAME}/||g" ${outputfile}
perl -pi -e "s|../${CI_PROJECT_NAME}/||g" ${outputfile}

## Remove leading ./ in path attribute
perl -pi -e 's|"./|"|g' ${outputfile}

# Make standard JSON conforming
## Insert braces
echo ']' >> ${outputfile}
sed -i '0,/^/s//\[\n/' ${outputfile}

## Remove trailing ','
perl -0777 -pi -e 's/,(\s*\n?\s*[}\]])/\1/g' ${outputfile}

