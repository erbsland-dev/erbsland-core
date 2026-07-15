// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StreamErrorSource.hpp"

#include "StreamError.hpp"

#include <utility>

namespace erbsland::stream {

void StreamErrorSource::throwError(const text::StringView title, const text::StringView description) const {
    auto context = createErrorContext();
    context.setTitle(title).setDescription(description);
    throw StreamError{std::move(context)};
}

auto StreamErrorSource::createErrorContext() const noexcept -> StreamErrorContext {
    return StreamErrorContext{{}, {}};
}

}
