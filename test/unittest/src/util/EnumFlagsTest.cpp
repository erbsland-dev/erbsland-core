// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/unittest/UnitTest.hpp>
#include <erbsland/util/EnumFlags.hpp>

#include <concepts>
#include <cstdint>
#include <functional>
#include <type_traits>

using el::util::EnumFlags;

namespace erbsland::test::enumflagstest {

enum class Flag : uint8_t {
    None = 0U,
    Read = 1U << 0U,
    Write = 1U << 1U,
    Execute = 1U << 2U,
    All = (1U << 0U) | (1U << 1U) | (1U << 2U),
};

using Flags = EnumFlags<Flag>;

[[nodiscard]] constexpr auto operator|(Flag left, Flag right) noexcept -> Flags {
    return Flags{left} | right;
}

enum class OpenFlag : uint16_t {
    Read = 1U << 0U,
    Write = 1U << 1U,
};

using OpenFlags = EnumFlags<OpenFlag>;

enum class SignedFlag : int {
    Read = 1,
};

enum PlainFlag : uint8_t {
    PlainRead = 1U,
};

template <typename tType>
concept CanCreateEnumFlags = requires { sizeof(EnumFlags<tType>); };

template <typename tType>
concept HasComplement = requires(tType value) { ~value; };

}

using namespace erbsland::test::enumflagstest;

TESTED_TARGETS(EnumFlags)
class EnumFlagsTest final : public el::UnitTest {
public:
    void testCompileTimeContracts() {
        static_assert(CanCreateEnumFlags<Flag>);
        static_assert(CanCreateEnumFlags<OpenFlag>);
        static_assert(!CanCreateEnumFlags<SignedFlag>);
        static_assert(!CanCreateEnumFlags<PlainFlag>);
        static_assert(!CanCreateEnumFlags<int>);

        static_assert(std::same_as<typename Flags::Enum, Flag>);
        static_assert(std::same_as<typename Flags::Value, uint8_t>);
        static_assert(std::constructible_from<Flags, Flag>);
        static_assert(!std::constructible_from<Flags, uint8_t>);
        static_assert(!std::convertible_to<Flags, uint8_t>);
        static_assert(std::equality_comparable<Flags>);
        static_assert(HasComplement<Flags>);
        static_assert(!HasComplement<OpenFlags>);

        static_assert(Flags{}.isEmpty());
        static_assert(!Flags{}.hasAny());
        static_assert(!Flags{}.isSet(Flag::None));
        static_assert(Flags{Flag::Read}.isSet(Flag::Read));
        static_assert((Flags{Flag::Read} | Flag::Write).contains(Flags{Flag::Read, Flag::Write}));
        static_assert((Flag::Read | Flag::Write).toRawValue() == 0x03U);
        static_assert((~Flags{Flag::Read}).toRawValue() == 0x06U);
        static_assert(Flags::fromRawValue(0x80U).toRawValue() == 0x80U);
    }

    void testConstructionAndAccessors() {
        const auto empty = Flags{};
        REQUIRE(empty.isEmpty());
        REQUIRE_FALSE(empty.hasAny());
        REQUIRE_FALSE(empty.isSet(Flag::None));
        REQUIRE_FALSE(empty.isSet(Flag::Read));
        REQUIRE(empty.contains(Flags{}));
        REQUIRE_FALSE(empty.intersects(Flags{Flag::Read}));

        const auto read = Flags{Flag::Read};
        REQUIRE_FALSE(read.isEmpty());
        REQUIRE(read.hasAny());
        REQUIRE(read.isSet(Flag::Read));
        REQUIRE_FALSE(read.isSet(Flag::Write));
        REQUIRE(read.contains(Flags{Flag::Read}));
        REQUIRE_FALSE(read.contains(Flags{Flag::Read, Flag::Write}));
        REQUIRE(read.intersects(Flags{Flag::Read, Flag::Write}));

        const auto readWrite = Flags{Flag::Read, Flag::Write};
        REQUIRE_EQUAL(readWrite.toRawValue(), 0x03U);

        const auto raw = Flags::fromRawValue(0x80U);
        REQUIRE_EQUAL(raw.toRawValue(), 0x80U);
        REQUIRE_FALSE(raw.isSet(Flag::Read));
    }

    void testOperators() {
        const auto read = Flags{Flag::Read};
        const auto write = Flags{Flag::Write};
        const auto execute = Flags{Flag::Execute};

        REQUIRE((read | write) == Flags{Flag::Read, Flag::Write});
        REQUIRE((read | Flag::Write) == Flags{Flag::Read, Flag::Write});
        REQUIRE((Flag::Write | read) == Flags{Flag::Read, Flag::Write});
        REQUIRE((Flag::Read | Flag::Write) == Flags{Flag::Read, Flag::Write});

        REQUIRE(((read | write) & read) == read);
        REQUIRE(((read | write) & Flag::Read) == read);
        REQUIRE((Flag::Read & (read | write)) == read);

        REQUIRE(((read | write) ^ write) == read);
        REQUIRE((read ^ Flag::Write) == Flags{Flag::Read, Flag::Write});
        REQUIRE((Flag::Write ^ read) == Flags{Flag::Read, Flag::Write});
        REQUIRE((read ^ read).isEmpty());

        REQUIRE((~read) == (write | execute));
        REQUIRE_EQUAL((~Flags::fromRawValue(0x80U)).toRawValue(), 0x07U);

        auto flags = read;
        flags |= Flag::Write;
        REQUIRE(flags == (read | write));
        flags &= Flag::Read;
        REQUIRE(flags == read);
        flags ^= Flag::Execute;
        REQUIRE(flags == (read | execute));
        flags |= write;
        REQUIRE(flags == Flags{Flag::Read, Flag::Write, Flag::Execute});
        flags &= Flags{Flag::Read, Flag::Write};
        REQUIRE(flags == (read | write));
        flags ^= Flags{Flag::Read, Flag::Execute};
        REQUIRE(flags == (write | execute));
    }

    void testModificationMethods() {
        auto flags = Flags{};

        flags.set(Flag::Read);
        REQUIRE(flags == Flags{Flag::Read});

        flags.set(Flags{Flag::Write, Flag::Execute});
        REQUIRE(flags == Flags{Flag::Read, Flag::Write, Flag::Execute});

        flags.clear(Flag::Write);
        REQUIRE(flags == Flags{Flag::Read, Flag::Execute});

        flags.clear(Flags{Flag::Read, Flag::Write});
        REQUIRE(flags == Flags{Flag::Execute});

        flags.clear();
        REQUIRE(flags.isEmpty());

        flags = Flags{Flag::Read, Flag::Execute};
        flags.replaceMasked(Flags{Flag::Write}, Flags{Flag::Read, Flag::Write});
        REQUIRE(flags == Flags{Flag::Write, Flag::Execute});

        flags.replaceMasked(Flags::fromRawValue(0x80U), Flags{Flag::Read, Flag::Write});
        REQUIRE(flags == Flags{Flag::Execute});
    }

    void testHashSupport() {
        REQUIRE(
            std::hash<Flags>{}(Flags{Flag::Read, Flag::Write}) == std::hash<Flags>{}(Flags{Flag::Read, Flag::Write}));
        REQUIRE(std::hash<Flags>{}(Flags::fromRawValue(0x80U)) == std::hash<uint8_t>{}(0x80U));
    }
};
