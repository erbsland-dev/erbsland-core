// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SubprocessEventEditor.hpp"

namespace erbsland::system {

SubprocessEventEditor::SubprocessEventEditor(event::EventSourcePtr source, event::EventTargetPtr target) :
    CommonEventEditor{std::move(source), std::move(target)} {
}

}
