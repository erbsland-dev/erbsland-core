// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/text/render/Environment.hpp>
#include <erbsland/text/render/Loader.hpp>

#include <memory>
#include <optional>

namespace demo {

/// Supply a layout source from application-owned storage.
///
/// A loader gets a validated logical name and returns a source only when it owns that name. This example
/// has one stable layout; a service-backed loader would look it up and derive a revision from its content.
/// @notest{Demo-only loader verified by the documentation executable.}
class LessonLoader final : public el::render::Loader {
public:
    [[nodiscard]] auto load(const el::String &layout) -> std::optional<el::render::LayoutSource> override {
        if (layout != "lesson.txt"_el) {
            return std::nullopt;
        }
        return el::render::LayoutSource{"Alıştırma: {{ pattern }}\n"_el, "lesson:built-in"_el, "1"_el};
    }
};

/// Supply layouts from a source owned by the application.
///
/// A custom Loader returns a LayoutSource for a known name and std::nullopt for other names. Its origin is
/// used in diagnostics, while its revision identifies changes when automatic reload is enabled.
/// @notest{Demo function verified by the documentation executable.}
void customLoader() {
    const auto environment = el::render::Environment::create();
    environment->addLayoutLoader(std::make_shared<LessonLoader>());
    el::io::print(environment->render("lesson.txt"_el, el::render::Context{}.set("pattern"_el, "yavaş-hızlı"_el)));
}

}
