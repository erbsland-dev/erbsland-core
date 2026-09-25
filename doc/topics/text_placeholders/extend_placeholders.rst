..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Placeholders; Custom Source
    single: Placeholders; Custom Filter
    single: Replacer; Providers

*************************************************
Providing Your Own Placeholder Values and Filters
*************************************************

The built-in ``var`` and ``env`` sources are convenient when values already live in a map or the process environment.
An application may instead keep text in a catalog, a protected store, or a service with its own lookup rules.
A custom :cpp:class:`Source <erbsland::text::placeholder::Source>` lets the application own that lookup while the
:cpp:class:`Replacer <erbsland::text::placeholder::Replacer>` continues to handle expression syntax.

You can also add a :cpp:class:`Filter <erbsland::text::placeholder::Filter>` for a transformation that belongs to your
application.
This page builds one of each for gallery labels.
For the basic expression form, start with :doc:`placeholders`.

Give a Source a Clear Responsibility
====================================

A source publishes one or more names through ``sourceNames()``.
For an expression such as ``${gallery:movement}``, the replacer finds the provider registered as ``gallery`` and calls
``resolve()`` with the normalized source name and the parameter ``movement``.
The returned :cpp:type:`String <erbsland::text::String>` replaces the whole expression.

The gallery example accepts one field.
An unknown field raises :cpp:class:`ReplacerError <erbsland::text::placeholder::ReplacerError>` with the
``ValueNotFound`` category, so strict replacement can tell the caller why it failed.
The provider also overrides ``validate()``: it checks the field name without querying the catalog.

.. erbsland-demo::
    :source: text/Placeholders/CustomSource.cpp
    :exec: text/text_placeholders --demo UseCustomSource
    :source-sha256: 1945ff260ca56a934eb4dd80e2807a161afc6991a5b804ae407eccf1f0e75146

.. code-block:: cpp

    /// Provide exhibition text from an application-owned catalog.
    ///
    /// A source publishes its accepted names and resolves a case-preserving parameter. A failed lookup throws
    /// `ReplacerError`; overriding `validate()` lets syntax checks avoid querying the catalog.
    class GallerySource final : public el::placeholder::Source {
    public:
        [[nodiscard]] auto sourceNames() const -> el::StringList override { return el::StringList{"gallery"_el}; }

        [[nodiscard]] auto resolve(const el::String &, const el::String &parameter) -> el::String override {
            if (parameter == "movement"_el) {
                return u8"Συμβολισμός"_el;
            }
            throw el::placeholder::ReplacerError{
                el::placeholder::ReplacerErrorCategory::ValueNotFound,
                el::StringFormat{"Unknown gallery field: {}"_el}.build(parameter)};
        }

        [[nodiscard]] auto validate(const el::String &, const el::String &parameter) -> bool override {
            return parameter == "movement"_el;
        }
    };

    /// Register a custom source before expanding a gallery caption.
    void useCustomSource() {
        auto replacer = el::placeholder::Replacer{};
        const auto source = std::make_shared<GallerySource>();
        replacer.addSource(source);

        el::io::printLine(replacer.replaceOrThrow(u8"Featured movement: ${gallery:movement}"_el));
        el::io::printLine("Valid caption: "_el, replacer.validate("${gallery:movement}"_el));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Featured movement: Συμβολισμός
    Valid caption: true

.. erbsland-demo-end::

The default ``Source::validate()`` calls ``resolve()`` and returns ``false`` when it catches a ``ReplacerError``.
That default is useful for a simple, side-effect-free source.
Override it when validation should check only the request's shape, or when a lookup is expensive or has side effects.
An accepted parameter still may fail during replacement if the underlying value is unavailable then.

Register a shared provider with ``addSource()`` before replacement.
The provider can announce several names when they share a backend.
Each name must be a unique regular ELCL name; a null provider, an empty name list, invalid names, or a collision is
rejected at registration.
``source()`` finds a provider by name, and ``removeSource()`` removes all names of the *same provider instance*.
Keep its shared pointer if you intend to remove it later.

Transform a Value with a Filter
===============================

Sometimes the source has the right data, but the destination needs one more step.
The built-in ``trim`` and ``upper`` filters handle general text changes.
A gallery-specific filter can add a label that the rest of the application uses consistently.

A filter announces its names through ``filterNames()``.
Its ``apply()`` method receives the normalized filter name, its case-preserving parameter, and the current text.
In a chain, that text is either the source result or the previous filter's result.
The following filter accepts ``room`` as its parameter and prefixes the trimmed value.

.. erbsland-demo::
    :source: text/Placeholders/CustomFilter.cpp
    :exec: text/text_placeholders --demo UseCustomFilter
    :source-sha256: 6da03e36a875da998ef0134047ea48393263942570fda3ba46a7dcf481b435b2

.. code-block:: cpp

    /// Add a destination-specific label to a placeholder value.
    ///
    /// A filter receives the source value or the result of the previous filter. Its parameter selects the label;
    /// `validate()` checks that choice without needing a value to transform.
    class GalleryLabelFilter final : public el::placeholder::Filter {
    public:
        [[nodiscard]] auto filterNames() const -> el::StringList override { return el::StringList{"gallery_label"_el}; }

        [[nodiscard]] auto apply(const el::String &, const el::String &parameter, const el::String &value)
            -> el::String override {
            if (parameter != "room"_el) {
                throw el::placeholder::ReplacerError{
                    el::placeholder::ReplacerErrorCategory::Syntax, "Expected gallery label 'room'."_el};
            }
            return el::StringFormat{"Room: {}"_el}.build(value);
        }

        [[nodiscard]] auto validate(const el::String &, const el::String &parameter) -> bool override {
            return parameter == "room"_el;
        }
    };

    /// Register and chain a custom filter after a built-in text filter.
    void useCustomFilter() {
        auto replacer = el::placeholder::Replacer{};
        replacer.setVariableSource({{{"movement"_el, u8"  Φουτουρισμός  "_el}}});
        replacer.addTextFilters();
        replacer.addFilter(std::make_shared<GalleryLabelFilter>());

        el::io::printLine(replacer.replaceOrThrow("${var:movement|trim|gallery_label:room}"_el));
    }

.. erbsland-demo-end::

The expression ``${var:movement|trim|gallery_label:room}`` is read from left to right.
The custom filter therefore receives the clean movement name, rather than the value with its original whitespace.
It throws ``ReplacerError`` for another parameter; its ``validate()`` checks the same rule before any value exists.

The default ``Filter::validate()`` calls ``apply()`` with an empty value and catches ``ReplacerError``.
For a value-dependent filter, that empty value may not represent a real request, so a separate parameter check is often
clearer.
``addFilter()``, ``filter()``, and ``removeFilter()`` follow the same naming and provider-identity rules as their source
counterparts.

Keep Failures Understandable
============================

A source should report a missing requested value with ``ValueNotFound`` and a rejected access with ``Access``.
A filter should report malformed parameters with ``Syntax`` and a value that fails a required condition with
``Validation``.
These categories help a caller distinguish an incorrect expression from unavailable data.

The replacer attaches the position of the expression in the original input when a ``ReplacerError`` reaches it.
``replaceOrThrow()`` passes that error to the caller.
``replace()`` keeps the failed expression and continues, which is useful for a preview but can leave unresolved text in
a final document.
Exceptions of other types from a provider are not converted into tolerant replacement.

Once your providers are in place, :doc:`customize_placeholder_syntax` explains how to choose delimiters that fit the
text around them and how to avoid accidental second-stage interpretation.
