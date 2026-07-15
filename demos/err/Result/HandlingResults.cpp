// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

class CalibrationResult final : public el::Result {
public:
    using Result::Result;

public:
    static const CalibrationResult Calibrated;
    static const CalibrationResult AlreadyCalibrated;
    static const CalibrationResult Obstructed;
};

inline constexpr CalibrationResult CalibrationResult::Calibrated = Value::success<0>();
inline constexpr CalibrationResult CalibrationResult::AlreadyCalibrated = Value::success<1>();
inline constexpr CalibrationResult CalibrationResult::Obstructed = Value::failure<0>();

[[nodiscard]] auto calibrateModule(const el::StringView &module) noexcept -> CalibrationResult {
    if (module == "visão"_el) {
        return CalibrationResult::Calibrated;
    }
    if (module == "movimento"_el) {
        return CalibrationResult::AlreadyCalibrated;
    }
    return CalibrationResult::Obstructed;
}

/// Results support free-function and member-function tests.
/// The free functions accept derived result types, while an initializer keeps a multi-state result available for
/// inspecting its exact value.
void handlingResults() {
    // Test a temporary directly when only the success group matters.
    if (isSuccessful(calibrateModule("visão"_el))) {
        el::io::printLine("The vision module is calibrated."_el);
    }

    // Keep the result when the exact failure state determines the response.
    if (auto result = calibrateModule("garra"_el); isFailure(result)) {
        if (result == CalibrationResult::Obstructed) {
            el::io::printLine("The gripper is obstructed."_el);
        }
    }

    // The member form is equally readable for a single grouped decision.
    if (calibrateModule("movimento"_el).isSuccessful()) {
        el::io::printLine("The movement module is ready."_el);
    }
}

}
