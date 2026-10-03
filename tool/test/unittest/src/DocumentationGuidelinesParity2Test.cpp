// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ProjectFixture.hpp"

#include <erbsland/CpCount.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/StringJoins.hpp>
#include <tool/doc/impl/ApiGuidelines.hpp>
#include <tool/doc/impl/HeaderScanner.hpp>
#include <tool/doc/impl/RstCleaner.hpp>

using namespace el::text::literals;
using namespace tool::doc;

TESTED_TARGETS(ApiGuidelines HeaderScanner RstCleaner DoxygenIndex)
class DocumentationGuidelinesParity2Test final : public UNITTEST_SUBCLASS(ProjectFixture) {
public:
    void testCase5() {
        const auto input = el::String{R"DOCFIX(**********************
Example API Guidelines
**********************

Primary Types
=============

.. code-block:: text

     Widget // wrong indentation
    Other // xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
)DOCFIX"_el};
        REQUIRE_FALSE(impl::ApiGuidelines{}.validate(input).isEmpty());
    }

    void testCase6() {
        const auto input = el::join(
            R"DOCFIX(**********************
Example API Guidelines
**********************

Primary Types
=============

.. code-block:: text

    Value // value)DOCFIX"_el,
            el::String::fromCharacter(U'\n', el::CpCount{492}));
        REQUIRE_FALSE(impl::ApiGuidelines{}.validate(input).isEmpty());
    }

    void testCase7() {
        const auto input = el::String{R"DOCFIX(**********************
Example API Guidelines
**********************

Primary Types
=============
.. code-block:: text
    Widget // one widget

This prose is not allowed.
)DOCFIX"_el};
        REQUIRE_FALSE(impl::ApiGuidelines{}.validate(input).isEmpty());
    }

    void testCase8() {
        const auto input = el::String{R"DOCFIX(**********************
Example API Guidelines
**********************

Primary Types
=============

.. code-block:: text

    Widget // one widget

Widget Patterns
===============

.. code-block:: text

    o.value() trailing() // invalid trailing call syntax
)DOCFIX"_el};
        REQUIRE_FALSE(impl::ApiGuidelines{}.validate(input).isEmpty());
    }
};
