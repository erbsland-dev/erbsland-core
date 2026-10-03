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
class DocumentationGuidelinesParity3Test final : public UNITTEST_SUBCLASS(ProjectFixture) {
public:
    void testCase9() {
        const auto input = el::String{R"DOCFIX(======================
Example API Guidelines
======================

Example Patterns
================

.. code-block:: text

    o.value() // access the value

Primary Types
=============

.. code-block:: text

    Widget // one widget
)DOCFIX"_el};
        REQUIRE_FALSE(impl::ApiGuidelines{}.validate(input).isEmpty());
    }

    void testCase10() {
        const auto input = el::String{R"DOCFIX(**********************
Example API Guidelines
**********************

Value Types
===========

.. code-block:: text

    Value // one value

Helper Types
============

.. code-block:: text

    Helper // one helper
)DOCFIX"_el};
        REQUIRE_FALSE(impl::ApiGuidelines{}.validate(input).isEmpty());
    }

    void testCase11() {
        const auto input = el::String{R"DOCFIX(**********************
Example API Guidelines
**********************

Primary Types
=============

.. code-block:: text

    Widget // one widget

Pattern Definitions
===================

.. code-block:: text

    value = Widget // invalid shortcut
    V = Widget | Other // invalid type separator
)DOCFIX"_el};
        REQUIRE_FALSE(impl::ApiGuidelines{}.validate(input).isEmpty());
    }

    void testCase12() {
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

    widget.value() // invalid object spelling
    o.value() -> Widget/Other // multiple return types
    o.value() -> Widget -> Other // two arrows
)DOCFIX"_el};
        REQUIRE_FALSE(impl::ApiGuidelines{}.validate(input).isEmpty());
    }
};
