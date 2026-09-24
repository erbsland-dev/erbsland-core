..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Configuration; Parsing Documents
    single: ELCL; Parsing Documents
    single: Validation Rules; Configuration Loading

*******************************
Parsing Configuration Documents
*******************************

An ELCL document becomes useful to an application when its text has been parsed, its assumptions have been checked, and
its values have been turned into application state.
This page follows that complete path, from creating a parser to choosing an appropriate validation strategy.
You will also see why a short, deliberate lifetime for parsed documents is a good default for production applications.

If ELCL itself is new to you, the `Erbsland Configuration Language documentation <https://config-lang.erbsland.dev>`_
explains its syntax and data types.
The examples on this page use small synthesizer-patch documents, but the same workflow applies to service settings, tool
preferences, and other application configuration.

Parse and Access a Document
===========================

:cpp:class:`Parser <erbsland::conf::Parser>` reads ELCL text and creates a
:cpp:class:`Document <erbsland::conf::Document>`.
For configuration stored in a file, call
:cpp:func:`parseFileOrThrow() <erbsland::conf::Parser::parseFileOrThrow>`.
For text already held in memory, call
:cpp:func:`parseTextOrThrow() <erbsland::conf::Parser::parseTextOrThrow>`.
The more general :cpp:func:`parseOrThrow() <erbsland::conf::Parser::parseOrThrow>` accepts a
:cpp:class:`Source <erbsland::conf::Source>` and is the entry point for custom sources.

The resulting document is the root of a typed value tree.
The most direct way to read a setting is to call one of its ``get...()`` methods with a name path such as
``patch.voices``.
The ``...OrThrow()`` variants require the value to exist and to have the requested type.
Methods without that suffix return a supplied default when the path is missing, malformed, or has another type.
This makes the intended distinction between required and optional settings visible at each call site.

.. erbsland-demo::
    :source: conf/ConfigurationDocuments/ParseAndAccess.cpp
    :exec: conf/configuration_documents --demo ParseAndAccess
    :source-sha256: 4f9444038091e6fd0fc3e510e199342aa1e96ff9060cf249b79d40eecf467e31

.. code-block:: cpp

    /// Parse an ELCL document and access its typed values.
    ///
    /// `Parser::parseTextOrThrow()` is convenient for embedded text. For files, use `parseFileOrThrow()`; for a custom
    /// source, create a `Source` and pass it to `parseOrThrow()`. The resulting document provides typed `get...()` methods
    /// that resolve complete name paths without manual tree traversal.
    void parseAndAccess() {
        const auto configuration = "[patch]\n"
                                   "name: \"Gece Göğü\"\n"
                                   "voices: 8\n"
                                   "stereo: yes\n"_el;

        // Parse the text and let a configuration error propagate to the application.
        auto parser = el::conf::Parser{};
        const auto document = parser.parseTextOrThrow(configuration);

        // Required values use throwing accessors; optional values provide a default.
        const auto name = document->getTextOrThrow("patch.name"_el);
        const auto voices = document->getIntegerOrThrow("patch.voices"_el);
        const auto stereo = document->getBoolean("patch.stereo"_el, false);

        el::io::printLine("Patch: "_el, name);
        el::io::printLine("Voices: "_el, voices);
        el::io::printLine("Stereo: "_el, el::BooleanFormat::yesNo(), stereo);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Patch: Gece Göğü
    Voices: 8
    Stereo: yes

.. erbsland-demo-end::

The parser also offers non-throwing :cpp:func:`parse() <erbsland::conf::Parser::parse>`,
:cpp:func:`parseFile() <erbsland::conf::Parser::parseFile>`, and
:cpp:func:`parseText() <erbsland::conf::Parser::parseText>` variants.
They return ``nullptr`` after a configuration error, and
:cpp:func:`lastError() <erbsland::conf::Parser::lastError>` provides the structured error context.
This form is helpful when failure is expected and local recovery is useful, for example in a configuration editor.

For ordinary application startup, the throwing variants are usually clearer.
They preserve the complete :cpp:class:`ConfError <erbsland::conf::ConfError>` and let the application's top-level
diagnostic handling present it consistently.
They also prevent the loading path from continuing with an incomplete configuration.

Understand Error Locations and Source Excerpts
==============================================

A :cpp:class:`ConfError <erbsland::conf::ConfError>` reports the source path, line, and column whenever that information
is available.
It also carries a short source excerpt when the originating source can provide one.
This applies to both in-memory text and file sources, so parse, access, and validation failures can use the same
diagnostic presentation.

File excerpts are a best-effort service.
They are normally available for errors raised during parsing and may also be recovered for errors raised later by a
throwing accessor or validation rule.
If a source file has become unavailable or is unsuitable for a small diagnostic read, the error keeps its location but
omits the excerpt; failure to obtain an excerpt never replaces the original configuration error.

Validation rules marked ``is_secret`` deliberately suppress source excerpts for errors associated with that value.
The path, line, and column remain available, but the diagnostic does not reproduce the secret source text.

Choose the Document Lifetime
============================

A parsed document can either be a temporary input or the application's long-lived configuration model.
Both strategies are valid, but they have different consequences.

For a larger application, prefer a load--validate--interpret cycle.
Parse the document, validate every assumption, copy the relevant values into an application-specific settings object,
and then release the document.
This separates the external file format from runtime state and prevents later code from discovering configuration errors
halfway through normal operation.
It also releases source excerpts and plain-text values that may contain credentials or other sensitive data as soon as
they are no longer needed.

.. erbsland-demo::
    :source: conf/ConfigurationDocuments/LoadingStrategies.cpp
    :exec: conf/configuration_documents --demo LoadingStrategies
    :source-sha256: 696b36eceef3e80d8b59076f75d8a246ab8cb7e5b2aaa71821526a9e9ea3be9c

.. code-block:: cpp

    /// Turn a validated document into an application-owned settings object.
    ///
    /// Keeping the parsed document only until its values have been checked and copied makes ownership clear. It also
    /// shortens the time in which the parser's source text and sensitive configuration values remain reachable.
    void loadingStrategies() {
        struct PatchSettings {
            el::String name;
            el::conf::Integer voices;
            bool stereo;
        };

        const auto configuration = "[patch]\n"
                                   "name: \"Ay Işığı\"\n"
                                   "voices: 6\n"
                                   "stereo: yes\n"_el;

        // Load and validate all required values before changing application state.
        auto document = el::conf::Parser{}.parseTextOrThrow(configuration);
        auto settings = PatchSettings{
            .name = document->getTextOrThrow("patch.name"_el),
            .voices = document->getIntegerOrThrow("patch.voices"_el),
            .stereo = document->getBoolean("patch.stereo"_el, false),
        };
        if (settings.voices < 1 || settings.voices > 32) {
            throw el::RuntimeError{"The patch must use between 1 and 32 voices."_el};
        }

        // The application no longer needs the document after interpretation.
        document.reset();
        el::io::printLine(settings.name, ": "_el, settings.voices, " voices"_el);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Ay Işığı: 6 voices

.. erbsland-demo-end::

Small tools and scripts may instead keep the document and read values when they are needed.
That approach removes a settings type and some copying, which can be worthwhile when the configuration is tiny and the
process is short-lived.
The trade-off is that parsing concerns remain mixed with application logic, missing values may be noticed late, and the
entire document stays in memory.
Whichever strategy you choose, validate required settings together before the application begins work.

Validate Small Documents Directly
=================================

Manual validation is often the simplest fit for a small, stable document.
Read required values with throwing accessors such as
:cpp:func:`getTextOrThrow() <erbsland::conf::Value::getTextOrThrow>` and
:cpp:func:`getIntegerOrThrow() <erbsland::conf::Value::getIntegerOrThrow>`.
A missing value or a type mismatch then becomes a configuration error at the point where the requirement is stated.
After access, ordinary application checks can enforce ranges and relationships.

Sometimes a setting deliberately accepts more than one ELCL type.
In that case, retrieve its :cpp:class:`Value <erbsland::conf::Value>` with
:cpp:func:`valueOrThrow() <erbsland::conf::Value::valueOrThrow>` and use tests such as
:cpp:func:`isText() <erbsland::conf::Value::isText>` or
:cpp:func:`isInteger() <erbsland::conf::Value::isInteger>` before interpreting it.

.. erbsland-demo::
    :source: conf/ConfigurationDocuments/ManualValidation.cpp
    :files: examples/valid-patch.elcl examples/invalid-patch.elcl
    :files-sha256: 96cdda1e29047fc389c2c1d6b97383a071d50de9043121613b87ce0850170a2c
    :exec: conf/configuration_documents --demo ManualValidation valid-patch.elcl
    :exec-2: conf/configuration_documents --demo ManualValidation invalid-patch.elcl
    :exec-2-exit-code: 1
    :source-sha256: 1bf55d2e39bd35c81e604dd7268397c157aa4ca968f5fca1d0b95cab245578a5

.. code-block:: cpp

    /// Validate a small document directly while reading its values.
    ///
    /// Throwing accessors are a compact way to require a value and its type. When several types are acceptable, first
    /// retrieve the value with `valueOrThrow()` and inspect it with the `is...()` methods.
    void manualValidation(const el::Path &configurationPath) {
        const auto document = el::conf::Parser{}.parseFileOrThrow(configurationPath);

        // These calls require both values and verify their native types.
        const auto name = document->getTextOrThrow("patch.name"_el);
        const auto voices = document->getIntegerOrThrow("patch.voices"_el);
        if (voices < 1 || voices > 32) {
            throw el::RuntimeError{"The patch must use between 1 and 32 voices."_el};
        }

        // Inspect the value when the application accepts more than one representation.
        const auto waveform = document->valueOrThrow("patch.oscillator.waveform"_el);
        if (!waveform->isText() && !waveform->isInteger()) {
            throw el::RuntimeError{"The waveform must be text or an integer identifier."_el};
        }

        el::io::printLine("Validated patch: "_el, name);
    }

.. rubric:: ``$ conf/configuration_documents --demo ManualValidation valid-patch.elcl``

.. erbsland-ansi::
    :escape-char: ␛

    Validated patch: Kuzey Rüzgârı

.. rubric:: ``$ conf/configuration_documents --demo ManualValidation invalid-patch.elcl``

.. erbsland-ansi::
    :escape-char: ␛


      ␛[1;91mConverting␛[22m ␛[1ma␛[22m ␛[1mConfiguration␛[22m ␛[1mValue␛[22m ␛[1mFailed

      ␛[22;39mA value has not the required type. Expected 'Integer' but got 'Text'.

    ␛[1mError␛[22m ␛[1mSource:
      ␛[22;90mPath:␛[39m     invalid-patch.elcl
      ␛[90mLine:␛[39m     3
      ␛[90mColumn:␛[39m   1
      ␛[90mPosition:␛[39m 31

    ␛[1mConfiguration␛[22m ␛[1mError␛[22m ␛[1mDetails:
      ␛[22;90mcategory:␛[39m  TypeMismatch
      ␛[90mname path:␛[39m patch.voices

      ␛[90m   1 │ ␛[39m[patch]
      ␛[90m   2 │ ␛[39mname: "Kuzey Rüzgârı"
      ␛[90m   3 │ ␛[91mv␛[39moices: "twelve"
      ␛[90m     │ ␛[91m▔
      ␛[90m   4 │
         5 │ ␛[39m[patch.oscillator]

.. erbsland-demo-end::

The examples in this section use the same two input documents.
The valid document satisfies all requirements:

.. literalinclude:: examples/valid-patch.elcl
    :language: elcl

The invalid document uses text where the ``voices`` setting requires an integer:

.. literalinclude:: examples/invalid-patch.elcl
    :language: elcl

This style keeps a short schema close to the code that consumes it.
As the number of values, alternatives, defaults, or cross-value constraints grows, dedicated validation rules become
easier to review and reuse.

Load Validation Rules from a Document
=====================================

Validation rules can be written as a separate ELCL document.
This keeps the schema readable as data and allows it to evolve independently from the code that loads the application
configuration.
The `ELCL validation-rules documentation <https://config-lang.erbsland.dev/validation-rules/index.html>`_ describes the
rule-document format and its available constraints.

Parse the rule document with the ordinary parser, then pass it to
:cpp:func:`Rules::createFromDocument() <erbsland::conf::vr::Rules::createFromDocument>`.
The factory validates the rule definition and returns a reusable :cpp:class:`Rules <erbsland::conf::vr::Rules>`
instance.
Calling :cpp:func:`Rules::validate() <erbsland::conf::vr::Rules::validate>` checks the application document and also
attaches validation metadata and configured defaults to its values.

.. erbsland-demo::
    :source: conf/ConfigurationDocuments/DocumentRules.cpp
    :files: examples/valid-patch.elcl examples/invalid-patch.elcl
    :files-sha256: 96cdda1e29047fc389c2c1d6b97383a071d50de9043121613b87ce0850170a2c
    :exec: conf/configuration_documents --demo DocumentRules valid-patch.elcl
    :exec-2: conf/configuration_documents --demo DocumentRules invalid-patch.elcl
    :exec-2-exit-code: 1
    :source-sha256: 7da2c53646cf8539ce1e8125e155601294bf66ab5fc89f73149cab39b6e62bac

.. code-block:: cpp

    /// Validate configuration with rules written as an ELCL document.
    ///
    /// Parse the rules like any other configuration, compile them with `Rules::createFromDocument()`, and apply the
    /// resulting reusable rule set to a parsed document. Validation checks types and constraints before values are used.
    void documentRules(const el::Path &configurationPath) {
        const auto ruleText = "[patch]\n"
                              "type: \"section\"\n"
                              "[patch.name]\n"
                              "type: \"text\"\n"
                              "[patch.voices]\n"
                              "type: \"integer\"\n"
                              "minimum: 1\n"
                              "maximum: 32\n"
                              "[patch.oscillator]\n"
                              "type: \"section\"\n"
                              "[patch.oscillator.waveform]\n"
                              "type: \"text\"\n"_el;

        // Compile the rules once, then use them for every document with this schema.
        const auto ruleDocument = el::conf::Parser{}.parseTextOrThrow(ruleText);
        const auto rules = el::conf::vr::Rules::createFromDocument(ruleDocument);
        const auto document = el::conf::Parser{}.parseFileOrThrow(configurationPath);
        rules->validate(document, 1);

        el::io::printLine("Validated voices: "_el, document->getIntegerOrThrow("patch.voices"_el));
    }

.. rubric:: ``$ conf/configuration_documents --demo DocumentRules valid-patch.elcl``

.. erbsland-ansi::
    :escape-char: ␛

    Validated voices: 12

.. rubric:: ``$ conf/configuration_documents --demo DocumentRules invalid-patch.elcl``

.. erbsland-ansi::
    :escape-char: ␛


      ␛[1;91mValidating␛[22m ␛[1mthe␛[22m ␛[1mConfiguration␛[22m ␛[1mFailed

      ␛[22;39mExpected an integer value but got a text value

    ␛[1mError␛[22m ␛[1mSource:
      ␛[22;90mPath:␛[39m     invalid-patch.elcl
      ␛[90mLine:␛[39m     3
      ␛[90mColumn:␛[39m   1
      ␛[90mPosition:␛[39m 31

    ␛[1mConfiguration␛[22m ␛[1mError␛[22m ␛[1mDetails:
      ␛[22;90mcategory:␛[39m  Validation
      ␛[90mname path:␛[39m patch.voices

      ␛[90m   1 │ ␛[39m[patch]
      ␛[90m   2 │ ␛[39mname: "Kuzey Rüzgârı"
      ␛[90m   3 │ ␛[91mv␛[39moices: "twelve"
      ␛[90m     │ ␛[91m▔
      ␛[90m   4 │
         5 │ ␛[39m[patch.oscillator]

.. erbsland-demo-end::

Create the rules once when the application starts and reuse them for documents that follow the same schema.
The integer passed to ``validate()`` is the schema version used to select versioned rules; applications without
versioned rules can consistently pass their current version.

Build Validation Rules in C++
=============================

When the schema is an inseparable part of the executable, construct it with
:cpp:class:`RulesBuilder <erbsland::conf::vr::RulesBuilder>`.
Each call to :cpp:func:`addRule() <erbsland::conf::vr::RulesBuilder::addRule>` associates a name path with a
:cpp:class:`RuleType <erbsland::conf::vr::RuleType>` and optional attributes or constraints.
:cpp:func:`takeRules() <erbsland::conf::vr::RulesBuilder::takeRules>` finalizes the definition and returns the same
``Rules`` interface used for document-based schemas.

.. erbsland-demo::
    :source: conf/ConfigurationDocuments/CompiledRules.cpp
    :files: examples/valid-patch.elcl examples/invalid-patch.elcl
    :files-sha256: 96cdda1e29047fc389c2c1d6b97383a071d50de9043121613b87ce0850170a2c
    :exec: conf/configuration_documents --demo CompiledRules valid-patch.elcl
    :exec-2: conf/configuration_documents --demo CompiledRules invalid-patch.elcl
    :exec-2-exit-code: 1
    :source-sha256: cef37203f2b2c277ac9b37ac0595f858ba25290a93c4eb8cadce663ff64ce6b5

.. code-block:: cpp

    /// Build validation rules directly in C++.
    ///
    /// `RulesBuilder` creates the same reusable validation model as a rules document. This form is useful when the schema
    /// belongs to the executable and should be checked by the C++ compiler along with the code that consumes it.
    void compiledRules(const el::Path &configurationPath) {
        using el::conf::vr::RuleType;
        using el::conf::vr::builder::Maximum;
        using el::conf::vr::builder::Minimum;

        auto builder = el::conf::vr::RulesBuilder{};
        builder.addRule("patch"_el, RuleType::Section);
        builder.addRule("patch.name"_el, RuleType::Text);
        builder.addRule("patch.voices"_el, RuleType::Integer, Minimum(1), Maximum(32));
        builder.addRule("patch.oscillator"_el, RuleType::Section);
        builder.addRule("patch.oscillator.waveform"_el, RuleType::Text);
        const auto rules = builder.takeRules();

        const auto document = el::conf::Parser{}.parseFileOrThrow(configurationPath);
        rules->validate(document, 1);

        el::io::printLine("Validated patch: "_el, document->getTextOrThrow("patch.name"_el));
    }

.. rubric:: ``$ conf/configuration_documents --demo CompiledRules valid-patch.elcl``

.. erbsland-ansi::
    :escape-char: ␛

    Validated patch: Kuzey Rüzgârı

.. rubric:: ``$ conf/configuration_documents --demo CompiledRules invalid-patch.elcl``

.. erbsland-ansi::
    :escape-char: ␛


      ␛[1;91mValidating␛[22m ␛[1mthe␛[22m ␛[1mConfiguration␛[22m ␛[1mFailed

      ␛[22;39mExpected an integer value but got a text value

    ␛[1mError␛[22m ␛[1mSource:
      ␛[22;90mPath:␛[39m     invalid-patch.elcl
      ␛[90mLine:␛[39m     3
      ␛[90mColumn:␛[39m   1
      ␛[90mPosition:␛[39m 31

    ␛[1mConfiguration␛[22m ␛[1mError␛[22m ␛[1mDetails:
      ␛[22;90mcategory:␛[39m  Validation
      ␛[90mname path:␛[39m patch.voices

      ␛[90m   1 │ ␛[39m[patch]
      ␛[90m   2 │ ␛[39mname: "Kuzey Rüzgârı"
      ␛[90m   3 │ ␛[91mv␛[39moices: "twelve"
      ␛[90m     │ ␛[91m▔
      ␛[90m   4 │
         5 │ ␛[39m[patch.oscillator]

.. erbsland-demo-end::

Compiled rules are checked together with the code and do not require a second deployment file.
A rules document is usually more approachable when non-C++ tooling or people outside the implementation team maintain
the schema.
The choice only changes how the ``Rules`` instance is created; parsing, validation, and application access remain the
same.

Continue with the Value API
===========================

The typed ``get...()`` methods are the recommended path for most settings.
When you need to inspect arbitrary sections, walk section lists, or preserve value locations, :doc:`value-trees`
explains how names, name paths, child lookup, parents, and iteration fit together.
Continue with :doc:`individual-values` for direct conversion, typed collections, template getters, and type tests, or
with :doc:`validation-rules` for complete document schemas.
