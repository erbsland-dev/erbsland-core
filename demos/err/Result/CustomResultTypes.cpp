// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Derive a custom result when callers need a small set of distinct, actionable outcomes.
/// Values created with `success<N>()` belong to the successful group, while `failure<N>()` values belong to the
/// failure group.
class RobotSetupResult final : public el::Result {
public:
    using Result::Result;

public:
    static const RobotSetupResult Ready;
    static const RobotSetupResult AlreadyReady;
    static const RobotSetupResult Obstructed;
    static const RobotSetupResult ControllerOffline;
};

inline constexpr RobotSetupResult RobotSetupResult::Ready = Value::success<0>();
inline constexpr RobotSetupResult RobotSetupResult::AlreadyReady = Value::success<1>();
inline constexpr RobotSetupResult RobotSetupResult::Obstructed = Value::failure<0>();
inline constexpr RobotSetupResult RobotSetupResult::ControllerOffline = Value::failure<1>();

[[nodiscard]] auto setUpRobot(const el::StringView &robotName) noexcept -> RobotSetupResult {
    if (robotName == "Lume"_el) {
        return RobotSetupResult::Ready;
    }
    if (robotName == "Brisa"_el) {
        return RobotSetupResult::AlreadyReady;
    }
    if (robotName == "Pedra"_el) {
        return RobotSetupResult::Obstructed;
    }
    return RobotSetupResult::ControllerOffline;
}

/// Group tests handle the common path, and equality tests distinguish the states that require different actions.
void customResultTypes() {
    if (isSuccessful(setUpRobot("Brisa"_el))) {
        el::io::printLine("Brisa is ready for the project."_el);
    }

    if (auto result = setUpRobot("Pedra"_el); isFailure(result)) {
        if (result == RobotSetupResult::Obstructed) {
            el::io::printLine("Remove the obstacle from Pedra."_el);
        } else if (result == RobotSetupResult::ControllerOffline) {
            el::io::printLine("Connect the Pedra controller."_el);
        }
    }
}

}
