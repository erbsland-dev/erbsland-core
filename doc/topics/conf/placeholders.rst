..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Configuration; Placeholders
    single: Placeholders; Configuration Text
    single: Configuration; Runtime Values

****************************
Expanding Configuration Text
****************************

Configuration files often need a small amount of information that belongs to the running application rather than the
file itself: a deployment name, a path selected at startup, or a value held in a protected store.
Placeholders let your application provide that text while the parser reads a document.
You choose the available sources and transformations, and ordinary ELCL parsing resumes after the resulting text has
been inserted.

Placeholder expansion is an optional parser extension.
Until at least one source is registered, placeholder-looking text remains literal text and has no special meaning.

Reading Placeholder Expressions
===============================

A placeholder begins with ``${`` and ends with ``}``.
The first name selects a source, an optional colon introduces its parameter, and each pipe appends a filter:

.. code-block:: elcl

    [academy]
    name: "${registry:main academy}"
    label: "${registry:main academy|trim|academy label:public}"

Source and filter names follow the regular ELCL name rules.
They are case-insensitive, and spaces and underscores are equivalent.
The parser normalizes these names before calling your provider, while parameters preserve their spelling and case after
ELCL escape sequences have been decoded.

The colon is optional and an empty parameter is valid.
A literal pipe starts the next filter, and a literal closing brace ends the expression.
When either character belongs to a parameter, write it using an ELCL Unicode escape, such as ``\u007c`` for a pipe.
A placeholder stays on one physical line and can contain at most 16 filters.

Expansion is deliberately not recursive.
If a source returns the text ``${registry:other}``, that result is inserted literally rather than interpreted as a
second request.
This makes every source lookup visible in the original document and prevents runtime data from introducing new
operations.

Where Expansion Applies
=======================

The parser expands placeholders in ordinary quoted text values, including multiline text and text items in a value list.
It does not expand text names, meta-values such as ``@include``, code values, or regular expressions.
Parser control data therefore remains independent from values supplied at runtime.

A dollar that is not immediately followed by ``{`` remains an ordinary character.
Use ``\$`` when a literal dollar is directly followed by an opening brace and must never start a placeholder.

Providing Application Values
============================

Implement :cpp:class:`PlaceholderSource <erbsland::conf::PlaceholderSource>` when the application owns the lookup.
The ``sourceNames()`` method announces one or more names handled by the provider, and ``resolve()`` turns a source name
and parameter into replacement text.

This demo provides two values from a magical academy registry.
The Portuguese names are application data selected by the generated demo theme; source names, diagnostics, and code
remain in English.

.. erbsland-demo::
    :source: conf/Placeholders/CustomSource.cpp
    :exec: conf/placeholders --demo CustomSource
    :source-sha256: bcc8a82ccd14282375b6e7e157e53b0d23a21573fbd1bfa2512ce9bd2411085d

.. code-block:: cpp

    /// Provide application-owned values for configuration placeholders.
    ///
    /// A placeholder source publishes one or more case-insensitive ELCL names. The parser passes the normalized source name
    /// and the decoded, case-preserving parameter to `resolve()`. The returned text replaces the complete placeholder.
    class AcademySource final : public el::conf::PlaceholderSource {
    public:
        [[nodiscard]] auto sourceNames() const -> el::StringList override { return el::StringList{"academy"_el}; }

        [[nodiscard]] auto resolve(const el::String &, const el::String &parameter) -> el::String override {
            if (parameter == "name"_el) {
                return "Academia da Lua"_el;
            }
            if (parameter == "library"_el) {
                return "Biblioteca das Estrelas"_el;
            }
            throw el::conf::ConfError{
                el::conf::ConfErrorCategory::ValueNotFound,
                el::StringFormat{"The academy value '{}' does not exist."_el}.build(parameter)};
        }
    };

    /// Register a custom source and expand its values while parsing quoted text.
    void customSource() {
        auto parser = el::conf::Parser{};
        parser.addPlaceholderSource(std::make_shared<AcademySource>());
        const auto document = parser.parseTextOrThrow(
            "[academy]\n"
            "name: \"${academy:name}\"\n"
            "welcome: \"Bem-vindo à ${academy:library}\"\n"_el);

        el::io::printLine("Academy: "_el, document->getTextOrThrow("academy.name"_el));
        el::io::printLine("Message: "_el, document->getTextOrThrow("academy.welcome"_el));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Academy: Academia da Lua
    Message: Bem-vindo à Biblioteca das Estrelas

.. erbsland-demo-end::

Register the shared provider with
:cpp:func:`Parser::addPlaceholderSource() <erbsland::conf::Parser::addPlaceholderSource>` before parsing.
One provider may announce several names when those names share a backend or lifecycle.
Each announced name must be a valid, unique regular ELCL name; registration rejects null providers, empty name lists,
invalid names, and collisions with an already registered source.

The parser keeps the provider available for later parse operations on the same parser.
Keep the shared pointer when you need to remove it with
:cpp:func:`Parser::removePlaceholderSource() <erbsland::conf::Parser::removePlaceholderSource>`.
Removal uses provider identity and quietly ignores an instance that is not registered.
Once the last source is removed, placeholder interpretation becomes inactive again.

Transforming Values with Filters
================================

A :cpp:class:`PlaceholderFilter <erbsland::conf::PlaceholderFilter>` transforms the current replacement text.
Its ``filterNames()`` method announces the handled names, while ``apply()`` receives the normalized filter name, the
decoded parameter, and the text produced by the source or previous filter.

Filters run from left to right.
That order makes a chain readable as a small pipeline: first obtain the value, then trim it, escape it, label it, or
apply another application-specific transformation.
The next demo registers a filter that adds a named academy area in front of a room description.

.. erbsland-demo::
    :source: conf/Placeholders/CustomFilter.cpp
    :exec: conf/placeholders --demo CustomFilter
    :source-sha256: 9478a3bfb45c1275ff896412f43594040f3f987b775b6d30eb849bbd3f282b34

.. code-block:: cpp

    /// Transform placeholder values with an application-owned filter.
    ///
    /// A filter publishes one or more names and receives the current text, including changes made by earlier filters in the
    /// chain. Its parameter is decoded but otherwise preserved. This filter adds an application label in front of a value.
    class AcademyLabelFilter final : public el::conf::PlaceholderFilter {
    public:
        [[nodiscard]] auto filterNames() const -> el::StringList override { return el::StringList{"academy label"_el}; }

        [[nodiscard]] auto apply(const el::String &, const el::String &parameter, const el::String &value)
            -> el::String override {
            return el::String::fromJoined({"["_el, parameter, "] "_el, value});
        }
    };

    /// Register a custom filter and apply it after a placeholder source.
    void customFilter() {
        auto parser = el::conf::Parser{};
        parser.addPlaceholderSource(std::make_shared<LiteralSource>());
        parser.addPlaceholderFilter(std::make_shared<AcademyLabelFilter>());
        const auto document = parser.parseTextOrThrow(
            "[academy]\n"
            "room: \"${literal:Biblioteca Arcana|academy label:ala norte}\"\n"_el);

        el::io::printLine("Room: "_el, document->getTextOrThrow("academy.room"_el));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Room: [ala norte] Biblioteca Arcana

.. erbsland-demo-end::

Add a filter with
:cpp:func:`Parser::addPlaceholderFilter() <erbsland::conf::Parser::addPlaceholderFilter>` and remove the same instance
with :cpp:func:`Parser::removePlaceholderFilter() <erbsland::conf::Parser::removePlaceholderFilter>`.
Filter registration follows the same validity and uniqueness rules as source registration.
Filters alone do not activate placeholder parsing because every expression must begin with a source.

Reporting Lookup and Transformation Errors
==========================================

A source or filter cannot always produce valid text.
It may encounter a missing protected value, a denied lookup, or application data that fails a requirement.
Throw :cpp:class:`ConfError <erbsland::conf::ConfError>` with the category and description that best explain the
failure.

The parser preserves that information and enriches the error with the placeholder location, the target value's name
path, and a source excerpt when one is available.
The following demo triggers one source failure and one filter failure so you can see the complete console diagnostics.

.. erbsland-demo::
    :source: conf/Placeholders/ErrorHandling.cpp
    :exec: conf/placeholders --demo SourceError
    :exec-exit-code: 1
    :exec-2: conf/placeholders --demo FilterError
    :exec-2-exit-code: 1
    :source-sha256: 773108932ed4a57f3bfd2f1282411cd62561fbea96a694b2cf4ab432c4168873

.. code-block:: cpp

    /// Create source and filter errors that the parser can enrich with configuration context.
    ///
    /// Providers should report expected lookup and transformation failures as `ConfError`. Choose the category that best
    /// describes the problem and keep the description useful without exposing protected values.
    class RequiredAcademySource final : public el::conf::PlaceholderSource {
    public:
        [[nodiscard]] auto sourceNames() const -> el::StringList override { return el::StringList{"academy"_el}; }
        [[nodiscard]] auto resolve(const el::String &, const el::String &parameter) -> el::String override {
            throw el::conf::ConfError{
                el::conf::ConfErrorCategory::Access,
                el::StringFormat{"The academy registry entry '{}' is unavailable."_el}.build(parameter)};
        }
    };

    class ErrorLiteralSource final : public el::conf::PlaceholderSource {
    public:
        [[nodiscard]] auto sourceNames() const -> el::StringList override { return el::StringList{"literal"_el}; }
        [[nodiscard]] auto resolve(const el::String &, const el::String &parameter) -> el::String override {
            return parameter;
        }
    };

    class SafeRuneFilter final : public el::conf::PlaceholderFilter {
    public:
        [[nodiscard]] auto filterNames() const -> el::StringList override { return el::StringList{"safe rune"_el}; }
        [[nodiscard]] auto apply(const el::String &, const el::String &, const el::String &) -> el::String override {
            throw el::conf::ConfError{
                el::conf::ConfErrorCategory::Validation,
                "The academy title contains a rune that is not permitted here."_el};
        }
    };

    /// Report a source failure at the configuration value that requested it.
    ///
    /// Sources and filters can throw `ConfError` with a meaningful category and description. The parser adds the
    /// placeholder location, the target value's name path, and an available source excerpt before the error reaches the
    /// application.
    void sourceError() {
        auto parser = el::conf::Parser{};
        parser.addPlaceholderSource(std::make_shared<RequiredAcademySource>());
        const auto document = parser.parseTextOrThrow(
            "[academy]\n"
            "name: \"${academy:secret archive}\"\n"_el);
        el::io::printLine("Unexpected value: "_el, document->getTextOrThrow("academy.name"_el));
    }

    /// Report a filter failure with the same configuration context.
    void filterError() {
        auto parser = el::conf::Parser{};
        parser.addPlaceholderSource(std::make_shared<ErrorLiteralSource>());
        parser.addPlaceholderFilter(std::make_shared<SafeRuneFilter>());
        const auto document = parser.parseTextOrThrow(
            "[academy]\n"
            "title: \"${literal:Grimório Antigo|safe rune}\"\n"_el);
        el::io::printLine("Unexpected value: "_el, document->getTextOrThrow("academy.title"_el));
    }

.. rubric:: ``$ conf/placeholders --demo SourceError``

.. erbsland-ansi::
    :escape-char: ␛


      ␛[1;91mAccess␛[22m ␛[1mto␛[22m ␛[1ma␛[22m ␛[1mConfiguration␛[22m ␛[1mSource␛[22m ␛[1mWas␛[22m ␛[1mDenied

      ␛[22;39mThe academy registry entry 'secret archive' is unavailable.

    ␛[1mError␛[22m ␛[1mSource:
      ␛[22;90mLine:␛[39m     2
      ␛[90mColumn:␛[39m   8
      ␛[90mPosition:␛[39m 18

    ␛[1mConfiguration␛[22m ␛[1mError␛[22m ␛[1mDetails:
      ␛[22;90mcategory:␛[39m  Access
      ␛[90mname path:␛[39m academy.name

      ␛[90m   1 │ ␛[39m[academy]
      ␛[90m   2 │ ␛[39mname: "␛[91m$␛[39m{academy:secret archive}"
      ␛[90m     │ ␛[39m       ␛[91m▔␛[0m

.. rubric:: ``$ conf/placeholders --demo FilterError``

.. erbsland-ansi::
    :escape-char: ␛


      ␛[1;91mValidating␛[22m ␛[1mthe␛[22m ␛[1mConfiguration␛[22m ␛[1mFailed

      ␛[22;39mThe academy title contains a rune that is not permitted here.

    ␛[1mError␛[22m ␛[1mSource:
      ␛[22;90mLine:␛[39m     2
      ␛[90mColumn:␛[39m   9
      ␛[90mPosition:␛[39m 19

    ␛[1mConfiguration␛[22m ␛[1mError␛[22m ␛[1mDetails:
      ␛[22;90mcategory:␛[39m  Validation
      ␛[90mname path:␛[39m academy.title

      ␛[90m   1 │ ␛[39m[academy]
      ␛[90m   2 │ ␛[39mtitle: "␛[91m$␛[39m{literal:Grimório Antigo|safe rune}"
      ␛[90m     │ ␛[39m        ␛[91m▔␛[0m

.. erbsland-demo-end::

Malformed expression syntax is reported as a syntax error.
An unknown source or filter is an unsupported-feature error, and a chain beyond the filter limit is a limit error.
These parser-generated errors receive the same value path, location, and excerpt as errors from custom providers.

Choosing the Available Runtime Surface
======================================

Register only sources and filters that a document actually needs.
A source is an application capability: if it can read secrets, inspect the environment, or query another subsystem,
every configuration accepted by that parser can request those operations.
Validate parameters inside the source, return only the required text, and avoid including sensitive values in error
messages.

The library also provides common components.
:doc:`built-in-sources` describes the environment-variable source, and :doc:`built-in-filters` covers reusable text
transformations and checks.
They use the same registration and error model as the custom providers on this page.
