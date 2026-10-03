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
class DocumentationCleanupParity3Test final : public UNITTEST_SUBCLASS(ProjectFixture) {
public:
    void testCase9() {
        const auto input = el::String{R"DOCFIX(Use `Missing` and `Mode` here.
)DOCFIX"_el};
        write("index.xml"_el, R"DOCFIX(<?xml version='1.0' encoding='UTF-8'?>
<doxygenindex>
  <compound kind="namespace"><name>erbsland::text</name>
    <member kind="typedef"><name>StringEditor</name></member>
    <member kind="enum"><name>StringEncoding</name></member>
    <member kind="function"><name>format</name></member>
    <member kind="function"><name>StringConverter</name></member>
    <member kind="function"><name>StringEncoder</name></member>
    <member kind="enum"><name>Mode</name></member>
  </compound>
  <compound kind="namespace"><name>erbsland::math</name>
    <member kind="enum"><name>Mode</name></member>
  </compound>
  <compound kind="class"><name>erbsland::text::U8StringEditor</name>
    <member kind="function"><name>append</name></member>
  </compound>
  <compound kind="class"><name>erbsland::text::StringConverter</name></compound>
  <compound kind="class"><name>erbsland::text::StringEncoder</name></compound>
  <compound kind="struct"><name>erbsland::unit::ByteUnit</name></compound>
  <compound kind="concept"><name>erbsland::math::IntegerType</name></compound>
  <compound kind="class"><name>erbsland::text::impl::PrivateThing</name></compound>
</doxygenindex>
)DOCFIX"_el);
        const auto files = tool::frame::ProjectFiles{_root};
        auto symbols = impl::DoxygenIndex{};
        symbols.read(files, _root / "index.xml"_el);
        auto warnings = el::StringList{};
        const auto cleaner = impl::RstCleaner{symbols, 88};
        REQUIRE_EQUAL(cleaner.process(input, warnings), R"DOCFIX(Use ``Missing`` and ``Mode`` here.
)DOCFIX"_el);
    }

    void testCase10() {
        const auto input = el::String{
            R"DOCFIX(This paragraph references `StringEditor` and ``std::string`` while it keeps words wrapped within the configured width. Another sentence follows with ordinary text.
)DOCFIX"_el};
        write("index.xml"_el, R"DOCFIX(<?xml version='1.0' encoding='UTF-8'?>
<doxygenindex>
  <compound kind="namespace"><name>erbsland::text</name>
    <member kind="typedef"><name>StringEditor</name></member>
    <member kind="enum"><name>StringEncoding</name></member>
    <member kind="function"><name>format</name></member>
    <member kind="function"><name>StringConverter</name></member>
    <member kind="function"><name>StringEncoder</name></member>
    <member kind="enum"><name>Mode</name></member>
  </compound>
  <compound kind="namespace"><name>erbsland::math</name>
    <member kind="enum"><name>Mode</name></member>
  </compound>
  <compound kind="class"><name>erbsland::text::U8StringEditor</name>
    <member kind="function"><name>append</name></member>
  </compound>
  <compound kind="class"><name>erbsland::text::StringConverter</name></compound>
  <compound kind="class"><name>erbsland::text::StringEncoder</name></compound>
  <compound kind="struct"><name>erbsland::unit::ByteUnit</name></compound>
  <compound kind="concept"><name>erbsland::math::IntegerType</name></compound>
  <compound kind="class"><name>erbsland::text::impl::PrivateThing</name></compound>
</doxygenindex>
)DOCFIX"_el);
        const auto files = tool::frame::ProjectFiles{_root};
        auto symbols = impl::DoxygenIndex{};
        symbols.read(files, _root / "index.xml"_el);
        auto warnings = el::StringList{};
        const auto cleaner = impl::RstCleaner{symbols, 74};
        REQUIRE_EQUAL(cleaner.process(input, warnings), R"DOCFIX(This paragraph references
:cpp:type:`StringEditor <erbsland::text::StringEditor>` and
``std::string`` while it keeps words wrapped within the configured width.
Another sentence follows with ordinary text.
)DOCFIX"_el);
    }

    void testCase11() {
        const auto input = el::String{
            R"DOCFIX(Use etc. and vs. as abbreviations inside the sentence. Initials like E. T. A. stay together. Numbers like 3.14, 2.0 and 1. number stay together. The final sentence still breaks.
)DOCFIX"_el};
        write("index.xml"_el, R"DOCFIX(<?xml version='1.0' encoding='UTF-8'?>
<doxygenindex>
  <compound kind="namespace"><name>erbsland::text</name>
    <member kind="typedef"><name>StringEditor</name></member>
    <member kind="enum"><name>StringEncoding</name></member>
    <member kind="function"><name>format</name></member>
    <member kind="function"><name>StringConverter</name></member>
    <member kind="function"><name>StringEncoder</name></member>
    <member kind="enum"><name>Mode</name></member>
  </compound>
  <compound kind="namespace"><name>erbsland::math</name>
    <member kind="enum"><name>Mode</name></member>
  </compound>
  <compound kind="class"><name>erbsland::text::U8StringEditor</name>
    <member kind="function"><name>append</name></member>
  </compound>
  <compound kind="class"><name>erbsland::text::StringConverter</name></compound>
  <compound kind="class"><name>erbsland::text::StringEncoder</name></compound>
  <compound kind="struct"><name>erbsland::unit::ByteUnit</name></compound>
  <compound kind="concept"><name>erbsland::math::IntegerType</name></compound>
  <compound kind="class"><name>erbsland::text::impl::PrivateThing</name></compound>
</doxygenindex>
)DOCFIX"_el);
        const auto files = tool::frame::ProjectFiles{_root};
        auto symbols = impl::DoxygenIndex{};
        symbols.read(files, _root / "index.xml"_el);
        auto warnings = el::StringList{};
        const auto cleaner = impl::RstCleaner{symbols, 240};
        REQUIRE_EQUAL(cleaner.process(input, warnings), R"DOCFIX(Use etc. and vs. as abbreviations inside the sentence.
Initials like E. T. A. stay together.
Numbers like 3.14, 2.0 and 1. number stay together.
The final sentence still breaks.
)DOCFIX"_el);
    }

    void testCase12() {
        const auto input = el::String{R"DOCFIX(.. code-block:: cpp

    auto value = `StringEditor`{};

* Use `StringEditor` in a bullet that must not be joined with anything else.

.. list-table::

    * - :cpp:type:`StringEditor`
)DOCFIX"_el};
        write("index.xml"_el, R"DOCFIX(<?xml version='1.0' encoding='UTF-8'?>
<doxygenindex>
  <compound kind="namespace"><name>erbsland::text</name>
    <member kind="typedef"><name>StringEditor</name></member>
    <member kind="enum"><name>StringEncoding</name></member>
    <member kind="function"><name>format</name></member>
    <member kind="function"><name>StringConverter</name></member>
    <member kind="function"><name>StringEncoder</name></member>
    <member kind="enum"><name>Mode</name></member>
  </compound>
  <compound kind="namespace"><name>erbsland::math</name>
    <member kind="enum"><name>Mode</name></member>
  </compound>
  <compound kind="class"><name>erbsland::text::U8StringEditor</name>
    <member kind="function"><name>append</name></member>
  </compound>
  <compound kind="class"><name>erbsland::text::StringConverter</name></compound>
  <compound kind="class"><name>erbsland::text::StringEncoder</name></compound>
  <compound kind="struct"><name>erbsland::unit::ByteUnit</name></compound>
  <compound kind="concept"><name>erbsland::math::IntegerType</name></compound>
  <compound kind="class"><name>erbsland::text::impl::PrivateThing</name></compound>
</doxygenindex>
)DOCFIX"_el);
        const auto files = tool::frame::ProjectFiles{_root};
        auto symbols = impl::DoxygenIndex{};
        symbols.read(files, _root / "index.xml"_el);
        auto warnings = el::StringList{};
        const auto cleaner = impl::RstCleaner{symbols, 88};
        REQUIRE_EQUAL(cleaner.process(input, warnings), R"DOCFIX(.. code-block:: cpp

    auto value = `StringEditor`{};

* Use :cpp:type:`StringEditor <erbsland::text::StringEditor>` in a bullet that must not be joined with anything else.

.. list-table::

    * - :cpp:type:`StringEditor <erbsland::text::StringEditor>`
)DOCFIX"_el);
    }
};
