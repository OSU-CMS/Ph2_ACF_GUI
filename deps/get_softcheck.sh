#!/usr/bin/env bash

echo "$# arguments passed"

echo "[$0]: Will download pySoftcheck from Huber website"
echo "[$0]: Please note: this will not install Huber runtime package!"
echo "[$0]: Please get the runtime package from https://www.huber-online.com/en/contact-service/downloads/software and install."

echo '[0]: Remove old pySoftcheck download and links'
# Remove third-party
rm -rf "${TMPDIR}/third-party"

SOFTCHECK_ARCHIVE="pySoftcheck.zip"
mkdir -p "${TMPDIR}/third-party"
curl "https://www.huber-online.com/fileadmin/user_upload/huber-online.com/Downloads/Software/pySoftcheck.zip" > "${TMPDIR}/third-party/${SOFTCHECK_ARCHIVE}" || (echo "Could not download ${SOFTCHECK_ARCHIVE} from https://www.huber-online.com/fileadmin/user_upload/huber-online.com/Downloads/Software/pySoftcheck.zip ... Giving up!"; exit 1)

echo '[2]: Unpack pySoftcheck distribution'
unzip "${TMPDIR}/third-party/${SOFTCHECK_ARCHIVE}" -d "${TMPDIR}/third-party/" || (echo "Could not unzip ${SOFTCHECK_ARCHIVE}... Giving up!"; exit 1)
rm "${TMPDIR}/third-party/${SOFTCHECK_ARCHIVE}"

# find wheel file
SOFTCHECK_WHEEL=$(find ${TMPDIR}/third-party/ -name *.whl)
echo "     Found pySoftcheck wheel: ${SOFTCHECK_WHEEL}"

echo '[3]: Install in python venv'
python3 -m pip install "${SOFTCHECK_WHEEL}"

echo '[4]: Done.'
