// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../ConfTestHelper.hpp"

#include <erbsland/conf/impl/source/FileSource.hpp>
#include <erbsland/conf/impl/source/StringSource.hpp>
#include <erbsland/conf/Source.hpp>
#include <erbsland/conf/StdFormat.hpp>
#include <erbsland/path/Path.hpp>
#include <erbsland/text/String.hpp>

using namespace el::conf;
using namespace el::text::literals;
using namespace std::filesystem;

TESTED_TARGETS(Source)
class SourceCreateTest final : public UNITTEST_SUBCLASS(ConfTestHelper) {
public:
    void testFromFileWithPath() {
        auto filePath = createTestFile("test"_el);
        auto source = Source::fromFile(el::path::Path{filePath});
        REQUIRE(source != nullptr);
        REQUIRE_EQUAL(source->name(), "file"_el);
        REQUIRE_EQUAL(source->path(), el::path::Path{filePath}.toString());
        auto expectedIdentifier = el::text::StringEditor{"file:"_el};
        expectedIdentifier.append(source->path().toSafeString(el::unit::CpLength{200}));
        REQUIRE_EQUAL(source->identifier()->toText(), el::text::String{expectedIdentifier});
        REQUIRE_FALSE(source->isOpen());
        REQUIRE(dynamic_cast<impl::FileSource *>(source.get()) != nullptr);
    }

    void testFromStringWithCoreString() {
        auto source = Source::fromString(el::text::String{"abc"});
        REQUIRE(source != nullptr);
        REQUIRE(source->name() == "text"_el);
        REQUIRE(source->path().isEmpty());
        REQUIRE(source->identifier()->toText() == "text"_el);
        REQUIRE_FALSE(source->isOpen());
        REQUIRE(dynamic_cast<impl::StringSource *>(source.get()) != nullptr);
    }
};
