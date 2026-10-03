// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ProjectFixture.hpp"

#include <erbsland/PathInfo.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringEncoder.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/zip/ArchiveWriter.hpp>
#include <tool/test_data/impl/VectorConverter.hpp>

TESTED_TARGETS(VectorConverter VectorSource)
class VectorArchiveTest final : public UNITTEST_SUBCLASS(ProjectFixture) {
public:
    void testSourceCatalogLight() {
        const auto &sources = tool::test_data::impl::VectorSource::all();
        REQUIRE_EQUAL(sources.count(), el::ItemCount{22});
        auto count = el::ItemCount{};
        for (const auto &source : sources) {
            count += source.outputs.count();
        }
        REQUIRE_EQUAL(count, el::ItemCount{60});
    }
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testGcmMatrixSelectionAndRejectedDimensions() {
        runInsideApplication([&] {
            auto input = el::StringEditor{"# GCM fixture\r\n[Keylen = 128]\r\n"_el};
            for (auto group = 0; group < 25; ++group) {
                input.append(
                    el::StringFormat{"[IVlen = 96]\r\n[PTlen = {}]\r\n[AADlen = {}]\r\n[Taglen = 128]\r\n"_el}.build(
                        group / 5 * 8, group % 5 * 8));
                for (auto record = 0; record < 15; ++record) {
                    input.append(el::StringFormat{"Count = {}\r\nKey = 00\r\n\r\n"_el}.build(record));
                }
            }
            input.append("[IVlen = 64]\r\n[Taglen = 128]\r\nCount = 0\r\n"_el);
            auto members = el::StringMap<el::String>{};
            for (const auto &path : source("gcm"_el).outputs) {
                members.set(el::Path{path}.name(), input);
            }
            const auto outputs = convert("gcm"_el, members);
            REQUIRE_EQUAL(outputs.count(), el::ItemCount{4});
            for (const auto &[path, text] : outputs) {
                const auto [records, groups] = tool::test_data::impl::VectorConverter::counts(path, text);
                REQUIRE_EQUAL(records, 375);
                REQUIRE_EQUAL(groups, 25);
                REQUIRE_FALSE(text.contains("[IVlen = 64]"_el));
                REQUIRE_FALSE(text.contains("\r"_el));
            }
            members.set("gcmDecrypt128.rsp"_el, "[Keylen = 128]\n"_el);
            REQUIRE_THROWS(convert("gcm"_el, members));
        });
    }
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testHmacAndMissingArchiveMembers() {
        runInsideApplication([&] {
            auto input = el::StringEditor{"# HMAC fixture\n[L=20]\nCount = 0\n\n[L=32]\n"_el};
            for (auto index = 0; index < 225; ++index) {
                input.append(el::StringFormat{"Count = {}\nKey = 00\n\n"_el}.build(index));
            }
            input.append("[L=48]\n"_el);
            for (auto index = 0; index < 300; ++index) {
                input.append(el::StringFormat{"Count = {}\nKey = 00\n\n"_el}.build(index));
            }
            input.append("[L=64]\nCount = 0\n"_el);
            const auto outputs = convert("hmac"_el, el::StringMap<el::String>{{"HMAC.rsp"_el, input}});
            const auto sha256 = outputs.get("hash/HMAC_SHA256.rsp"_el, el::String{});
            const auto sha384 = outputs.get("hash/HMAC_SHA384.rsp"_el, el::String{});
            REQUIRE_EQUAL(tool::test_data::impl::VectorConverter::counts("hash/HMAC_SHA256.rsp"_el, sha256).first, 225);
            REQUIRE_EQUAL(tool::test_data::impl::VectorConverter::counts("hash/HMAC_SHA384.rsp"_el, sha384).first, 300);
            REQUIRE_FALSE(sha256.contains("[L=48]"_el));
            REQUIRE_FALSE(sha384.contains("[L=32]"_el));
            REQUIRE_THROWS(convert("hmac"_el, el::StringMap<el::String>{{"wrong.rsp"_el, input}}));
        });
    }
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testNistCurveSelectionAndCanonicalDer() {
        runInsideApplication([&] {
            auto input = el::StringEditor{};
            for (const auto width : {64, 96}) {
                if (width == 64) {
                    input.append("[P-256,SHA-256]\n"_el);
                } else {
                    input.append("[P-384,SHA-384]\n"_el);
                }
                auto x = el::StringEditor{};
                auto y = el::StringEditor{};
                for (auto index = 0; index < width; ++index) {
                    x.append("1"_el);
                    y.append("2"_el);
                }
                for (auto index = 0; index < 15; ++index) {
                    input.append(
                        el::StringFormat{"Msg = 00\nQx = {}\nQy = {}\nR = 01\nS = 80\nResult = P (0 )\n\n"_el}.build(
                            x, y));
                }
            }
            input.append("[P-256,SHA-384]\nMsg = 00\nQx = 00\n\n"_el);
            const auto outputs =
                convert("ecdsa_nist"_el, el::StringMap<el::String>{{"186-4ecdsatestvectors/SigVer.rsp"_el, input}});
            const auto text = outputs.get("signature/ECDSA_NIST_SIGVER.rsp"_el, el::String{});
            REQUIRE_EQUAL(
                tool::test_data::impl::VectorConverter::counts("signature/ECDSA_NIST_SIGVER.rsp"_el, text).first, 30);
            REQUIRE(text.contains("Sig = 300702010102020080"_el));
            REQUIRE(text.contains("[AlgorithmDer = 300a06082a8648ce3d040302]"_el));
            REQUIRE(text.contains("[AlgorithmDer = 300a06082a8648ce3d040303]"_el));
        });
    }

private:
    auto source(const el::String &id) -> tool::test_data::impl::VectorSource {
        for (const auto &value : tool::test_data::impl::VectorSource::all()) {
            if (value.id == id) {
                return value;
            }
        }
        throw el::LogicFailure{"Missing test source."_el};
    }
    auto convert(const el::String &id, const el::StringMap<el::String> &members) -> el::StringMap<el::String> {
        const auto path = _root / "fixture.zip"_el;
        if (path.info().exists()) {
            path.operations().removeOrThrow();
        }
        const auto archive = el::zip::ArchiveWriter::create(path);
        for (const auto &[name, text] : members) {
            archive->addData(el::StringEncoder{text}.encode(el::StringEncoding::Utf8), el::Path{name});
        }
        archive->finalize();
        auto converter = tool::test_data::impl::VectorConverter{
            tool::frame::app().createRenderEnvironment(tool::frame::TemplateLanguage::Text), source(id), ""_el};
        return converter.convert(files().readData(path));
    }
};
