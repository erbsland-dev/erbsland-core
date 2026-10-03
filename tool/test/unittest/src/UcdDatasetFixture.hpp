// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ProjectFixture.hpp"

#include <erbsland/base_n/BaseNEncoder.hpp>
#include <erbsland/Hasher.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/StringJoins.hpp>
#include <tool/unicode/UcdDataset.hpp>

/// Construct small, integrity-checked UCD inputs for failure tests.
/// @notest{Unit-test fixture.}
class UcdDatasetFixture : public ProjectFixture {
protected: // inputs
    void prepareDataset(
        const el::String &unicodeData = "0061;LATIN SMALL LETTER A;Ll;0;L;;;;;N;;;;;\n"_el,
        const el::StringMap<el::String> &overrides = {},
        const el::String &sourceVersion = "17.0.0"_el,
        const el::String &manifestVersion = "17.0.0"_el) {
        auto texts = el::StringMap<el::String>{};
        texts.set("UnicodeData.txt"_el, unicodeData);
        texts.set("CaseFolding.txt"_el, "# empty case mappings\n"_el);
        texts.set("EastAsianWidth.txt"_el, "0041;Na\n"_el);
        texts.set("ReadMe.txt"_el, el::StringFormat{"Version {} of the Unicode Standard.\n"_el}.build(sourceVersion));
        texts.set(
            "Blocks.txt"_el,
            "20D0..20FF;Combining Diacritical Marks for Symbols\n1D100..1D1FF;Musical Symbols\n1D200..1D24F;Ancient Greek Musical Notation\n"_el);
        texts.set("DerivedCoreProperties.txt"_el, "200B;Default_Ignorable_Code_Point\n"_el);
        texts.set("DerivedNormalizationProps.txt"_el, "0041;Changes_When_NFKC_Casefolded\n"_el);
        texts.set("DerivedJoiningType.txt"_el, "0300;T\n"_el);
        texts.set("HangulSyllableType.txt"_el, "1100;L\n1161;V\n11A8;T\n"_el);
        texts.set("PropList.txt"_el, "0020;White_Space\nFDD0;Noncharacter_Code_Point\n"_el);
        texts.set("Scripts.txt"_el, "0061;Latin\n"_el);
        auto manifest = el::StringEditor{
            el::StringFormat{"[dataset]\nunicode_version: \"{}\"\n[files]\n"_el}.build(manifestVersion)};
        auto names = tool::unicode::UcdDataset::filenames();
        names.append("CaseFolding.txt"_el);
        names.append("EastAsianWidth.txt"_el);
        names.append("ReadMe.txt"_el);
        for (const auto &[name, body] : overrides) {
            if (!names.contains(name)) {
                names.append(name);
            }
        }
        for (const auto &name : names) {
            auto body = texts.get(name, el::String{});
            if (overrides.contains(name)) {
                body = overrides.get(name, el::String{});
            }
            const auto text = el::StringFormat{"# {}-{}.txt\n{}"_el}.build(name, sourceVersion, body);
            write(el::join("ucd/"_el, name), text);
            auto hash = el::Hasher{el::HashAlgorithm::Sha3_256};
            hash.update(text);
            const auto checksum =
                el::text::base_n::BaseNEncoder{hash.finalize(), el::text::base_n::BaseNFormat::base16()}.toString();
            manifest.append(el::StringFormat{"\"{}\": \"{}\"\n"_el}.build(name, checksum));
        }
        write("ucd/manifest.elcl"_el, manifest);
    }
};
