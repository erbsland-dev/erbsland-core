// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "UcdDatasetFixture.hpp"

#include <erbsland/base_n/BaseNEncoder.hpp>
#include <erbsland/Hasher.hpp>
#include <erbsland/StringFormat.hpp>
#include <tool/cterm/BoxFrameData.hpp>
#include <tool/cterm/CommonBoxFrameStyleTool.hpp>
#include <tool/cterm/FrameBorderJointCharsTool.hpp>
#include <tool/unicode/UnicodeData.hpp>

TESTED_TARGETS(BoxFrameData CommonBoxFrameStyleTool FrameBorderJointCharsTool)
class BoxFrameToolTest final : public UNITTEST_SUBCLASS(UcdDatasetFixture) {
public:
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testSourceNamesAndSelection() {
        prepareDataset(
            "2500;BOX DRAWINGS LIGHT HORIZONTAL;So;0;ON;;;;;N;;;;;\n"
            "2502;BOX DRAWINGS LIGHT VERTICAL;So;0;ON;;;;;N;;;;;\n"
            "253C;BOX DRAWINGS LIGHT VERTICAL AND HORIZONTAL;So;0;ON;;;;;N;;;;;\n"
            "256D;BOX DRAWINGS LIGHT ARC DOWN AND RIGHT;So;0;ON;;;;;N;;;;;\n"_el);
        runInsideApplication([&] {
            const auto dataset =
                tool::unicode::UcdDataset{el::Path{"ucd"_el}, "17.0.0"_el, el::StringList{"UnicodeData.txt"_el}};
            const auto unicode = tool::unicode::UnicodeData{dataset};
            const auto frame = tool::cterm::BoxFrameData{{0x2500, 0x2502, 0x253C, 0x256D}, unicode};
            REQUIRE_EQUAL(frame.characters().size(), 4U);
            REQUIRE_EQUAL(frame.codePointForName("BOX DRAWINGS LIGHT ARC DOWN AND RIGHT"_el), char32_t{0x256D});
            REQUIRE_EQUAL(frame.combine(0x2500, 0x2502), char32_t{0x253C});
            REQUIRE_EQUAL(frame.best(frame.character(0x2500).attributes), char32_t{0x2500});
            REQUIRE_THROWS(frame.codePointForName("MISSING"_el));
        });
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testSourceOrderControlsTieAndMalformedInputsFail() {
        prepareDataset(
            "2500;BOX DRAWINGS LIGHT HORIZONTAL;So;0;ON;;;;;N;;;;;\n"
            "2501;BOX DRAWINGS LIGHT HORIZONTAL;So;0;ON;;;;;N;;;;;\n"_el);
        runInsideApplication([&] {
            const auto dataset =
                tool::unicode::UcdDataset{el::Path{"ucd"_el}, "17.0.0"_el, el::StringList{"UnicodeData.txt"_el}};
            const auto unicode = tool::unicode::UnicodeData{dataset};
            const auto first = tool::cterm::BoxFrameData{{0x2500, 0x2501}, unicode};
            const auto second = tool::cterm::BoxFrameData{{0x2501, 0x2500}, unicode};
            REQUIRE_EQUAL(first.best(first.character(0x2500).attributes), char32_t{0x2501});
            REQUIRE_EQUAL(second.best(second.character(0x2500).attributes), char32_t{0x2500});
            REQUIRE_THROWS((tool::cterm::BoxFrameData{{0x2500, 0x2500}, unicode}));
            REQUIRE_THROWS((tool::cterm::BoxFrameData{{0x253C}, unicode}));
        });
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testBothCommandsCheckAndSourceIntegrity() {
        const auto unicodeText = el::String{"0020;SPACE;Zs;0;WS;;;;;N;;;;;\n"
                                            "2500;BOX DRAWINGS LIGHT HORIZONTAL;So;0;ON;;;;;N;;;;;\n"
                                            "2502;BOX DRAWINGS LIGHT VERTICAL;So;0;ON;;;;;N;;;;;\n"
                                            "253C;BOX DRAWINGS LIGHT VERTICAL AND HORIZONTAL;So;0;ON;;;;;N;;;;;\n"
                                            "256D;BOX DRAWINGS LIGHT ARC DOWN AND RIGHT;So;0;ON;;;;;N;;;;;\n"
                                            "256E;BOX DRAWINGS LIGHT ARC DOWN AND LEFT;So;0;ON;;;;;N;;;;;\n"
                                            "256F;BOX DRAWINGS LIGHT ARC UP AND LEFT;So;0;ON;;;;;N;;;;;\n"
                                            "2570;BOX DRAWINGS LIGHT ARC UP AND RIGHT;So;0;ON;;;;;N;;;;;\n"_el};
        prepareDataset(
            unicodeText,
            {{"CompositionExclusions.txt"_el, "# empty\n"_el}, {"IdnaMappingTable.txt"_el, "0000..10FFFF;valid\n"_el}});
        const auto repertoire = "2500\n2502\n253C\n256D\n256E\n256F\n2570\n0020\n"_el;
        // Keep the manifest hash based on LF, and simulate a Windows CRLF checkout of the snapshot.
        write("box/characters.txt"_el, el::String{repertoire}.replacedAll("\n"_el, "\r\n"_el));
        auto hash = el::Hasher{el::HashAlgorithm::Sha3_256};
        hash.update(repertoire);
        const auto digest =
            el::text::base_n::BaseNEncoder{hash.finalize(), el::text::base_n::BaseNFormat::base16()}.toString();
        write("box/manifest.elcl"_el, el::StringFormat{"[files]\n\"characters.txt\": \"{}\"\n"_el}.build(digest));
        write("out/.keep"_el, ""_el);
        write(
            "tool/conf/erbsland-core-tool.elcl"_el,
            "[rendering]\n[unicode]\nucd_directory: \"ucd\"\nunicode_version: \"17.0.0\"\n"
            "[box_frame]\ninput_file: \"box/characters.txt\"\n"
            "[generate_common_box_frame_style]\noutput_file: \"out/common.cpp\"\n"
            "[generate_frame_border_joint_chars]\noutput_file: \"out/joints.hpp\"\n"_el);
        const auto common = [this](const el::StringList &options = {}) {
            return runTool<tool::cterm::CommonBoxFrameStyleTool>({}, {}, "generate-common-box-frame-style"_el, options);
        };
        const auto joints = [this](const el::StringList &options = {}) {
            return runTool<tool::cterm::FrameBorderJointCharsTool>(
                {}, {}, "generate-frame-border-joint-chars"_el, options);
        };
        REQUIRE(common(el::StringList{"--check"_el}) != 0);
        REQUIRE_FALSE((_root / "out/common.cpp"_el).content().readText().has_value());
        REQUIRE_EQUAL(common(), 0);
        REQUIRE_EQUAL(joints(), 0);
        REQUIRE_EQUAL(common(el::StringList{"--check"_el}), 0);
        REQUIRE_EQUAL(joints(el::StringList{"--check"_el}), 0);
        const auto generated = files().readText(el::Path{"out/joints.hpp"_el});
        write("out/joints.hpp"_el, "stale"_el);
        REQUIRE(joints(el::StringList{"--check"_el}) != 0);
        REQUIRE_EQUAL(files().readText(el::Path{"out/joints.hpp"_el}), "stale"_el);
        REQUIRE_EQUAL(joints(), 0);
        REQUIRE_EQUAL(files().readText(el::Path{"out/joints.hpp"_el}), generated);
        write("box/characters.txt"_el, "2500\n"_el);
        REQUIRE(common() != 0);
    }
};
