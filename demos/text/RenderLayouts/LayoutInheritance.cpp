// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/text/render/Context.hpp>
#include <erbsland/text/render/Environment.hpp>
#include <erbsland/text/render/FileSystemLoader.hpp>

namespace demo {

/// Give a document a default passage with a named block.
///
/// A block renders its own content when no derived layout overrides it. This makes a complete base layout useful on
/// its own while leaving a clear place for a related document to supply different content.
/// @notest{Demo function verified by the documentation executable.}
void blockDefaults() {
    const auto directory = el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
    (directory->path() / "sound_note.txt"_el)
        .content()
        .writeTextOrThrow(
            "声学笔记: {{ subject }}\n"
            "{% block observation %}观察: 尚无记录。\n{% endblock %}"
            "结论: 请比较声源与回声。\n"_el);

    // Render the base layout directly to use its default observation.
    const auto environment = el::render::Environment::create();
    environment->addLayoutLoader(el::render::FileSystemLoader::create(directory->path()));
    el::io::print(environment->render("sound_note.txt"_el, el::render::Context{}.set("subject"_el, "山谷回声"_el)));
}

/// Reuse a document shape by extending a base layout.
///
/// The derived layout replaces the observation block and extends the base interpretation with super(). The title still
/// comes from the render context and the surrounding document still comes from the base layout.
/// @notest{Demo function verified by the documentation executable.}
void layoutInheritance() {
    const auto directory = el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
    (directory->path() / "base.txt"_el)
        .content()
        .writeTextOrThrow(
            "声学笔记: {{ subject }}\n"
            "{% block observation %}观察: 尚无记录。\n{% endblock %}"
            "{% block interpretation %}解释: 声波遇到表面会反射。\n{% endblock %}"_el);
    (directory->path() / "echo.txt"_el)
        .content()
        .writeTextOrThrow(
            "{% extends \"base.txt\" %}"
            "{% block observation %}观察: 拍手后听到两次回声。\n{% endblock %}"
            "{% block interpretation %}{{ super() }}补充: 两次反射来自不同的岩壁。\n{% endblock %}"_el);

    // Render the derived layout by name; the base supplies the surrounding text.
    const auto environment = el::render::Environment::create();
    environment->addLayoutLoader(el::render::FileSystemLoader::create(directory->path()));
    el::io::print(environment->render("echo.txt"_el, el::render::Context{}.set("subject"_el, "山谷回声"_el)));
}

/// Select an earlier block implementation in a three-layout chain.
///
/// super() inserts the next block implementation, while super.super() reaches the one after it. Both calls belong
/// inside a block and can be used to keep or bypass an intermediate specialization.
/// @notest{Demo function verified by the documentation executable.}
void chainedInheritance() {
    const auto directory = el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
    (directory->path() / "base.txt"_el)
        .content()
        .writeTextOrThrow("{% block note %}基础: 声波可以反射。\n{% endblock %}"_el);
    (directory->path() / "outdoors.txt"_el)
        .content()
        .writeTextOrThrow(
            "{% extends \"base.txt\" %}"
            "{% block note %}{{ super() }}户外: 山坡也会反射声波。\n{% endblock %}"_el);
    (directory->path() / "valley.txt"_el)
        .content()
        .writeTextOrThrow(
            "{% extends \"outdoors.txt\" %}"
            "{% block note %}{{ super() }}直达基础: {{ super.super() }}{% endblock %}"_el);

    // The final layout can use both earlier block implementations.
    const auto environment = el::render::Environment::create();
    environment->addLayoutLoader(el::render::FileSystemLoader::create(directory->path()));
    el::io::print(environment->render("valley.txt"_el));
}

}
