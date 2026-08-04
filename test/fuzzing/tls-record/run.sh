#!/usr/bin/env bash
# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

scriptDir="$(cd "$(dirname "$0")" && pwd)"
projectDir="$(cd "${scriptDir}/../../.." && pwd)"
buildDir="${ERBSLAND_CORE_FUZZ_BUILD_DIR:-${projectDir}/cmake-build-fuzzing}"
compiler="${CXX:-/opt/local/bin/clang++-mp-22}"
corpusDir="${buildDir}/corpus/tls-record"

if [[ ! -x "${compiler}" ]]; then
    echo "C++ compiler not found: ${compiler}" >&2
    echo "Set CXX to an upstream Clang compiler that includes the libFuzzer runtime." >&2
    exit 1
fi

cmake -S "${projectDir}" -B "${buildDir}" -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_COMPILER="${compiler}" \
    -DCMAKE_UNITY_BUILD=OFF \
    -DERBSLAND_CORE_DEVELOPER_BUILD=OFF \
    -DERBSLAND_CORE_ENABLE_DEMOS=OFF \
    -DERBSLAND_CORE_ENABLE_FUZZING=ON \
    -DERBSLAND_CORE_ENABLE_PRECOMPILED_HEADERS=OFF \
    -DERBSLAND_CORE_ENABLE_TESTS=OFF
cmake --build "${buildDir}" --target tls-record-fuzz

mkdir -p "${corpusDir}"
for source in "${scriptDir}"/corpus/*.hex; do
    destination="${corpusDir}/$(basename "${source}" .hex)"
    xxd -r -p "${source}" "${destination}"
done
exec "${buildDir}/test/fuzzing/tls-record/tls-record-fuzz" "${corpusDir}" "$@"
