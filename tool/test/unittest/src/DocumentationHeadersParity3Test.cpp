// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ProjectFixture.hpp"

#include <erbsland/PathInfo.hpp>
#include <tool/doc/impl/ApiGuidelines.hpp>
#include <tool/doc/impl/HeaderScanner.hpp>
#include <tool/doc/impl/RstCleaner.hpp>

using namespace el::text::literals;
using namespace tool::doc;

TESTED_TARGETS(ApiGuidelines HeaderScanner RstCleaner DoxygenIndex)
class DocumentationHeadersParity3Test final : public UNITTEST_SUBCLASS(ProjectFixture) {
public:
    void testCase9() {
        const auto input = el::String{R"DOCFIX(#pragma once

namespace erbsland::text {

/// A documented alias.
using StringMap = int;

}
)DOCFIX"_el};
        REQUIRE_EQUAL(
            impl::HeaderScanner{}.scan(input).join("\n"_el).trimmed(),
            el::String{R"DOCFIX(.. doxygentypedef:: erbsland::text::StringMap

)DOCFIX"_el}
                .trimmed());
    }

    void testCase10() {
        const auto input = el::String{R"DOCFIX(#pragma once

namespace erbsland::math {

class Undocumented {};

}
)DOCFIX"_el};
        REQUIRE_EQUAL(
            impl::HeaderScanner{}.scan(input).join("\n"_el).trimmed(), el::String{R"DOCFIX()DOCFIX"_el}.trimmed());
    }

    void testCase11() {
        const auto input = el::String{R"DOCFIX(namespace erbsland::unit {
/// A public Core type.
class Version {};
}
namespace std {
/// Standard-library integration for Core's public type.
template <>
struct hash<erbsland::unit::Version> {};
}
)DOCFIX"_el};
        REQUIRE_EQUAL(
            impl::HeaderScanner{}.scan(input).join("\n"_el).trimmed(),
            el::String{R"DOCFIX(.. doxygenclass:: erbsland::unit::Version
    :members:

)DOCFIX"_el}
                .trimmed());
    }

    void testCase12() {
        const auto input = el::String{R"DOCFIX(#pragma once

namespace erbsland::math {

/// A documented alias.
using Example = int;

}
)DOCFIX"_el};
        REQUIRE_EQUAL(
            impl::HeaderScanner{}.scan(input).join("\n"_el).trimmed(),
            el::String{R"DOCFIX(.. doxygentypedef:: erbsland::math::Example

)DOCFIX"_el}
                .trimmed());
    }
};
