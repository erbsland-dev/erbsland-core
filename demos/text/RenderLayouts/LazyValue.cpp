// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/text/render/Context.hpp>
#include <erbsland/text/render/Environment.hpp>
#include <erbsland/text/render/FileSystemLoader.hpp>

namespace demo {

/// Supply a value that is computed only when a layout reads it.
///
/// A ValueCallbackFn produces a Value on demand. The layout below skips one callback and reads another; the
/// captured counter makes the difference visible when the demo runs.
/// @notest{Demo function verified by the documentation executable.}
void lazyValue() {
    using el::render::Value;
    using el::render::ValueCallbackFn;

    auto calculations = 0;
    auto context = el::render::Context{};
    context.set("region"_el, "altiplano"_el);
    context.set("classification"_el, ValueCallbackFn{[&calculations]() -> Value {
        ++calculations;
        return "meseta"_el;
    }});
    context.set("unused"_el, ValueCallbackFn{[]() -> Value { return "never requested"_el; }});

    const auto directory = el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
    (directory->path() / "classification.txt"_el).content().writeTextOrThrow("{{ region }}: {{ classification }}\n"_el);
    const auto environment = el::render::Environment::create();
    environment->addLayoutLoader(el::render::FileSystemLoader::create(directory->path()));

    el::io::printLine("Calculations before render: "_el, calculations);
    el::io::print(environment->render("classification.txt"_el, context));
    el::io::printLine("Calculations after render: "_el, calculations);
}

}
