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
class DocumentationHeadersParity2Test final : public UNITTEST_SUBCLASS(ProjectFixture) {
public:
    void testCase5() {
        const auto input = el::String{R"DOCFIX(#pragma once

namespace erbsland::text {

/// Throw an error.
[[noreturn]]
void throwError();

/// Create a value.
[[nodiscard]] constexpr auto
makeValue(int value) noexcept -> int {
    return value;
}

/// Add values.
[[nodiscard]] constexpr auto addValues(
    int first, int second) noexcept -> int {
    return first + second;
}

/// Create a wide value.
[[nodiscard]] constexpr auto wideValue(int value) noexcept
    -> int {
    return value;
}

}
)DOCFIX"_el};
        REQUIRE_EQUAL(
            impl::HeaderScanner{}.scan(input).join("\n"_el).trimmed(),
            el::String{R"DOCFIX(.. doxygenfunction:: erbsland::text::throwError()

.. doxygenfunction:: erbsland::text::makeValue(int value) noexcept -> int

.. doxygenfunction:: erbsland::text::addValues(int first, int second) noexcept -> int

.. doxygenfunction:: erbsland::text::wideValue(int value) noexcept -> int

)DOCFIX"_el}
                .trimmed());
    }

    void testCase6() {
        const auto input = el::String{R"DOCFIX(#pragma once

namespace erbsland::math {

/// A documented class.
template <typename T,
    typename U = T>
    requires true
class Example {
public:
    using Nested = int;
};

/// A documented enum.
enum class Mode { A, B };

using Ignored = int;

/// A documented alias.
using ExampleAlias = Example<int>;

}
)DOCFIX"_el};
        REQUIRE_EQUAL(
            impl::HeaderScanner{}.scan(input).join("\n"_el).trimmed(),
            el::String{R"DOCFIX(.. doxygenclass:: erbsland::math::Example
    :members:

.. doxygenenum:: erbsland::math::Mode

.. doxygentypedef:: erbsland::math::ExampleAlias

)DOCFIX"_el}
                .trimmed());
    }

    void testCase7() {
        const auto input = el::String{R"DOCFIX(#pragma once

namespace erbsland::math {

/// A documented function.
auto add(int left, int right) -> int;

class Undocumented {};

}
)DOCFIX"_el};
        REQUIRE_EQUAL(
            impl::HeaderScanner{}.scan(input).join("\n"_el).trimmed(),
            el::String{R"DOCFIX(.. doxygenfunction:: erbsland::math::add(int left, int right) -> int

)DOCFIX"_el}
                .trimmed());
    }

    void testCase8() {
        const auto input = el::String{R"DOCFIX(#pragma once

namespace erbsland::text {

/// A documented alias.
using StringEditorList = int;

}
)DOCFIX"_el};
        REQUIRE_EQUAL(
            impl::HeaderScanner{}.scan(input).join("\n"_el).trimmed(),
            el::String{R"DOCFIX(.. doxygentypedef:: erbsland::text::StringEditorList

)DOCFIX"_el}
                .trimmed());
    }
};
