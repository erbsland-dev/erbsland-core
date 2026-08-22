// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpRouteHandler.hpp"

#include "../../../../text/String.hpp"
#include "../../../http/HttpMethod.hpp"
#include "../../../http/HttpMethodType.hpp"

#include <optional>
#include <utility>
#include <vector>

namespace erbsland::network::impl {

/// Ordered Flask-like HTTP route registrations.
/// @tested{HttpRoutesTest}
class HttpRoutes final {
public:
    /// One captured route parameter name and value.
    using Parameter = std::pair<text::String, text::String>;
    /// Registration-ordered captured parameters.
    using Parameters = std::vector<Parameter>;

    /// Result of method and decoded-segment matching.
    struct Match final {
        HttpRouteHandler handler;        ///< First selected handler.
        Parameters parameters;           ///< Captured route parameters.
        std::vector<HttpMethod> allowed; ///< Methods for path-only matches.
        bool pathMatched{};              ///< Whether any registered path matched.
        bool usedHeadFallback{};         ///< Whether HEAD selected a GET route.
    };

private:
    /// Parsed pattern-segment category.
    enum class SegmentKind : uint8_t {
        Literal,   ///< Exact decoded text.
        Parameter, ///< One nonempty complete segment.
        CatchAll,  ///< Every remaining segment.
    };

    /// One parsed pattern segment.
    struct Segment final {
        SegmentKind kind{SegmentKind::Literal}; ///< Segment interpretation.
        text::String text;                      ///< Literal or parameter name.
    };

    /// One additive route registration.
    struct Entry final {
        std::optional<std::vector<Segment>> pattern;    ///< No pattern denotes the fallback.
        std::optional<std::vector<HttpMethod>> methods; ///< No methods denotes every method.
        HttpRouteHandler handler;                       ///< Route handler.
    };

public:
    /// Add an every-method patterned route.
    void add(text::String pattern, HttpRouteHandler handler);
    /// Add a one-method patterned route.
    void add(HttpMethod method, text::String pattern, HttpRouteHandler handler);
    /// Add a standard-method-set patterned route.
    void add(HttpMethodTypes methods, text::String pattern, HttpRouteHandler handler);
    /// Add a scope fallback route.
    void addFallback(HttpRouteHandler handler);
    /// Find the first registration-ordered method and path match.
    [[nodiscard]] auto match(const HttpMethod &method, const std::vector<text::String> &segments) const -> Match;

private:
    /// Parse and validate one brace-pattern path.
    [[nodiscard]] static auto parsePattern(text::String pattern) -> std::vector<Segment>;
    /// Split one normalized pattern on literal slashes.
    [[nodiscard]] static auto splitPath(const text::String &path) -> std::vector<text::String>;
    /// Expand standard method flags into exact method values.
    [[nodiscard]] static auto methodsFromFlags(HttpMethodTypes methods) -> std::vector<HttpMethod>;
    /// Test exact method selection or HEAD-to-GET fallback.
    [[nodiscard]] static auto methodMatches(const Entry &entry, const HttpMethod &method, bool getFallback) -> bool;
    /// Test decoded segments and capture complete parameters.
    [[nodiscard]] static auto pathMatches(
        const Entry &entry, const std::vector<text::String> &segments, Parameters &parameters) -> bool;
    /// Add unique permitted methods, including implicit HEAD.
    static void appendAllowed(const Entry &entry, std::vector<HttpMethod> &allowed);
    /// Validate automatic-body bounds and configured media patterns.
    static void validateHandler(const HttpRouteHandler &handler);
    /// Validate one supported media pattern.
    static void validateContentTypePattern(const text::String &pattern);

private:
    std::vector<Entry> _entries; ///< Registration-ordered routes and fallbacks.
};

}
