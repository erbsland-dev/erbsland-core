// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../path/PathTestFixture.hpp"

#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/path/PathContent.hpp>
#include <erbsland/path/PathCreateMode.hpp>
#include <erbsland/path/PathOperations.hpp>
#include <erbsland/path/PathWriteDataOptions.hpp>
#include <erbsland/path/PathWriteTextOptions.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/render/Environment.hpp>
#include <erbsland/text/render/FileSystemLoader.hpp>
#include <erbsland/text/render/RenderError.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <vector>

using namespace el::text::literals;
using namespace el::text::render;
using erbsland::test::pathtest::PathTestFixture;

TESTED_TARGETS(FileSystemLoader FileSystemLoaderOptions)
class FileSystemLoaderTest final : public el::UnitTest {
public:
    void testLoadAndRevision() {
        const auto fixture = PathTestFixture{"render-loader"};
        fixture.child("pages").operations().createDirectoryOrThrow();
        fixture.child("pages/hello.txt").content().writeTextOrThrow("Hello {{ name }}"_el);
        const auto loader = FileSystemLoader::create(fixture.path());

        const auto source = loader->load("pages/hello.txt"_el);
        REQUIRE(source.has_value());
        REQUIRE_EQUAL(source->text(), "Hello {{ name }}"_el);
        REQUIRE_FALSE(source->origin().isEmpty());
        REQUIRE_FALSE(source->revision().isEmpty());
        REQUIRE_FALSE(loader->load("missing.txt"_el).has_value());

        const auto environment = Environment::create();
        environment->addLayoutLoader(loader);
        REQUIRE_EQUAL(environment->render("pages/hello.txt"_el, Context{}.set("name"_el, "Ada"_el)), "Hello Ada"_el);
    }

    void testAutoReloadAndInvalidUtf8() {
        const auto fixture = PathTestFixture{"render-reload"};
        const auto page = fixture.child("page.txt");
        page.content().writeTextOrThrow("old"_el);
        const auto environment = Environment::create();
        environment->addLayoutLoader(FileSystemLoader::create(fixture.path()));
        environment->enableAutoReload();

        REQUIRE_EQUAL(environment->render("page.txt"_el), "old"_el);
        auto writeOptions = el::path::PathWriteTextOptions{};
        writeOptions.setCreationMode(el::path::PathCreateMode::CreateOrOverwrite);
        page.content().writeTextOrThrow("new"_el, writeOptions);
        REQUIRE_EQUAL(environment->render("page.txt"_el), "new"_el);

        auto dataOptions = el::path::PathWriteDataOptions{};
        dataOptions.setCreationMode(el::path::PathCreateMode::CreateOrOverwrite);
        page.content().writeDataOrThrow(el::mem::ByteBlock::fromVector(std::vector<uint8_t>{0xffU}), dataOptions);
        REQUIRE_THROWS_AS(RenderError, environment->render("page.txt"_el));
    }
};
