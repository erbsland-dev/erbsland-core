// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/StringMap.hpp>
#include <tool/frame/PreCommitTask.hpp>
#include <tool/frame/Tool.hpp>

#include <functional>

namespace test::impl {

/// Shared task fixture recording preparation and execution of one command instance.
class PreCommitProbe : public tool::frame::Tool, public tool::frame::PreCommitTask {
public:
    /// Count construction to detect duplicate command/task ownership.
    PreCommitProbe() { ++created; }

public: // implement Tool and PreCommitTask
    [[nodiscard]] auto identifier() const -> el::String override = 0;
    [[nodiscard]] auto title() const -> el::String override { return identifier(); }
    [[nodiscard]] auto description() const -> el::String override;
    [[nodiscard]] auto usedDataSources() const -> el::StringList override;
    [[nodiscard]] auto optionModule() const -> el::OptionModulePtr override;
    void configure(const el::conf::ValuePtr &configuration, const el::OptionValuesPtr &values) override;
    void initialize() override;
    void execute() override;

public: // test state
    /// Reset callbacks and recorded observations before a test.
    static void reset();
    static inline std::size_t created{};              ///< Number of instances constructed.
    static inline el::StringList sourceUsers;         ///< Probes requiring the shared source fixture.
    static inline bool progressActive{};              ///< Whether the last executed context retained progress.
    static inline bool burstLogging{};                ///< Queue a burst to exercise draining.
    static inline el::StringList events;              ///< Ordered lifecycle observations.
    static inline el::StringMap<el::String> failures; ///< Identifier to failing lifecycle phase.
    static inline el::StringMap<el::String> messages; ///< Identifier to emitted log level.
    static inline std::function<void()> action;       ///< Optional execution hook.
    static inline bool defaultsApplied{};             ///< Whether the declared option default was parsed.
    static inline const PreCommitProbe *configured{}; ///< Last prepared instance.
    static inline const PreCommitProbe *executed{};   ///< Last executed instance.

private:
    /// Record a lifecycle phase and raise its configured operational failure.
    void phase(const el::String &name);
};

}
