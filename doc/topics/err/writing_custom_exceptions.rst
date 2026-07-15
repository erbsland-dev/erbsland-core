..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Exceptions; Custom Exceptions
    single: Diagnostics; Custom Diagnostics
    single: Error Context Pattern

*************************
Writing Custom Exceptions
*************************

Create a custom exception when callers need to identify a domain failure or when the failure carries structured details
that a neutral exception cannot express.
This page starts with a minimal named type, then develops the context and diagnostic patterns used by Erbsland Core
itself.

Start with the Smallest Useful Type
===================================

Use an existing neutral exception when its category and reason are sufficient.
A custom type is valuable when callers need a specific catch clause, when several operations share the same failure
contract, or when you want to attach a domain diagnostic.

Derive from :cpp:class:`RuntimeError <erbsland::err::RuntimeError>` for failures that application code may handle.
Derive from :cpp:class:`LogicError <erbsland::err::LogicError>` for broken invariants and invalid API use.
A minimal type can inherit the base constructors and therefore receives reason and cause support without boilerplate.

.. erbsland-demo::
    :source: err/Exception/MinimalCustomException.cpp
    :exec: err/exception --demo MinimalCustomException
    :source-sha256: b6b4928d7da851abf036df76cf5fb8f794c4a3234a17e2b6045fcee9988076d9

.. code-block:: cpp

    /// A custom exception can be as small as a named type derived from the appropriate neutral branch.
    /// Inheriting the base constructors preserves the standard reason and optional cause handling.
    class InstrumentError final : public el::RuntimeError {
    public:
        using RuntimeError::RuntimeError;
        ~InstrumentError() override = default;
    };

    /// A named exception lets callers recover specifically without parsing an error message.
    void minimalCustomException() {
        try {
            throw InstrumentError{"The koto is missing one string."_el};
        } catch (const InstrumentError &error) {
            el::io::printLine("Instrument setup: "_el, error.reason());
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Instrument setup: The koto is missing one string.

.. erbsland-demo-end::

Prefer one exception type for a coherent family of failures.
Put machine-readable reasons or operation details in its context instead of creating a separate derived class for every
message.

Keep Details in a Context Object
================================

For a richer error, create a value type named ``MyErrorContext`` and let ``MyError`` own it.
The context can collect a title, description, source information, operation identifiers, or domain-specific values
before code decides to throw.

This pattern has practical benefits: constructors stay short, tests can prepare and inspect error data directly,
translation layers can add context without formatting strings, and another domain can retain the context while wrapping
the failure.

Provide a short exception constructor for the common case, such as ``MyError{"reason"_el}``, and an overload that
accepts the context for detailed reports.
Let context setters return ``MyErrorContext&`` so callers can start with the same concise reason and fluently add only
the available details before transferring the context into the exception.

.. erbsland-demo::
    :source: err/Exception/ContextPattern.cpp
    :exec: err/exception --demo ContextPattern
    :source-sha256: 727560a39d566e74c65cdbf13d48cc2924f314acf64aa165dadf0d4aeec44341

.. code-block:: cpp

    /// A context object keeps structured error data independent from the act of throwing.
    /// It is easy to prepare, pass between layers, test, and later embed in a richer diagnostic.
    class InstrumentErrorContext final {
    public:
        explicit InstrumentErrorContext(el::StringView problem) : _problem{std::move(problem)} {}

    public: // accessors
        auto setInstrument(el::StringView instrument) noexcept -> InstrumentErrorContext & {
            _instrument = std::move(instrument);
            return *this;
        }
        [[nodiscard]] auto instrument() const noexcept -> const el::StringView & { return _instrument; }
        auto setStringNumber(const int stringNumber) noexcept -> InstrumentErrorContext & {
            _stringNumber = stringNumber;
            return *this;
        }
        [[nodiscard]] auto stringNumber() const noexcept -> int { return _stringNumber; }
        [[nodiscard]] auto problem() const noexcept -> const el::StringView & { return _problem; }

    private:
        el::StringView _problem;
        el::StringView _instrument;
        int _stringNumber{};
    };

    /// The exception accepts the complete context in one constructor and exposes it without duplicating accessors.
    class InstrumentSetupError final : public el::RuntimeError {
    public:
        explicit InstrumentSetupError(el::StringView problem) : InstrumentSetupError{InstrumentErrorContext{problem}} {}
        explicit InstrumentSetupError(InstrumentErrorContext context) :
            RuntimeError{context.problem()}, _context{std::move(context)} {}
        ~InstrumentSetupError() override = default;

    public: // accessors
        [[nodiscard]] auto context() const noexcept -> const InstrumentErrorContext & { return _context; }

    private:
        InstrumentErrorContext _context;
    };

    /// Build the context where all details are known, then transfer it into the exception.
    void contextPattern() {
        const auto simpleError = InstrumentSetupError{"The instrument could not be prepared."_el};
        el::io::printLine("Simple: "_el, simpleError.reason());

        try {
            throw InstrumentSetupError{InstrumentErrorContext{"The string could not be tuned."_el}
                                           .setInstrument("三味線"_el)
                                           .setStringNumber(2)};
        } catch (const InstrumentSetupError &error) {
            el::io::printLine(
                "Detailed: "_el,
                error.context().instrument(),
                " string "_el,
                error.context().stringNumber(),
                ": "_el,
                error.context().problem());
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Simple: The instrument could not be prepared.
    Detailed: 三味線 string 2: The string could not be tuned.

.. erbsland-demo-end::

Let the exception reason be a concise description that remains useful through ``what()`` and ``toString()``.
Keep additional fields in the context and expose the complete context through a const accessor.
If the custom exception can wrap another failure, pass its ``std::exception_ptr`` to the ``RuntimeError`` or
``LogicError`` constructor.

Translate Context into a Diagnostic
===================================

Override :cpp:func:`Exception::diagnostic() <erbsland::err::Exception::diagnostic>` when the context contains more than
a plain reason.
Return an immutable shared :cpp:class:`Diagnostic <erbsland::err::Diagnostic>` that owns every value it needs; callers
may retain it after the exception object is gone.

The custom diagnostic should override source accessors when applicable and implement
:cpp:func:`Diagnostic::toTextDocument() <erbsland::err::Diagnostic::toTextDocument>`. Use
:cpp:class:`ErrorDocumentBuilder <erbsland::err::ErrorDocumentBuilder>` to create the common error title, optional
description, source section, display texts, and root style.
Add domain details as semantic ``TextNode`` structures.

.. erbsland-demo::
    :source: err/ErrorDocumentBuilder/CustomDiagnostic.cpp
    :exec: err/error_document_builder --demo CustomDiagnostic
    :source-sha256: e741ad47f5f00dc1f3339d470d237b3ce86115650fd3e2a5b1255368106a8616

.. code-block:: cpp

    /// A custom diagnostic translates domain context into a semantic document.
    /// `ErrorDocumentBuilder` supplies the common error title, source fields, display texts, and document styling.
    class InstrumentDiagnostic final : public el::Diagnostic {
    public:
        InstrumentDiagnostic(el::StringView instrument, el::StringView problem, el::StringView sourcePath) :
            _instrument{std::move(instrument)}, _problem{std::move(problem)}, _sourcePath{std::move(sourcePath)} {}

    public: // implement Diagnostic
        [[nodiscard]] auto sourcePath() const noexcept -> el::StringView override { return _sourcePath; }
        [[nodiscard]] auto toTextDocument(const el::DisplayTextMapConstPtr &displayText) const
            -> el::TextDocument override {
            auto builder = el::ErrorDocumentBuilder{_problem, {}, displayText};
            builder.addSource({}, _sourcePath, {});
            builder.addSection("Instrument"_el);
            auto paragraph = builder.root()->addParagraph();
            paragraph->addText("Subject: "_el);
            paragraph->addEscapedText(_instrument, el::EscapeFormat::Display);
            return builder.takeDocument();
        }

    private:
        el::StringView _instrument;
        el::StringView _problem;
        el::StringView _sourcePath;
    };

    /// A domain exception owns its context and creates the matching immutable diagnostic on demand.
    class InstrumentError final : public el::RuntimeError {
    public:
        InstrumentError(el::StringView instrument, el::StringView problem, el::StringView sourcePath) :
            RuntimeError{problem},
            _instrument{std::move(instrument)},
            _sourcePath{std::move(sourcePath)} {}
        ~InstrumentError() override = default;

    public: // implement Exception
        [[nodiscard]] auto diagnostic() const -> el::DiagnosticConstPtr override {
            return std::make_shared<InstrumentDiagnostic>(_instrument, reason(), _sourcePath);
        }

    private:
        el::StringView _instrument;
        el::StringView _sourcePath;
    };

    /// Reporting code remains independent from the custom exception and diagnostic implementations.
    void customDiagnostic() {
        const auto error = InstrumentError{"箏\x1b[31m"_el, "The instrument could not be prepared."_el, "舞台/春/箏.music"_el};
        auto document = el::DiagnosticHelper{error}.toDocument();
        el::io::printLine(document.toString());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Error: The instrument could not be prepared.
    Error Source:
      Path: 舞台/春/箏.music
    Instrument:
      Subject: 箏\033[31m

.. erbsland-demo-end::

Keep the diagnostic renderer-neutral.
Do not insert ANSI sequences or align fields with spaces.
Use semantic nodes and let plain-text, terminal, or future renderers choose their presentation.

Handle External Text Safely
===========================

Developer-authored titles, descriptions, and labels are trusted application text.
Paths, arguments, native messages, file contents, and foreign exception messages may contain control characters.
Add such values with escaped text nodes, as the custom diagnostic above does with ``addEscapedText()``.
Use a semantic style for values such as paths when a renderer should treat them specially.

Pass the display-text map received by ``toTextDocument()`` into ``ErrorDocumentBuilder``.
This allows the reporting boundary to choose structural wording consistently, including when your diagnostic appears
inside a cause chain from a different domain.

With these boundaries in place, the exception controls failure propagation, the context stores domain facts, the
diagnostic describes those facts semantically, and the final renderer controls presentation.
