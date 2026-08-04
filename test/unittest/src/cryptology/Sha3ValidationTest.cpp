// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CryptologyTestHelper.hpp"

#include <erbsland/cryptology/Hasher.hpp>

using namespace el::cryptology;

TESTED_TARGETS(Hasher Sha3)
class Sha3ValidationTest final : public UNITTEST_SUBCLASS(CryptologyTestHelper) {
public:
    void testZeroByte() {
        auto hash = Hasher{HashAlgorithm::Sha3_256};
        hash.update(bytesFromHex("00"));
        REQUIRE_EQUAL(
            hash.finalize(), bytesFromHex("5d53469f20fef4f8eab52b88044ede69c77a6a68a60728609fc4a65ff531e7d0"));
    }

    void testFourByteMessage() {
        auto hash = Hasher{HashAlgorithm::Sha3_256};
        hash.update(bytesFromHex("74657374"));
        REQUIRE_EQUAL(
            hash.finalize(), bytesFromHex("36f028580bb02cc8272a9a020f4200e346e276ae664e45ee80745574e2f5ab80"));
    }
};
