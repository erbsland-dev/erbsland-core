// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../EventEditor.hpp"

namespace erbsland::event::impl {

/// Common source and target storage for event editors.
/// The source is weakly held to avoid a cycle with its source-owned editor. The target is retained so generic editor
/// code can safely retain either object returned by the common interface.
/// @tested{EventSourceTest}
class CommonEventEditor : public EventEditor {
public:
    // defaults
    ~CommonEventEditor() override = default;

public: // implement EventEditor
    [[nodiscard]] auto source() const -> EventSourcePtr override;
    [[nodiscard]] auto target() const noexcept -> EventTargetPtr override;

protected:
    /// Create common storage for a source-owned editor.
    /// @param source The source that owns the editor. Must not be null.
    /// @param target The source owner and callback target. Must not be null and must own `source`.
    CommonEventEditor(EventSourcePtr source, EventTargetPtr target);

private:
    EventSourceWeakPtr _source; ///< Non-owning source access that prevents an ownership cycle.
    EventTargetPtr _target;     ///< Retained callback target.
};

}
