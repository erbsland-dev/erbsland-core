..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Text Rendering; Loaders
    single: Layouts; File System Loader
    single: Layouts; Resource Loader
    single: Layouts; Custom Loader

**********************
Loading Render Layouts
**********************

A render call names a layout, but it does not need to know where that layout is stored.
A :cpp:class:`Loader <erbsland::text::render::Loader>` connects the logical name to source text.
You can keep layouts in an editable directory, compile them into the application, or supply them from another source
that your application owns.
This page shows each approach and explains how several loaders can work together.

Names That Stay the Same Across Sources
=======================================

The environment passes a validated logical name such as ``lessons/tempo.txt`` to its loaders.
Names use lowercase ASCII letters, digits, ``-``, ``_``, ``.``, and ``/``.
They cannot start or end with a slash, contain empty path components, or exceed 200 code points.
The name is the same whether the source lives on disk or in a compiled resource set.
Includes and inherited layouts use these names too, so a layout tree can move between storage choices without editing
its references.

A loader returns a :cpp:class:`LayoutSource <erbsland::text::render::LayoutSource>` when it finds the name, or
``std::nullopt`` when it does not.
The environment tries the next loader after a miss.
An actual loading error is reported as :cpp:class:`RenderError <erbsland::text::render::RenderError>` during rendering;
it is not treated as a miss.

Use Files While Editing Layouts
===============================

:cpp:class:`FileSystemLoader <erbsland::text::render::FileSystemLoader>` looks below one or more existing, absolute
directories.
It maps ``lessons/tempo.txt`` to the corresponding relative file below each directory.
When given several directories, it searches them in the order supplied.
The following demo uses two directories with the same layout name; the first one wins.

.. erbsland-demo::
    :source: text/RenderLayouts/LoaderExamples.cpp
    :function-blocks: fileLoader
    :function-blocks-sha256: 240c25a959f5d5426bde069c1096df2ea63395126a72b85ccdf10ae260a8fabb
    :exec: text/render_layouts --demo FileLoader
    :source-sha256: a27ec7b7cd8665f31f15276ca82bc7b4c0901ed49c67d3443348665eb0f59783

.. code-block:: cpp

    void fileLoader() {
        const auto first = el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
        const auto second = el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
        (first->path() / "lesson.txt"_el).content().writeTextOrThrow("İlk alıştırma: {{ pattern }}\n"_el);
        (second->path() / "lesson.txt"_el).content().writeTextOrThrow("İkinci alıştırma: {{ pattern }}\n"_el);

        const auto environment = el::render::Environment::create();
        environment->addLayoutLoader(
            el::render::FileSystemLoader::create(el::util::List<el::Path>{first->path(), second->path()}));
        el::io::print(environment->render("lesson.txt"_el, el::render::Context{}.set("pattern"_el, "yavaş-hızlı"_el)));
    }

.. erbsland-ansi::
    :escape-char: ␛

    İlk alıştırma: yavaş-hızlı

.. erbsland-demo-end::

Use an absolute directory that your application knows, rather than depending on its current working directory.
The loader only accepts regular files below its roots, does not traverse symbolic links below a root, and reads valid
UTF-8 layout text.
:cpp:class:`FileSystemLoaderOptions <erbsland::text::render::FileSystemLoaderOptions>` is accepted by the factory;
it currently has no configurable settings.

Use Layouts Compiled into the Application
=========================================

Compiled layouts are useful when the application ships a stable set of defaults.
:cpp:class:`ResourceLoader <erbsland::text::render::ResourceLoader>` resolves a name against an exact resource
identifier and an optional path prefix.
In this demo, the logical name ``note.txt`` becomes the resource key ``(render-layouts, tempo/note.txt)``.

The resource is added to the demo target at build time:

.. code-block:: cmake

    erbsland_core_add_resources(
            TARGET render_layouts
            DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/data"
            IDENTIFIER "render-layouts"
            RECURSIVE
            SUFFIXES ".txt"
    )

The runtime code still asks the environment for the logical name:

.. erbsland-demo::
    :source: text/RenderLayouts/LoaderExamples.cpp
    :function-blocks: resourceLoader
    :function-blocks-sha256: 3df427a4ad85a1e6491093291f242e72e5079f93b1b84cb51ad7ffe29da9c074
    :exec: text/render_layouts --demo ResourceLoader
    :source-sha256: a27ec7b7cd8665f31f15276ca82bc7b4c0901ed49c67d3443348665eb0f59783

.. code-block:: cpp

    void resourceLoader() {
        const auto environment = el::render::Environment::create();
        environment->addLayoutLoader(el::render::ResourceLoader::create("render-layouts"_el, "tempo"_el));
        el::io::print(environment->render(
            "note.txt"_el, el::render::Context{}.set("pattern"_el, "yavaş-hızlı"_el).set("bpm"_el, 84)));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Çalışma: yavaş-hızlı (84 BPM)

.. erbsland-demo-end::

The one-argument factory uses the application's resource manager.
An overload accepts an explicit ``ResourcesConstPtr`` when resources come from a provider you supply; the loader retains
that provider.
Create an application-backed loader after the application is initialized.
The optional prefix is a normalized relative path with ``/`` separators; it does not change the name passed to
``render()``.
For the build-time resource workflow, see :doc:`/topics/resource/compiled_resources`.

Combine Defaults with Local Overrides
=====================================

Register more than one loader when a user-editable layout should take precedence over a compiled default.
``addLayoutLoader(loader, priority)`` searches larger priorities first; equal priorities keep registration order.
For example, register a resource loader at the default priority, then a file-system loader with priority ``10``.
If the file exists, it is used; otherwise, the resource loader may provide the layout.

.. erbsland-demo::
    :source: text/RenderLayouts/LoaderExamples.cpp
    :function-blocks: layeredLoaders
    :function-blocks-sha256: baedd4e30ca770d323d0c019269dbe9e931b0a75f7e89a8559dbc45e09df9cca
    :exec: text/render_layouts --demo LayeredLoaders
    :source-sha256: a27ec7b7cd8665f31f15276ca82bc7b4c0901ed49c67d3443348665eb0f59783

.. code-block:: cpp

    void layeredLoaders() {
        const auto directory = el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
        (directory->path() / "note.txt"_el).content().writeTextOrThrow("Yerel: {{ pattern }}\n"_el);

        const auto environment = el::render::Environment::create();
        environment->addLayoutLoader(el::render::ResourceLoader::create("render-layouts"_el, "tempo"_el));
        environment->addLayoutLoader(el::render::FileSystemLoader::create(directory->path()), 10);
        const auto context = el::render::Context{}.set("pattern"_el, "yavaş-hızlı"_el);
        el::io::print(environment->render("note.txt"_el, context));
        el::io::print(environment->render("default.txt"_el, context));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Yerel: yavaş-hızlı
    Varsayılan: yavaş-hızlı

.. erbsland-demo-end::

This choice is made for every named layout, including :doc:`included layouts <template_feature_cheat_sheet>` and
:doc:`inherited parents <blocks_and_inheritance>`.
One local partial can therefore override its compiled counterpart without copying the entire layout tree.
When automatic reload is enabled, the environment checks sources again at render time and picks up changed revisions.

Supply Layouts from Your Own Source
===================================

You can implement :cpp:class:`Loader <erbsland::text::render::Loader>` for layouts held by an application service or
generated from another store.
The example below keeps one small layout in its loader.
The class appears with the demo because it is the part you would adapt for your own source.

.. erbsland-demo::
    :source: text/RenderLayouts/CustomLoader.cpp
    :exec: text/render_layouts --demo CustomLoader
    :source-sha256: b81f9712aa7e205f1e9c8f2729b3f00319dcdd34215a14bc4934b6f6591ad31c

.. code-block:: cpp

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

.. erbsland-ansi::
    :escape-char: ␛

    Alıştırma: yavaş-hızlı

.. erbsland-demo-end::

``load()`` receives a name that the environment has already validated.
Return ``std::nullopt`` only when your source has no layout by that name.
A present source needs its exact UTF-8 text, a stable origin for diagnostics, and a revision token that changes when its
content changes.
The loader may be called from concurrent renders, so its ``load()`` implementation must be thread-safe.

For the environment settings that accompany these loaders, continue with
:doc:`configuring_the_render_environment`.
The :doc:`rendering reference </reference/text/documents_and_rendering>` describes the values supplied to a loaded
layout.
