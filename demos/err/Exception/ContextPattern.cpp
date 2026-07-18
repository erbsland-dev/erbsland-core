// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// A context object keeps structured error data independent from the act of throwing.
/// It is easy to prepare, pass between layers, test, and later embed in a richer diagnostic.
class InstrumentErrorContext final {
public:
    explicit InstrumentErrorContext(el::String problem) : _problem{std::move(problem)} {}

public: // accessors
    auto setInstrument(el::String instrument) noexcept -> InstrumentErrorContext & {
        _instrument = std::move(instrument);
        return *this;
    }
    [[nodiscard]] auto instrument() const noexcept -> const el::String & { return _instrument; }
    auto setStringNumber(const int stringNumber) noexcept -> InstrumentErrorContext & {
        _stringNumber = stringNumber;
        return *this;
    }
    [[nodiscard]] auto stringNumber() const noexcept -> int { return _stringNumber; }
    [[nodiscard]] auto problem() const noexcept -> const el::String & { return _problem; }

private:
    el::String _problem;
    el::String _instrument;
    int _stringNumber{};
};

/// The exception accepts the complete context in one constructor and exposes it without duplicating accessors.
class InstrumentSetupError final : public el::RuntimeError {
public:
    explicit InstrumentSetupError(el::String problem) : InstrumentSetupError{InstrumentErrorContext{problem}} {}
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
        throw InstrumentSetupError{
            InstrumentErrorContext{"The string could not be tuned."_el}.setInstrument("三味線"_el).setStringNumber(2)};
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

}
