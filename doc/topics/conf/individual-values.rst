..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Configuration; Value Access
    single: Value; Typed Access
    single: Value; Lists and Matrices

******************************
Accessing Configuration Values
******************************

After you locate a value in a configuration tree, you still need to turn it into a useful application type.
The :cpp:class:`Value <erbsland::conf::Value>` interface offers two complementary approaches: ``as...()`` methods
interpret the current value, while ``get...()`` methods resolve a child and interpret it in one step.

This page explains both approaches, including their error behavior, template forms, and list handling.
For everyday application settings, prefer the typed ``get...()`` methods.
Use direct ``as...()`` access when you already have a value object, for example while iterating through an unknown tree
or accepting several deliberate representations.

Convert the Current Value
=========================

Every scalar ELCL type has a matching pair of accessors.
For example, :cpp:func:`asText() <erbsland::conf::Value::asText>` and
:cpp:func:`asTextOrThrow() <erbsland::conf::Value::asTextOrThrow>` access text, while
:cpp:func:`asInteger() <erbsland::conf::Value::asInteger>` and
:cpp:func:`asIntegerOrThrow() <erbsland::conf::Value::asIntegerOrThrow>` access an integer.
Equivalent pairs exist for booleans, floating-point values, dates, times, date-times, byte data, time deltas, and
regular expressions.

These methods do not coerce between ELCL types.
An integer cannot be read as text, and text containing digits cannot be read as an integer.
The non-throwing form returns the type's default-constructed value after a mismatch, such as an empty string or zero.
The ``OrThrow`` form raises :cpp:class:`ConfError <erbsland::conf::ConfError>` with the value's name path and source
location.
Throwing access is therefore the safer choice for required data: an actual zero cannot be confused with a failed
conversion.

.. erbsland-demo::
    :source: conf/IndividualValues/ScalarConversions.cpp
    :exec: conf/individual_values --demo ScalarConversions
    :source-sha256: 6606015640cd2e40d78b2a0868d0b9082e192399066350225a80741f82571a0b

.. code-block:: cpp

    /// Convert one configuration value to its native C++ type.
    ///
    /// The named `as...()` methods make the expected type visible, while `asType<T>()` is useful in generic code and for
    /// converting an ELCL integer to a narrower native integer. The `OrThrow` forms report type and range mismatches.
    void scalarConversions() {
        const auto configuration = "[survey]\n"
                                   "forest: \"Mata Atlântica\"\n"
                                   "maximum_observers: 18\n"_el;
        const auto document = el::conf::Parser{}.parseTextOrThrow(configuration);
        const auto forestValue = document->valueOrThrow("survey.forest"_el);
        const auto observerValue = document->valueOrThrow("survey.maximum_observers"_el);

        // Use a named conversion when the configuration type is known at the call site.
        const auto forest = forestValue->asTextOrThrow();

        // Template conversion can select a narrower native representation for an integer.
        const auto maximumObservers = observerValue->asTypeOrThrow<std::uint16_t>();
        el::io::printLine(forest, ": up to "_el, maximumObservers, " observers"_el);

        // A throwing conversion preserves a type mismatch instead of hiding it as a default value.
        try {
            (void)observerValue->asTextOrThrow();
        } catch (const el::conf::ConfError &) {
            el::io::printLine("An integer cannot be accessed as text."_el);
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Mata Atlântica: up to 18 observers
    An integer cannot be accessed as text.

.. erbsland-demo-end::

The template forms :cpp:func:`asType() <erbsland::conf::Value::asType>` and
:cpp:func:`asTypeOrThrow() <erbsland::conf::Value::asTypeOrThrow>` are useful when the expected C++ type is itself a
template argument.
They also let you select a native integer or floating-point width.
For a target integer that is too narrow, ``asType<T>()`` saturates at the target type's limit, while
``asTypeOrThrow<T>()`` reports the range mismatch.
That distinction makes the throwing form preferable when truncation would change the meaning of a setting.

Choose a Structural View for Lists
==================================

A value list can be viewed either as its original :cpp:type:`ValueList <erbsland::conf::ValueList>` or as a native,
type-checked collection.
:cpp:func:`asValueList() <erbsland::conf::Value::asValueList>` is deliberately strict: it succeeds only when the current
node is a value list.
It does not wrap a scalar in a one-element list.
The returned values retain their names, locations, and validation metadata, which is useful to diagnostics and generic
tree processing.

:cpp:func:`toValueList() <erbsland::conf::Value::toValueList>` provides a more relaxed structural view.
It returns the entries of a value list, wraps a scalar in a one-element list, and returns an empty list for sections and
other structural nodes.
Similarly, :cpp:func:`toValueMatrix() <erbsland::conf::Value::toValueMatrix>` turns a scalar into a one-by-one matrix, a
flat list into one row per entry, and a nested list into its row-and-column layout.
These methods keep ``Value`` objects; they normalize shape without converting content.

For native content, :cpp:func:`asList() <erbsland::conf::Value::asList>` and
:cpp:func:`asMatrix() <erbsland::conf::Value::asMatrix>` are strict about element types.
They accept a scalar of the requested type or a list shape in which every populated element has exactly that ELCL type;
they do not coerce mixed lists.
The non-throwing forms return an empty collection after any mismatch, while ``asListOrThrow<T>()`` and
``asMatrixOrThrow<T>()`` identify the offending value.

.. erbsland-demo::
    :source: conf/IndividualValues/ValueLists.cpp
    :exec: conf/individual_values --demo ValueLists
    :source-sha256: 6f6288fc08833f97ac1f772ddfb04167aca943815b69e325afc3fe983eb9339a

.. code-block:: cpp

    /// Inspect raw value lists and normalize scalar or list-shaped input.
    ///
    /// `asValueList()` strictly requires a value-list node. `toValueList()` and `toValueMatrix()` are deliberately more
    /// relaxed: they also wrap scalar values and turn flat or nested lists into a uniform structural view.
    void valueLists() {
        const auto configuration = "[survey]\n"
                                   "lead_observer: \"Inês\"\n"
                                   "species: \"lobo-guará\", \"jaguatirica\", \"capivara\"\n"
                                   "observations:\n"
                                   "    * 4, 6, 3\n"
                                   "    * 2, 5\n"_el;
        const auto document = el::conf::Parser{}.parseTextOrThrow(configuration);
        const auto speciesValue = document->valueOrThrow("survey.species"_el);

        // Strict list access keeps the individual Value objects and their metadata.
        const auto species = speciesValue->asValueListOrThrow();
        el::io::printLine("First species: "_el, speciesValue->firstValue()->asTextOrThrow());
        el::io::printLine("Last species: "_el, speciesValue->lastValue()->asTextOrThrow());
        el::io::printLine("Species entries: "_el, species.size());

        // Relaxed structural conversion also gives a scalar a one-element list view.
        const auto leadObserver = document->valueOrThrow("survey.lead_observer"_el)->toValueList();
        const auto observationMatrix = document->valueOrThrow("survey.observations"_el)->toValueMatrix();
        el::io::printLine("Lead-observer entries: "_el, leadObserver.size());
        el::io::printLine(
            "Observation matrix: "_el,
            observationMatrix.rowCount(),
            " rows, "_el,
            observationMatrix.columnCount(),
            " columns"_el);
    }

.. erbsland-ansi::
    :escape-char: ␛

    First species: lobo-guará
    Last species: capivara
    Species entries: 3
    Lead-observer entries: 1
    Observation matrix: 2 rows, 3 columns

.. erbsland-demo-end::

:cpp:func:`empty() <erbsland::conf::Value::empty>` tests whether any container has children.
:cpp:func:`firstValue() <erbsland::conf::Value::firstValue>` and
:cpp:func:`lastValue() <erbsland::conf::Value::lastValue>` return its boundary entries, or ``nullptr`` when it is empty.
They are convenient for a quick inspection; use iteration or indexed access when every entry matters.

Read Settings with Typed Getters
================================

The ``get...()`` families combine name-path lookup with type checking.
A call such as :cpp:func:`getText() <erbsland::conf::Value::getText>` first resolves the path relative to the current
value and then requires the result to be text.
If the path is invalid, absent, or has another type, it returns the supplied default.
:cpp:func:`getTextOrThrow() <erbsland::conf::Value::getTextOrThrow>` instead reports the problem as a ``ConfError``.

Use a non-throwing getter with a meaningful default for a genuinely optional setting.
Use an ``OrThrow`` getter when the setting is required or when continuing with a substitute would hide a broken
configuration.
Named methods such as ``getText()``, ``getInteger()``, and ``getDateTime()`` usually produce the clearest application
code.
The template :cpp:func:`get() <erbsland::conf::Value::get>` and
:cpp:func:`getOrThrow() <erbsland::conf::Value::getOrThrow>` variants are helpful in generic code and for selecting a
native numeric width.

.. erbsland-demo::
    :source: conf/IndividualValues/TypedGetters.cpp
    :exec: conf/individual_values --demo TypedGetters
    :source-sha256: 8832afcaeb5124fdc00b57c61426bddacb81bfa0053e722c4a5810feada5cfb4

.. code-block:: cpp

    /// Read application settings with typed convenience getters.
    ///
    /// Named getters produce compact, expressive code for ordinary settings. Template getters support the same path-like
    /// inputs and are useful in generic code or when a native integer width must be selected explicitly.
    void typedGetters() {
        const auto configuration = "[survey]\n"
                                   "forest: \"Floresta da Tijuca\"\n"
                                   "observers: 7\n"
                                   "species: \"mico-leão-dourado\", \"tucano\"\n"_el;
        const auto document = el::conf::Parser{}.parseTextOrThrow(configuration);

        // A string path is concise, while a Name or NamePath is convenient when paths are assembled or reused.
        const auto forest = document->getTextOrThrow("survey.forest"_el);
        const auto survey = document->valueOrThrow(el::conf::Name::createRegular("survey"_el));
        const auto observersPath = el::conf::NamePath::fromText("observers"_el);
        const auto observers = survey->getOrThrow<std::uint16_t>(observersPath);

        // An index is the fourth path-like form and addresses an entry in a list directly.
        const auto species = document->valueOrThrow("survey.species"_el);
        const auto firstSpecies = species->getOrThrow<el::String>(std::size_t{0});

        // Non-throwing getters make an application default explicit for optional values.
        const auto season = document->getText("survey.season"_el, "seca"_el);
        el::io::printLine(forest, ": "_el, observers, " observers"_el);
        el::io::printLine("First species: "_el, firstSpecies, "; season: "_el, season);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Floresta da Tijuca: 7 observers
    First species: mico-leão-dourado; season: seca

.. erbsland-demo-end::

All getters accept :cpp:type:`NamePathLike <erbsland::conf::NamePathLike>`.
As shown in the demo, that can be a string containing a complete path, one
:cpp:class:`Name <erbsland::conf::Name>`, a reusable
:cpp:class:`NamePath <erbsland::conf::NamePath>`, or an index into a list.
See :doc:`value-trees` for path syntax and guidance on choosing among these forms.

Require Uniform Lists and Matrices
==================================

When the application needs native values rather than tree nodes, use
:cpp:func:`getList() <erbsland::conf::Value::getList>` or
:cpp:func:`getListOrThrow() <erbsland::conf::Value::getListOrThrow>`.
These methods resolve the path and require every list entry to have the requested ELCL type.
They also accept one scalar of that type and return a one-element vector, which lets a setting support both concise and
expanded input without special-case code.

The corresponding :cpp:func:`getMatrix() <erbsland::conf::Value::getMatrix>` and
:cpp:func:`getMatrixOrThrow() <erbsland::conf::Value::getMatrixOrThrow>` methods accept scalar, flat-list, and
nested-list shapes and return a type-checked :cpp:class:`Matrix <erbsland::conf::Matrix>`.
On any path, shape, or element-type problem, a non-throwing getter returns an empty collection; the ``OrThrow`` form
identifies the failing value.

.. erbsland-demo::
    :source: conf/IndividualValues/TypedCollections.cpp
    :exec: conf/individual_values --demo TypedCollections
    :source-sha256: da7257835d7533b85a9eed7f7589e4883eca655bcc47b2371653a649be485762

.. code-block:: cpp

    /// Read type-checked lists and matrices from a document.
    ///
    /// Typed collection getters resolve the path, verify every element, and return native values. They also accept a scalar
    /// of the requested type as a one-element list or matrix, which lets a setting support concise and expanded forms.
    void typedCollections() {
        const auto configuration = "[survey]\n"
                                   "species: \"onça-pintada\", \"tamanduá-bandeira\"\n"
                                   "sample_count: 12\n"
                                   "observations:\n"
                                   "    * 4, 6, 3\n"
                                   "    * 2, 5, 1\n"_el;
        const auto document = el::conf::Parser{}.parseTextOrThrow(configuration);

        // Typed getters return native collections after checking every element.
        const auto species = document->getListOrThrow<el::String>("survey.species"_el);
        const auto sampleCounts = document->getListOrThrow<int>("survey.sample_count"_el);
        const auto observations = document->getMatrixOrThrow<int>("survey.observations"_el);

        el::io::printLine("Species: "_el, species.size());
        el::io::printLine("Sample-count entries: "_el, sampleCounts.size());
        el::io::printLine("First observation: "_el, observations.valueOrThrow({}, {}));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Species: 2
    Sample-count entries: 1
    First observation: 4

.. erbsland-demo-end::

The similarly named APIs serve different layers.
``value(...)->asValueList()`` first resolves a value explicitly and then strictly exposes its raw children.
``getValueList(...)`` combines those two operations but still requires an actual value-list node.
``getList<T>(...)`` additionally checks every element, returns native C++ values, and accepts a matching scalar.
The direct ``asList<T>()`` and ``asMatrix<T>()`` methods perform the same typed conversion on a ``Value`` you already
hold.

Inspect Types Before Choosing a Representation
==============================================

Most settings should have one documented type and use a typed getter.
When a format intentionally permits alternatives, the ``is...()`` methods let you inspect the node without triggering a
conversion.
For example, :cpp:func:`isText() <erbsland::conf::Value::isText>` and
:cpp:func:`isInteger() <erbsland::conf::Value::isInteger>` can distinguish a named identifier from a numeric one before
the appropriate ``as...OrThrow()`` call.

.. erbsland-demo::
    :source: conf/IndividualValues/TypeTests.cpp
    :exec: conf/individual_values --demo TypeTests
    :source-sha256: 041def5890b6a1ce757924c5268197c67ac6ceb49142f0a1417f96f09cb17647

.. code-block:: cpp

    /// Inspect a value before accepting one of several representations.
    ///
    /// The `is...()` methods make intentional alternatives explicit. Use them when an application genuinely accepts more
    /// than one ELCL type; prefer a typed getter when the configuration contract requires exactly one type.
    void typeTests() {
        const auto configuration = "[survey]\n"
                                   "station: 42\n"
                                   "reference_species: \"bugio-ruivo\"\n"_el;
        const auto document = el::conf::Parser{}.parseTextOrThrow(configuration);

        const auto station = document->valueOrThrow("survey.station"_el);
        if (station->isInteger()) {
            el::io::printLine("Numeric station: "_el, station->asIntegerOrThrow());
        } else if (station->isText()) {
            el::io::printLine("Named station: "_el, station->asTextOrThrow());
        }

        const auto referenceSpecies = document->valueOrThrow("survey.reference_species"_el);
        el::io::printLine("Reference is text: "_el, el::BooleanFormat::yesNo(), referenceSpecies->isText());
        el::io::printLine("Document is a map: "_el, el::BooleanFormat::yesNo(), document->isMap());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Numeric station: 42
    Reference is text: yes
    Document is a map: yes

.. erbsland-demo-end::

Tests exist for every scalar and container type.
The broader :cpp:func:`isList() <erbsland::conf::Value::isList>` recognizes value lists and section lists, while
:cpp:func:`isMap() <erbsland::conf::Value::isMap>` recognizes documents and named sections.
These broader tests are useful for generic tree tools; application configuration usually benefits from the more precise
type tests.

Validate Before Access
======================

Typed access verifies the setting currently being read, but it does not prove that the whole document follows the
application's contract.
For larger configurations, :doc:`validation-rules` explains how to check the complete tree, apply defaults, and attach
metadata before application code starts reading values.
