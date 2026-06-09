// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StandardStreamRegistry.hpp"

#include "NativeOutputStream.hpp"
#include "StandardStreamProxy.hpp"
#include "StandardStreamRedirectData.hpp"
#include "StandardTextOutputStream.hpp"

#include "../../err/StreamError.hpp"

namespace erbsland::stream::impl {

auto StandardStreamRegistry::outputProxy() -> TextOutputStreamPtr {
    auto lock = std::scoped_lock{_mutex};
    if (_outputProxy == nullptr) {
        _outputProxy = std::make_shared<StandardStreamProxy>(StandardStreamSlot::Out);
    }
    return _outputProxy;
}

auto StandardStreamRegistry::errorProxy() -> TextOutputStreamPtr {
    auto lock = std::scoped_lock{_mutex};
    if (_errorProxy == nullptr) {
        _errorProxy = std::make_shared<StandardStreamProxy>(StandardStreamSlot::Err);
    }
    return _errorProxy;
}

auto StandardStreamRegistry::outputTarget() -> TextOutputStreamPtr {
    auto lock = std::scoped_lock{_mutex};
    if (_outputTarget == nullptr) {
        _outputTarget =
            std::make_shared<StandardTextOutputStream>(createNativeStandardOutputStream(NativeStandardStream::Out));
    }
    return _outputTarget;
}

auto StandardStreamRegistry::errorTarget() -> TextOutputStreamPtr {
    auto lock = std::scoped_lock{_mutex};
    if (_errorTarget == nullptr) {
        _errorTarget =
            std::make_shared<StandardTextOutputStream>(createNativeStandardOutputStream(NativeStandardStream::Err));
    }
    return _errorTarget;
}

auto StandardStreamRegistry::replace(
    const StandardStreamSlot slot, TextOutputStreamPtr output, TextOutputStreamPtr error)
    -> std::shared_ptr<StandardStreamRedirectData> {
    auto lock = std::scoped_lock{_mutex};
    if ((slot == StandardStreamSlot::Out || slot == StandardStreamSlot::Both) && output == nullptr) {
        throw err::StreamError{"The replacement standard output stream must not be empty."};
    }
    if ((slot == StandardStreamSlot::Err || slot == StandardStreamSlot::Both) && error == nullptr) {
        throw err::StreamError{"The replacement standard error stream must not be empty."};
    }
    if (output != nullptr && output == _outputProxy) {
        throw err::StreamError{"The standard output proxy cannot replace itself."};
    }
    if (error != nullptr && error == _errorProxy) {
        throw err::StreamError{"The standard error proxy cannot replace itself."};
    }

    auto previousOutput = _outputTarget;
    auto previousError = _errorTarget;
    if (slot == StandardStreamSlot::Out || slot == StandardStreamSlot::Both) {
        if (previousOutput == nullptr) {
            previousOutput =
                std::make_shared<StandardTextOutputStream>(createNativeStandardOutputStream(NativeStandardStream::Out));
        }
        _outputTarget = std::move(output);
    }
    if (slot == StandardStreamSlot::Err || slot == StandardStreamSlot::Both) {
        if (previousError == nullptr) {
            previousError =
                std::make_shared<StandardTextOutputStream>(createNativeStandardOutputStream(NativeStandardStream::Err));
        }
        _errorTarget = std::move(error);
    }
    return std::make_shared<StandardStreamRedirectData>(slot, std::move(previousOutput), std::move(previousError));
}

void StandardStreamRegistry::restore(
    const StandardStreamSlot slot, TextOutputStreamPtr output, TextOutputStreamPtr error) noexcept {
    auto lock = std::scoped_lock{_mutex};
    if (slot == StandardStreamSlot::Out || slot == StandardStreamSlot::Both) {
        _outputTarget = std::move(output);
    }
    if (slot == StandardStreamSlot::Err || slot == StandardStreamSlot::Both) {
        _errorTarget = std::move(error);
    }
}

auto standardStreamRegistry() -> StandardStreamRegistry & {
    static auto result = StandardStreamRegistry{};
    return result;
}

}
