// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ApplicationError.hpp"

#include "../text/StringFormat.hpp"

namespace erbsland::err {

text::StringView ApplicationError::toString() const noexcept {
    static auto messageFormat = text::StringFormat{"{} (exit code: {})"};
    return messageFormat.build(reason(), _exitCode);
}

}
