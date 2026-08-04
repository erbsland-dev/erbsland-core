// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Subprocess.hpp"

#include "PlatformError.hpp"

#include "impl/SubprocessBackend.hpp"

#include "../err/ParameterError.hpp"
#include "../text/Literals.hpp"
#include "../text/StringConverter.hpp"

#include <chrono>
#include <string>
#include <thread>

namespace erbsland::system {
using namespace text::literals;

Subprocess::Subprocess(impl::SubprocessBackendPtr backend) noexcept : _backend{std::move(backend)} {
}

Subprocess::~Subprocess() {
    cleanup();
}

Subprocess::Subprocess(Subprocess &&other) noexcept : _backend{std::move(other._backend)} {
}

auto Subprocess::operator=(Subprocess &&other) noexcept -> Subprocess & {
    if (this != &other) {
        cleanup();
        _backend = std::move(other._backend);
    }
    return *this;
}

auto Subprocess::start(
    const path::Path &executable, const text::StringList &arguments, const SubprocessOptions &options) -> Subprocess {
    validate(executable, arguments, options, false);
    return Subprocess{impl::createSubprocessBackend(executable, arguments, options)};
}

void Subprocess::startDetached(
    const path::Path &executable, const text::StringList &arguments, const SubprocessOptions &options) {
    validate(executable, arguments, options, true);
    auto backend = impl::createSubprocessBackend(executable, arguments, options);
    std::thread{[backend = std::move(backend)]() mutable -> void {
        try {
            backend->wait();
        } catch (...) {
            // A detached child has no observer to receive a wait failure.
        }
    }}.detach();
}

auto Subprocess::isRunning() -> bool {
    return _backend != nullptr && _backend->isRunning();
}

auto Subprocess::exitStatus() const noexcept -> const std::optional<SubprocessExitStatus> & {
    static const auto cNoStatus = std::optional<SubprocessExitStatus>{};
    return _backend == nullptr ? cNoStatus : _backend->exitStatus();
}

auto Subprocess::wait() -> SubprocessExitStatus {
    if (_backend == nullptr) {
        throw err::ParameterError{"Cannot wait for a moved-from subprocess."_el, "subprocess"_el};
    }
    return _backend->wait();
}

auto Subprocess::wait(const time::TimeDelta timeout) -> std::optional<SubprocessExitStatus> {
    if (_backend == nullptr) {
        throw err::ParameterError{"Cannot wait for a moved-from subprocess."_el, "subprocess"_el};
    }
    if (timeout.isNegative()) {
        throw err::ParameterError{"The subprocess wait timeout must not be negative."_el, "timeout"_el};
    }
    return _backend->wait(timeout);
}

void Subprocess::terminate() {
    if (_backend != nullptr) {
        _backend->terminate();
    }
}

void Subprocess::kill() {
    if (_backend != nullptr) {
        _backend->kill();
    }
}

auto Subprocess::standardOutput() const -> text::String {
    return _backend == nullptr ? text::String{} : _backend->standardOutput();
}

auto Subprocess::standardError() const -> text::String {
    return _backend == nullptr ? text::String{} : _backend->standardError();
}

auto Subprocess::wasStandardOutputTruncated() const noexcept -> bool {
    return _backend != nullptr && _backend->wasStandardOutputTruncated();
}

auto Subprocess::wasStandardErrorTruncated() const noexcept -> bool {
    return _backend != nullptr && _backend->wasStandardErrorTruncated();
}

void Subprocess::cleanup() noexcept {
    if (_backend == nullptr) {
        return;
    }
    try {
        if (_backend->isRunning()) {
            _backend->terminate();
            if (!_backend->wait(time::TimeDelta::milliseconds(500)).has_value()) {
                _backend->kill();
                _backend->wait();
            }
        }
    } catch (...) {
        try {
            _backend->kill();
            _backend->wait();
        } catch (...) {
            // Destruction cannot report a native cleanup failure.
        }
    }
    _backend.reset();
}

void Subprocess::validate(
    const path::Path &executable,
    const text::StringList &arguments,
    const SubprocessOptions &options,
    const bool detached) {
    if (executable.isEmpty() || !executable.isValid()) {
        throw err::ParameterError{"The subprocess executable path must be non-empty and valid."_el, "executable"_el};
    }
    for (const auto &argument : arguments) {
        if (!argument.isValidUtf8() || text::StringConverter{argument}.toStdString().find('\0') != std::string::npos) {
            throw err::ParameterError{
                "Subprocess arguments must be valid UTF-8 without null characters."_el, "arguments"_el};
        }
    }
    if (options.workingDirectory().has_value() &&
        (options.workingDirectory()->isEmpty() || !options.workingDirectory()->isValid())) {
        throw err::ParameterError{
            "The subprocess working directory must be non-empty and valid."_el, "workingDirectory"_el};
    }
    if (options.captureLimit().isZero() || options.captureLimit() > SubprocessOptions::cMaximumCaptureLimit) {
        throw err::ParameterError{
            "The subprocess capture limit must be positive and within the supported maximum."_el, "captureLimit"_el};
    }
    for (const auto &[name, value] : options.environmentChanges()) {
        const auto nativeName = text::StringConverter{name}.toStdString();
        if (name.isEmpty() || !name.isValidUtf8() || nativeName.find('=') != std::string::npos ||
            nativeName.find('\0') != std::string::npos) {
            throw err::ParameterError{
                "Subprocess environment names must be non-empty valid UTF-8 without '=' or null characters."_el,
                "environment"_el};
        }
        if (value.has_value() &&
            (!value->isValidUtf8() || text::StringConverter{*value}.toStdString().find('\0') != std::string::npos)) {
            throw err::ParameterError{
                "Subprocess environment values must be valid UTF-8 without null characters."_el, "environment"_el};
        }
    }
    if (detached &&
        (options.standardOutputMode() == SubprocessOutputMode::Capture ||
            options.standardErrorMode() == SubprocessOutputMode::Capture)) {
        throw err::ParameterError{"Detached subprocesses cannot capture output."_el, "options"_el};
    }
}

}
