#!/usr/bin/env bash
# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

scriptDir="$(cd "$(dirname "$0")" && pwd)"
projectDir="$(cd "${scriptDir}/../../.." && pwd)"
buildDir="${ERBSLAND_CORE_FUZZ_BUILD_DIR:-${projectDir}/cmake-build-fuzzing}"
compiler="${CXX:-/opt/local/bin/clang++-mp-22}"
corpusDir="${buildDir}/corpus/pkcs8-key"

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
cmake --build "${buildDir}" --target pkcs8-key-fuzz

mkdir -p "${corpusDir}"
for seed in "${scriptDir}"/corpus/*.hex; do
    xxd -r -p "${seed}" "${corpusDir}/$(basename "${seed}" .hex)"
done
cp "${projectDir}/test/unittest/data/cryptology/signing/rsa-2048-pkcs8.pem" "${corpusDir}/rsa-2048-pkcs8.pem"
exec "${buildDir}/test/fuzzing/pkcs8-key/pkcs8-key-fuzz" "${corpusDir}" "$@"
