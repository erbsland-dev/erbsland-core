// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../core/ApplicationTestScope.hpp"
#include "../../path/PathTestFixture.hpp"

#include <erbsland/err/ParameterError.hpp>
#include <erbsland/path/PathContent.hpp>
#include <erbsland/path/PathOperations.hpp>
#include <erbsland/resource/ResourceError.hpp>
#include <erbsland/resource/Resources.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/render/Environment.hpp>
#include <erbsland/text/render/FileSystemLoader.hpp>
#include <erbsland/text/render/RenderError.hpp>
#include <erbsland/text/render/ResourceLoader.hpp>
#include <erbsland/text/StringMap.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <thread>
#include <vector>

using namespace el::text::literals;
using namespace el::text::render;
using erbsland::test::pathtest::PathTestFixture;
namespace th = erbsland::unittest::th;

/// Immutable in-memory resource provider used by loader tests.
/// @notest{Test utility.}
class ResourceLoaderMemoryResources final : public el::resource::Resources {
public:
    /// Add one logical text resource before concurrent use.
    auto set(const el::text::String &identifier, const el::text::String &path, el::text::String value)
        -> ResourceLoaderMemoryResources & {
        _texts.set(key(identifier, path), std::move(value));
        return *this;
    }
    /// Mark one present resource as invalid before concurrent use.
    auto setInvalid(const el::text::String &identifier, const el::text::String &path)
        -> ResourceLoaderMemoryResources & {
        _invalid.set(key(identifier, path), true);
        return *this;
    }

public: // implement Resources
    [[nodiscard]] auto contains(const el::text::String &identifier, const el::text::String &path) const
        -> bool override {
        return _texts.contains(key(identifier, path)) || _invalid.contains(key(identifier, path));
    }
    [[nodiscard]] auto getStoredData(
        [[maybe_unused]] const el::text::String &identifier, [[maybe_unused]] const el::text::String &path) const
        -> std::optional<el::mem::ConstByteSpan> override {
        return {};
    }
    [[nodiscard]] auto getStoredDataOrThrow(
        [[maybe_unused]] const el::text::String &identifier, [[maybe_unused]] const el::text::String &path) const
        -> el::mem::ConstByteSpan override {
        throw el::resource::ResourceError{
            el::resource::ResourceErrorCategory::InvalidData, "Stored data is unavailable in this test provider."_el};
    }
    [[nodiscard]] auto getData(
        [[maybe_unused]] const el::text::String &identifier, [[maybe_unused]] const el::text::String &path) const
        -> std::optional<el::mem::ByteBlock> override {
        return {};
    }
    [[nodiscard]] auto getDataOrThrow(
        [[maybe_unused]] const el::text::String &identifier, [[maybe_unused]] const el::text::String &path) const
        -> el::mem::ByteBlock override {
        throw el::resource::ResourceError{
            el::resource::ResourceErrorCategory::InvalidData, "Binary data is unavailable in this test provider."_el};
    }
    [[nodiscard]] auto getText(const el::text::String &identifier, const el::text::String &path) const
        -> std::optional<el::text::String> override {
        if (_invalid.contains(key(identifier, path))) {
            return {};
        }
        return _texts.get(key(identifier, path));
    }
    [[nodiscard]] auto getTextOrThrow(const el::text::String &identifier, const el::text::String &path) const
        -> el::text::String override {
        if (_invalid.contains(key(identifier, path))) {
            throw el::resource::ResourceError{
                el::resource::ResourceErrorCategory::InvalidData, "The test resource is invalid."_el};
        }
        const auto result = _texts.get(key(identifier, path));
        if (!result.has_value()) {
            throw el::resource::ResourceError{
                el::resource::ResourceErrorCategory::NotFound, "The test resource is missing."_el};
        }
        return *result;
    }
    [[nodiscard]] auto getInfo(const el::text::String &identifier, const el::text::String &path) const
        -> std::optional<el::resource::ResourceInfo> override {
        const auto resourceKey = key(identifier, path);
        const auto text = _texts.get(resourceKey);
        if (!text.has_value() && !_invalid.contains(resourceKey)) {
            return {};
        }
        const auto length = text.has_value() ? text->length() : el::unit::ByteLength{};
        return el::resource::ResourceInfo{length, length, {}, {}, {}, false};
    }
    [[nodiscard]] auto getInfoOrThrow(const el::text::String &identifier, const el::text::String &path) const
        -> el::resource::ResourceInfo override {
        const auto result = getInfo(identifier, path);
        if (!result.has_value()) {
            throw el::resource::ResourceError{
                el::resource::ResourceErrorCategory::NotFound, "The test resource is missing."_el};
        }
        return *result;
    }

private:
    [[nodiscard]] static auto key(const el::text::String &identifier, const el::text::String &path)
        -> el::text::String {
        return el::text::StringList{identifier, ":"_el, path}.join();
    }

private:
    el::text::StringMap<el::text::String> _texts; ///< Exact resource text by combined test key.
    el::text::StringMap<bool> _invalid;           ///< Present resources that fail decoding.
};

TESTED_TARGETS(ResourceLoader)
class ResourceLoaderTest final : public el::UnitTest {
public:
    void testPrefixMappingOriginAndRetention() {
        auto resources = std::make_shared<ResourceLoaderMemoryResources>();
        resources->set("layouts"_el, "builtin/email/body.html"_el, "Hello {{ name }}"_el);
        resources->set("layouts"_el, "builtin/empty"_el, {});
        const auto weakResources = std::weak_ptr<ResourceLoaderMemoryResources>{resources};
        const auto loader = ResourceLoader::create(resources, "layouts"_el, "builtin"_el);
        resources.reset();

        const auto source = loader->load("email/body.html"_el);
        REQUIRE(source.has_value());
        REQUIRE_EQUAL(source->text(), "Hello {{ name }}"_el);
        REQUIRE_EQUAL(source->origin(), "resource:layouts/builtin/email/body.html"_el);
        REQUIRE_FALSE(source->revision().isEmpty());
        REQUIRE_EQUAL(loader->load("email/body.html"_el)->revision(), source->revision());
        REQUIRE(loader->load("empty"_el).has_value());
        REQUIRE(loader->load("empty"_el)->text().isEmpty());
        REQUIRE_FALSE(loader->load("missing"_el).has_value());
        REQUIRE_FALSE(weakResources.expired());

        const auto environment = Environment::create();
        environment->addLayoutLoader(loader);
        REQUIRE_EQUAL(environment->render("email/body.html"_el, Context{}.set("name"_el, "Ada"_el)), "Hello Ada"_el);
    }

    void testApplicationResourcesAndValidation() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto loader = ResourceLoader::create(el::resource::ResourcesConstPtr{}, "test"_el);
        const auto source = loader->load("plain.txt"_el);
        REQUIRE(source.has_value());
        REQUIRE_EQUAL(source->text(), "hello"_el);

        REQUIRE_THROWS_AS(el::err::ParameterError, ResourceLoader::create(""_el));
        REQUIRE_THROWS_AS(el::err::ParameterError, ResourceLoader::create("bad/id"_el));
        REQUIRE_THROWS_AS(el::err::ParameterError, ResourceLoader::create("test"_el, "/absolute"_el));
        REQUIRE_THROWS_AS(el::err::ParameterError, ResourceLoader::create("test"_el, "bad//prefix"_el));
        REQUIRE_THROWS_AS(el::err::ParameterError, ResourceLoader::create("test"_el, "bad/../prefix"_el));
    }

    void testInvalidPresentResourceIsLoadFailure() {
        auto resources = std::make_shared<ResourceLoaderMemoryResources>();
        resources->setInvalid("layouts"_el, "bad"_el);
        resources->set("layouts"_el, "invalid-utf8"_el, el::text::String{th::stdStringFromHex("41 C0 42")});
        const auto environment = Environment::create();
        environment->addLayoutLoader(ResourceLoader::create(resources, "layouts"_el));
        try {
            static_cast<void>(environment->render("bad"_el));
            REQUIRE(false);
        } catch (const RenderError &error) {
            REQUIRE_EQUAL(error.context().category(), RenderErrorCategory::Load);
        }
        REQUIRE_THROWS_AS(RenderError, environment->render("invalid-utf8"_el));
    }

    void testFilesystemOverrideOfResourceInclude() {
        auto resources = std::make_shared<ResourceLoaderMemoryResources>();
        resources->set("layouts"_el, "page"_el, "A{% include \"partials/header\" %}B"_el);
        resources->set("layouts"_el, "partials/header"_el, "built-in"_el);

        const auto fixture = PathTestFixture{"render-resource-override"};
        fixture.child("partials").operations().createDirectoryOrThrow();
        fixture.child("partials/header").content().writeTextOrThrow("external"_el);
        const auto environment = Environment::create();
        environment->addLayoutLoader(ResourceLoader::create(resources, "layouts"_el));
        environment->addLayoutLoader(FileSystemLoader::create(fixture.path()), 10);

        REQUIRE_EQUAL(environment->render("page"_el), "AexternalB"_el);
    }

    void testConcurrentReads() {
        auto resources = std::make_shared<ResourceLoaderMemoryResources>();
        resources->set("layouts"_el, "page"_el, "Hello"_el);
        const auto loader = ResourceLoader::create(resources, "layouts"_el);
        auto valid = std::array<bool, 8U>{};
        auto threads = std::vector<std::thread>{};
        for (auto index = std::size_t{}; index < valid.size(); ++index) {
            threads.emplace_back([&loader, &valid, index]() -> void {
                const auto source = loader->load("page"_el);
                valid[index] = source.has_value() && source->text() == "Hello"_el;
            });
        }
        for (auto &thread : threads) {
            thread.join();
        }
        for (const auto result : valid) {
            REQUIRE(result);
        }
    }
};
