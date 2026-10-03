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
class DocumentationGuidelinesParity1Test final : public UNITTEST_SUBCLASS(ProjectFixture) {
public:
    void testCase1() {
        const auto input = el::String{R"DOCFIX(**********************
Example API Guidelines
**********************

Core Semantics
==============

Vocabulary
----------

``value``
    One immutable value.

Primary Types
=============

.. code-block:: text

    Widget, WidgetEditor // immutable value and mutable editor
    Value❮Kind❯ // value selected by kind
    ❮domain❯::❮Type❯Error // domain-specific error pattern

Secondary Types
===============

.. code-block:: text

    WidgetError // widget operation error

Pattern Definitions
===================

.. code-block:: text

    V, Vp = Value/Value❮Kind❯ // value or kind-specific value

Widget Patterns
===============

.. code-block:: text

    T(value) // create a widget
    T::fromValue(value) -> T // create from a value
    o.value() -> V // access the value
    o.get❮Type❯OrThrow() -> V // access a typed value or throw
    o.❮operation❯() // apply a named operation
    o.first()/last() -> V // access a boundary value
    o.enable/disable() // change a state
    makeWidget([value]) -> Widget // create a default or initialized widget
)DOCFIX"_el};
        REQUIRE(impl::ApiGuidelines{}.validate(input).isEmpty());
    }

    void testCase2() {
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

    o.value() -> [Widget] // access an optional value
    o.values() -> Map<Key, List<Value>> // access nested values
)DOCFIX"_el};
        REQUIRE(impl::ApiGuidelines{}.validate(input).isEmpty());
    }

    void testCase3() {
        const auto input = el::String{R"DOCFIX(**********************
Example API Guidelines
**********************

Core Semantics
==============

line 0
line 1
line 2
line 3
line 4
line 5
line 6
line 7
line 8
line 9
line 10
line 11
line 12
line 13
line 14
line 15
line 16
line 17
line 18
line 19
line 20
line 21
line 22
line 23
line 24
line 25
line 26
line 27
line 28
line 29
line 30
line 31
line 32
line 33
line 34
line 35
line 36
line 37
line 38
line 39
line 40
line 41
line 42
line 43
line 44
line 45
line 46
line 47
line 48
line 49
line 50
line 51
line 52
line 53
line 54
line 55
line 56
line 57
line 58
line 59
line 60

Part 0
------


Part 1
------


Part 2
------


Part 3
------


Part 4
------

Primary Types
=============

.. code-block:: text

    Widget // one widget
)DOCFIX"_el};
        REQUIRE_FALSE(impl::ApiGuidelines{}.validate(input).isEmpty());
    }

    void testCase4() {
        const auto input = el::String{R"DOCFIX(**********************
Example API Guidelines
**********************

This introduction is not allowed.

Primary Types
=============

.. code-block:: text

    Widget // one widget
)DOCFIX"_el};
        REQUIRE_FALSE(impl::ApiGuidelines{}.validate(input).isEmpty());
    }
};
