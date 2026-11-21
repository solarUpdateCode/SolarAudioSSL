#!/usr/bin/env bash
set -euo pipefail
VERSION="1.0.0"
BUILD_DIR="build/SolarAudioSSL_artefacts/Release"
DIST_DIR="dist"
STAGE_DIR="build/pkg_stage"
rm -rf "${DIST_DIR}" "${STAGE_DIR}"
mkdir -p "${DIST_DIR}" "${STAGE_DIR}/vst3"
cp -R "${BUILD_DIR}/VST3/SolarAudioSSL.vst3" "${STAGE_DIR}/vst3/"
pkgbuild --root "${STAGE_DIR}/vst3" --identifier "com.solaraudio.solaraudiossl.vst3" --version "${VERSION}" --install-location "/Library/Audio/Plug-Ins/VST3" "${DIST_DIR}/SolarAudioSSL_${VERSION}_macOS_VST3_Installer.pkg"
echo "Built macOS VST3 installer in ${DIST_DIR}/"
