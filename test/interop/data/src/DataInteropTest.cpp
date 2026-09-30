// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/data/bson/BsonValue.hpp>
#include <erbsland/data/cbor/CborValue.hpp>
#include <erbsland/data/xml/XmlDocument.hpp>
#include <erbsland/path/Path.hpp>
#include <erbsland/path/PathContent.hpp>
#include <erbsland/path/PathOperations.hpp>
#include <erbsland/path/TempDirectory.hpp>
#include <erbsland/system/Subprocess.hpp>
#include <erbsland/system/SubprocessOptions.hpp>
#include <erbsland/system/SubprocessOutputMode.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/StringList.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstdint>
#include <vector>

using namespace el::text::literals;

TESTED_TARGETS(BsonValue CborValue XmlDocument)
class DataInteropTest final : public el::UnitTest {
private:
    [[nodiscard]] static auto counterpartPath() -> el::path::Path {
        auto path = el::path::Path{el::unittest::fh::unitTestExecutablePath()}.parent();
#if defined(_WIN32)
        path /= "erbsland-core-data-interop-counterpart.exe"_el;
#else
        path /= "erbsland-core-data-interop-counterpart"_el;
#endif
        return path;
    }

    [[nodiscard]] static auto nativePathText(const el::path::Path &path) -> el::text::String {
        return el::text::StringConverter{path.toStdPath().string()}.toString();
    }

    [[nodiscard]] static auto vectorPath(const char *filename) -> el::path::Path {
        return el::path::Path{el::unittest::fh::resolveDataPath(filename)};
    }

    void verifyWithRust(const el::path::Path &directory) {
        auto options = el::system::SubprocessOptions{};
        options.setInheritStandardInput(false)
            .setStandardOutputMode(el::system::SubprocessOutputMode::Discard)
            .setStandardErrorMode(el::system::SubprocessOutputMode::Capture);
        auto process = el::system::Subprocess::start(
            counterpartPath(), el::text::StringList{"verify"_el, nativePathText(directory)}, options);
        REQUIRE(process.wait().isSuccess());
    }

public:
    void testRustToCore() {
        const auto bson = el::data::bson::BsonValue::fromByteBlockOrThrow(
            vectorPath("vectors/rust-bson.bin").content().readDataOrThrow());
        REQUIRE_EQUAL(bson.getOrThrow("ritim"_el).getText().value(), "aksak"_el);
        REQUIRE_EQUAL(bson.getOrThrow("bpm"_el).getInteger().value(), int64_t{96});
        REQUIRE_EQUAL(bson.getOrThrow("vurgu"_el).getBinary()->bytes,
            el::mem::ByteBlock::fromVector(std::vector<uint8_t>{0, 2, 4}));

        const auto cbor = el::data::cbor::CborValue::fromByteBlockOrThrow(
            vectorPath("vectors/rust-cbor.bin").content().readDataOrThrow());
        REQUIRE_EQUAL(cbor.getOrThrow("ritim"_el).getText().value(), "aksak"_el);
        REQUIRE_EQUAL(cbor.getOrThrow("bpm"_el).getSigned().value(), int64_t{96});

        const auto xmlText = el::text::StringConverter{
            vectorPath("vectors/rust-xml.xml").content().readTextOrThrow()}.toString();
        const auto xml = el::data::xml::XmlDocument::fromStringOrThrow(xmlText);
        REQUIRE_EQUAL(xml.root()->name(), "ritimler"_el);
        REQUIRE_EQUAL(xml.root()->children().get(el::unit::ItemIndex{})->getAttribute("bpm"_el).value(), "96"_el);
    }

    void testCoreToRust() {
        const auto directory = el::path::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
        auto bsonMap = el::data::bson::BsonDocument{};
        bsonMap.set("ritim"_el, el::data::bson::BsonValue{"aksak"_el});
        bsonMap.set("bpm"_el, el::data::bson::BsonValue{int32_t{96}});
        bsonMap.set("etkin"_el, el::data::bson::BsonValue{true});
        bsonMap.set("vurgu"_el, el::data::bson::BsonValue{
            el::mem::ByteBlock::fromVector(std::vector<uint8_t>{0, 2, 4})});
        (directory->path() / "core-bson.bin"_el).content().writeDataOrThrow(
            el::data::bson::BsonValue{std::move(bsonMap)}.toByteBlock());

        auto cborMap = el::data::cbor::CborMap{};
        cborMap.set("ritim"_el, el::data::cbor::CborValue{"aksak"_el});
        cborMap.set("bpm"_el, el::data::cbor::CborValue{int64_t{96}});
        cborMap.set("vurgu"_el, el::data::cbor::CborValue{
            el::mem::ByteBlock::fromVector(std::vector<uint8_t>{0, 2, 4})});
        (directory->path() / "core-cbor.bin"_el).content().writeDataOrThrow(
            el::data::cbor::CborValue{std::move(cborMap)}.toByteBlock());

        auto xml = el::data::xml::XmlDocument{};
        auto root = xml.addRoot("ritimler"_el);
        auto rhythm = root->addElement("ritim"_el, "aksak"_el);
        rhythm->setAttribute("bpm"_el, "96"_el);
        rhythm->addElement("vurgu"_el, "2"_el);
        (directory->path() / "core-xml.xml"_el).content().writeTextOrThrow(xml.toString());
        verifyWithRust(directory->path());
    }
};
