// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StreamErrorSource.hpp"

#include "StreamError.hpp"

#include <utility>

namespace erbsland::stream {

void StreamErrorSource::throwError(text::String title, text::String description) const {
    auto context = createErrorContext();
    context.setTitle(std::move(title)).setDescription(std::move(description));
    throw StreamError{std::move(context)};
}

auto StreamErrorSource::createErrorContext() const noexcept -> StreamErrorContext {
    return StreamErrorContext{{}, {}};
}

}
