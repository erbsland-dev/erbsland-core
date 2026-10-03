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
class DocumentationHeadersParity1Test final : public UNITTEST_SUBCLASS(ProjectFixture) {
public:
    void testCase1() {
        const auto input = el::String{R"DOCFIX(#pragma once

namespace erbsland::common {

/// A documented alias repeated by two public headers.
using Shared = int;

}
)DOCFIX"_el};
        REQUIRE_EQUAL(
            impl::HeaderScanner{}.scan(input).join("\n"_el).trimmed(),
            el::String{R"DOCFIX(.. doxygentypedef:: erbsland::common::Shared

)DOCFIX"_el}
                .trimmed());
    }

    void testCase2() {
        const auto input = el::String{R"DOCFIX(#pragma once

#include "Length_fwd.hpp"
)DOCFIX"_el};
        REQUIRE_EQUAL(
            impl::HeaderScanner{}.scan(input).join("\n"_el).trimmed(), el::String{R"DOCFIX()DOCFIX"_el}.trimmed());
    }

    void testCase3() {
        const auto input = el::String{R"DOCFIX(#pragma once

namespace erbsland::unit {

/// A documented unit alias.
using Length = int;

}
)DOCFIX"_el};
        REQUIRE_EQUAL(
            impl::HeaderScanner{}.scan(input).join("\n"_el).trimmed(),
            el::String{R"DOCFIX(.. doxygentypedef:: erbsland::unit::Length

)DOCFIX"_el}
                .trimmed());
    }

    void testCase4() {
        const auto input = el::String{R"DOCFIX(#pragma once

namespace erbsland::text {

/// A documented concept.
template <typename T>
concept StringConvertible = true;

}
)DOCFIX"_el};
        REQUIRE_EQUAL(
            impl::HeaderScanner{}.scan(input).join("\n"_el).trimmed(),
            el::String{R"DOCFIX(.. doxygenconcept:: erbsland::text::StringConvertible

)DOCFIX"_el}
                .trimmed());
    }
};
