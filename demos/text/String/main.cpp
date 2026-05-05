// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "StringDemos.hpp"

#include <DemoCommon.hpp>

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("AlignAndTruncate"_el, alignAndTruncate);
    app.registerDemo("ClearResetAndAppend"_el, clearResetAndAppend);
    app.registerDemo("CopyOnWrite"_el, copyOnWrite);
    app.registerDemo("EditingText"_el, editingText);
    app.registerDemo("InsertAndReplace"_el, insertAndReplace);
    app.registerDemo("RemoveAndKeepRanges"_el, removeAndKeepRanges);
    app.registerDemo("RemoveFirstAndAll"_el, removeFirstAndAll);
    app.registerDemo("ManualDetach"_el, manualDetach);
    app.registerDemo("ReserveForAppend"_el, reserveForAppend);
    app.registerDemo("ShrinkSlices"_el, shrinkSlices);
    return app.run();
}
