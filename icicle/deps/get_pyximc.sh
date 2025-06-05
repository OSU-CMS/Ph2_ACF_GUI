#!/usr/bin/env bash

LIBXIMC_VERSION='2.14.28'

echo "$# arguments passed"

if [[ "$#" > "0" ]] ; then LIBXIMC_VERSION="$1" ; fi

echo "[$0]: Will download pyximc profiles from libximc version ${LIBXIMC_VERSION}"
echo "[$0]: Please note: this will not install libximc libraries or python package - this is your responsibility!"
echo "[$0]: This script only locally downloads and links the required python profiles."

echo '[0]: Remove old pyximc download and links'
# Remove symlinks
rm -rf pyximc.py
rm -rf pyximc_profiles
rm -rf libximc
rm -rf libximc_profiles

# Remove third-party/ximc
rm -rf "${TMPDIR}/third-party/ximc-${LIBXIMC_VERSION}/"

LIBXIMC_ARCHIVE="libximc-${LIBXIMC_VERSION}-all.tar.gz"
echo "[1]: Download libximc distribution ${LIBXIMC_ARCHIVE}"
mkdir -p "${TMPDIR}/third-party"
curl "https://files.xisupport.com/libximc/${LIBXIMC_ARCHIVE}" > "${TMPDIR}/third-party/${LIBXIMC_ARCHIVE}" || (echo "Could not download ${LIBXIMC_ARCHIVE} from https://files.xisupport.com/libximc/... Giving up!"; exit 1)

echo '[2]: Unpack libximc distribution'
tar -xvzf "${TMPDIR}/third-party/${LIBXIMC_ARCHIVE}" -C "${TMPDIR}/third-party/" || (echo "Could not unzip ${LIBXIMC_ARCHIVE}... Giving up!"; exit 1)
rm "${TMPDIR}/third-party/${LIBXIMC_ARCHIVE}"

echo '[3]: Symlink libximc_profiles'
cp -rv "${TMPDIR}/third-party/ximc-${LIBXIMC_VERSION}/ximc/python-profiles" libximc_profiles
if [ ! -d libximc_profiles ] || [ ! -e libximc_profiles ]; then
  echo "Failed to copy ${TMPDIR}/third-party/ximc-${LIBXIMC_VERSION}/ximc/python-profiles => libximc_profiles"
fi

echo '[4]: Done.'
