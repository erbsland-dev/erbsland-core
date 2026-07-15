// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StandardStreamRegistry.hpp"

#include "EncodedTextInputStream.hpp"
#include "NativeOutputStream.hpp"
#include "StandardInputStreamProxy.hpp"
#include "StandardStreamProxy.hpp"
#include "StandardStreamRedirectData.hpp"
#include "StandardTextOutputStream.hpp"

#include "../StreamError.hpp"

#include "../../text/Literals.hpp"

namespace erbsland::stream::impl {

using namespace text::literals;

auto StandardStreamRegistry::inputProxy() -> TextInputStreamPtr {
    auto lock = std::scoped_lock{_mutex};
    if (_inputProxy == nullptr) {
        _inputProxy = std::make_shared<StandardInputStreamProxy>();
    }
    return _inputProxy;
}

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

auto StandardStreamRegistry::inputTarget() -> TextInputStreamPtr {
    auto lock = std::scoped_lock{_mutex};
    if (_inputTarget == nullptr) {
        _inputTarget =
            std::make_shared<EncodedTextInputStream>(createNativeStandardInputStream(), text::StringEncoding::Utf8);
    }
    return _inputTarget;
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
    const StandardStreamSlot slot, TextInputStreamPtr input, TextOutputStreamPtr output, TextOutputStreamPtr error)
    -> std::shared_ptr<StandardStreamRedirectData> {
    auto lock = std::scoped_lock{_mutex};
    if (slot == StandardStreamSlot::In && input == nullptr) {
        throw StreamError{StreamErrorContext{
            "Failed to replace the standard input stream."_el, "The replacement standard input stream is empty."_el}};
    }
    if ((slot == StandardStreamSlot::Out || slot == StandardStreamSlot::Both) && output == nullptr) {
        throw StreamError{StreamErrorContext{
            "Failed to replace the standard output stream."_el, "The replacement standard output stream is empty."_el}};
    }
    if ((slot == StandardStreamSlot::Err || slot == StandardStreamSlot::Both) && error == nullptr) {
        throw StreamError{StreamErrorContext{
            "Failed to replace the standard error stream."_el, "The replacement standard error stream is empty."_el}};
    }
    if (input != nullptr && input == _inputProxy) {
        throw StreamError{StreamErrorContext{
            "Failed to replace the standard input stream."_el, "The standard input proxy cannot replace itself."_el}};
    }
    if (output != nullptr && output == _outputProxy) {
        throw StreamError{StreamErrorContext{
            "Failed to replace the standard output stream."_el, "The standard output proxy cannot replace itself."_el}};
    }
    if (error != nullptr && error == _errorProxy) {
        throw StreamError{StreamErrorContext{
            "Failed to replace the standard error stream."_el, "The standard error proxy cannot replace itself."_el}};
    }

    auto previousInput = _inputTarget;
    auto previousOutput = _outputTarget;
    auto previousError = _errorTarget;
    if (slot == StandardStreamSlot::In) {
        if (previousInput == nullptr) {
            previousInput =
                std::make_shared<EncodedTextInputStream>(createNativeStandardInputStream(), text::StringEncoding::Utf8);
        }
        _inputTarget = std::move(input);
    }
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
    return std::make_shared<StandardStreamRedirectData>(
        slot, std::move(previousInput), std::move(previousOutput), std::move(previousError));
}

void StandardStreamRegistry::restore(
    const StandardStreamSlot slot,
    TextInputStreamPtr input,
    TextOutputStreamPtr output,
    TextOutputStreamPtr error) noexcept {
    auto lock = std::scoped_lock{_mutex};
    if (slot == StandardStreamSlot::In) {
        _inputTarget = std::move(input);
    }
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
