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
class DocumentationCleanupParity2Test final : public UNITTEST_SUBCLASS(ProjectFixture) {
public:
    void testCase5() {
        const auto input = el::String{R"DOCFIX(Use `StringEditor` there.
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
        REQUIRE_EQUAL(
            cleaner.process(input, warnings),
            R"DOCFIX(Use :cpp:type:`StringEditor <erbsland::text::StringEditor>` there.
)DOCFIX"_el);
    }

    void testCase6() {
        const auto input = el::String{
            R"DOCFIX(Use `StringEditor`, :cpp:class:`U8StringEditor`, :cpp:any:`format <erbsland::text::format>`, and :cpp:enum:`Text StringEditor <erbsland::text::StringEditor>`.
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
        REQUIRE_EQUAL(
            cleaner.process(input, warnings),
            R"DOCFIX(Use :cpp:type:`StringEditor <erbsland::text::StringEditor>`, :cpp:class:`U8StringEditor <erbsland::text::U8StringEditor>`, :cpp:func:`format <erbsland::text::format>`, and :cpp:type:`Text StringEditor <erbsland::text::StringEditor>`.
)DOCFIX"_el);
    }

    void testCase7() {
        const auto input = el::String{R"DOCFIX(Use `StringEditor` with a veryveryveryveryveryveryverylongword here.
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
        const auto cleaner = impl::RstCleaner{symbols, 30};
        REQUIRE_EQUAL(
            cleaner.process(input, warnings),
            R"DOCFIX(Use :cpp:type:`StringEditor <erbsland::text::StringEditor>` with a veryveryveryveryveryveryverylongword here.
)DOCFIX"_el);
    }

    void testCase8() {
        const auto input = el::String{R"DOCFIX(Use :cpp:any:`U8StringEditor::append` for appending text.
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
        REQUIRE_EQUAL(
            cleaner.process(input, warnings),
            R"DOCFIX(Use :cpp:func:`U8StringEditor::append <erbsland::text::U8StringEditor::append>` for
appending text.
)DOCFIX"_el);
    }
};
