// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/mem/ByteArray.hpp>
#include <erbsland/mem/ByteSpan.hpp>
#include <erbsland/mem/impl/SecureErase.hpp>
#include <erbsland/mem/SecureErase.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <algorithm>
#include <cstddef>
#include <span>
#include <utility>
#include <vector>

using el::mem::Byte;
using el::mem::ByteArray;
using el::mem::ByteSpan;
using el::mem::secureErase;

namespace {

struct EraseEvent final {
    std::size_t size{};
    bool isZero{};
};

std::vector<EraseEvent> gEraseEvents;

void observeErase(const std::span<const std::byte> bytes) noexcept {
    gEraseEvents.push_back({bytes.size(), std::ranges::all_of(bytes, [](const std::byte value) noexcept -> bool {
                                return value == std::byte{};
                            })});
}

class EraseObserverGuard final {
public:
    EraseObserverGuard() {
        gEraseEvents.clear();
        el::mem::impl::setSecureEraseObserver(observeErase);
    }
    ~EraseObserverGuard() { el::mem::impl::setSecureEraseObserver(nullptr); }
};

}

static_assert(noexcept(secureErase(std::declval<ByteSpan>())));
static_assert(noexcept(std::declval<ByteArray<4> &>().secureErase()));

TESTED_TARGETS(SecureErase)
class SecureEraseTest final : public el::UnitTest {
public:
    void testByteSpan() {
        const auto observer = EraseObserverGuard{};
        auto bytes = std::vector<Byte>{Byte{1U}, Byte{2U}, Byte{3U}};

        secureErase(ByteSpan{bytes});

        REQUIRE(std::ranges::all_of(bytes, [](const Byte value) noexcept -> bool { return value == Byte{}; }));
        REQUIRE_EQUAL(gEraseEvents.size(), std::size_t{1U});
        REQUIRE_EQUAL(gEraseEvents[0].size, std::size_t{3U});
        REQUIRE(gEraseEvents[0].isZero);
    }

    void testByteArray() {
        const auto observer = EraseObserverGuard{};
        auto bytes = ByteArray{Byte{0xaaU}, Byte{0xbbU}, Byte{0xccU}, Byte{0xddU}};

        bytes.secureErase();

        auto allZero = true;
        static_cast<void>(bytes.forEach([&allZero](const Byte value) noexcept { allZero &= value == Byte{}; }));
        REQUIRE(allZero);
        REQUIRE_EQUAL(gEraseEvents.size(), std::size_t{1U});
        REQUIRE_EQUAL(gEraseEvents[0].size, std::size_t{4U});
        REQUIRE(gEraseEvents[0].isZero);
    }

    void testEmptyInputs() {
        const auto observer = EraseObserverGuard{};
        auto emptyArray = ByteArray<0>{};
        auto emptySpan = ByteSpan{};

        emptyArray.secureErase();
        secureErase(emptySpan);

        REQUIRE(gEraseEvents.empty());
    }
};
