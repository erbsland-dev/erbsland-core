// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ProjectFixture.hpp"

#include <erbsland/json/JsonValue.hpp>
#include <tool/test_data/impl/VectorConverter.hpp>

/// Store synthetic upstream JSON documents and exercise the native conversion pipeline.
class VectorJsonFixture : public ProjectFixture {
protected:
    using Json = el::json::JsonValue;
    using Object = el::json::JsonObject;
    using Array = el::json::JsonArray;

protected:
    /// Construct common fields for a synthetic signature test record.
    auto record(const int index, const el::String &result) -> Json {
        return Object{
            {"tcId"_el, index},
            {"result"_el, result},
            {"flags"_el, Array{Json{"Synthetic"_el}}},
            {"msg"_el, ""_el},
            {"sig"_el, "3001"_el}};
    }
    /// Store a synthetic source and convert it using its catalog entry.
    auto convert(const el::String &id, const Json &json) -> el::StringMap<el::String> {
        // JSON is the stored source format, not an intermediate representation for rendering or hashing.
        write("input.json"_el, json.toString());
        for (const auto &source : tool::test_data::impl::VectorSource::all()) {
            if (source.id == id) {
                auto converter = tool::test_data::impl::VectorConverter{
                    tool::frame::app().createRenderEnvironment(tool::frame::TemplateLanguage::Text),
                    source,
                    "0123456789012345678901234567890123456789"_el};
                return converter.convert(files().readData(el::Path{"input.json"_el}));
            }
        }
        throw el::LogicFailure{"Missing test source."_el};
    }
};
