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
class DocumentationGuidelinesParity4Test final : public UNITTEST_SUBCLASS(ProjectFixture) {
public:
    void testCase13() {
        const auto input = el::String{R"DOCFIX(**********************
Example API Guidelines
**********************

Primary Types
=============

.. code-block:: text

    widget // invalid lowercase type.
    List<Value> // use a placeholder-based type pattern
)DOCFIX"_el};
        REQUIRE_FALSE(impl::ApiGuidelines{}.validate(input).isEmpty());
    }
};
