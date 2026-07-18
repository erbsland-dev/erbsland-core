// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// A custom diagnostic translates domain context into a semantic document.
/// `ErrorDocumentBuilder` supplies the common error title, source fields, display texts, and document styling.
class InstrumentDiagnostic final : public el::Diagnostic {
public:
    InstrumentDiagnostic(el::String instrument, el::String problem, el::String sourcePath) :
        _instrument{std::move(instrument)}, _problem{std::move(problem)}, _sourcePath{std::move(sourcePath)} {}

public: // implement Diagnostic
    [[nodiscard]] auto sourcePath() const noexcept -> el::String override { return _sourcePath; }
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
    el::String _instrument;
    el::String _problem;
    el::String _sourcePath;
};

/// A domain exception owns its context and creates the matching immutable diagnostic on demand.
class InstrumentError final : public el::RuntimeError {
public:
    InstrumentError(el::String instrument, el::String problem, el::String sourcePath) :
        RuntimeError{problem}, _instrument{std::move(instrument)}, _sourcePath{std::move(sourcePath)} {}
    ~InstrumentError() override = default;

public: // implement Exception
    [[nodiscard]] auto diagnostic() const -> el::DiagnosticConstPtr override {
        return std::make_shared<InstrumentDiagnostic>(_instrument, reason(), _sourcePath);
    }

private:
    el::String _instrument;
    el::String _sourcePath;
};

/// Reporting code remains independent from the custom exception and diagnostic implementations.
void customDiagnostic() {
    const auto error =
        InstrumentError{"箏\x1b[31m"_el, "The instrument could not be prepared."_el, "舞台/春/箏.music"_el};
    auto document = el::DiagnosticHelper{error}.toDocument();
    el::io::printLine(document.toString());
}

}
