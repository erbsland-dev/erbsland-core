..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Text Rendering; Workflow
    single: Layouts; Loading
    single: Render Context; Preparing

***********************
Using the Render Engine
***********************

Once a layout exists, rendering it is a short sequence: configure an environment, give it a way to find the layout,
prepare the values for this document, and request the result.
This page takes you through that sequence with a file-backed layout.

A Complete Rendering Pass
=========================

The example creates a ``rhythm.txt`` layout, registers the directory that contains it, and renders a practice note.
It also asks for a missing layout so you can see where a loading failure enters the workflow.
The temporary directory keeps the demo independent of the current working directory; your application can use any
existing absolute layout directory.

.. erbsland-demo::
    :source: text/RenderLayouts/RenderWorkflow.cpp
    :exec: text/render_layouts --demo RenderWorkflow
    :source-sha256: e3e405d69ef21a4755db709ac0d999b9a5b7e77921f4726e0d5e50daed8e47da

.. code-block:: cpp

    /// Load a named layout and render it with a fresh local context.
    ///
    /// Create and configure the environment once. The file-system loader resolves the logical layout name below its
    /// root directory. Each render call receives its own context, so the same layout can produce different documents.
    void renderWorkflow() {
        const auto temporary = el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
        const auto layout = temporary->path() / "rhythm.txt"_el;
        layout.content().writeTextOrThrow(
            "Esercizio: {{ title }}\n"
            "Tempo: {{ bpm }} BPM\n"
            "{% if practice %}Ripeti lentamente.\n{% endif %}"_el);

        // A loader maps logical names to files below its absolute root directory.
        const auto environment = el::render::Environment::create();
        environment->addLayoutLoader(el::render::FileSystemLoader::create(temporary->path()));

        // Prepare a separate local context for this rendering.
        auto context = el::render::Context{};
        context.set("title"_el, "Tre battiti"_el).set("bpm"_el, 72).set("practice"_el, true);
        el::io::print(environment->render("rhythm.txt"_el, context));

        // A missing layout is reported as a render error.
        try {
            el::io::print(environment->render("missing.txt"_el, context));
        } catch (const el::render::RenderError &error) {
            el::io::printLine("Could not render: "_el, error.context().layout());
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Esercizio: Tre battiti
    Tempo: 72 BPM
    Ripeti lentamente.
    Could not render: missing.txt

.. erbsland-demo-end::

The rendered string is available directly from ``render()``.
You can write it to a file, send it to an output stream, or keep it as an ordinary
:cpp:type:`String <erbsland::text::String>`.

Configure the Environment
=========================

:cpp:class:`Environment <erbsland::text::render::Environment>` is the entry point for rendering.
``Environment::create()`` uses the default syntax: ``{{ ... }}`` for expressions, ``{% ... %}`` for statements, and
``{# ... #}`` for comments.
When you need different delimiters, escaping, or rendering limits, create
:cpp:class:`EnvironmentOptions <erbsland::text::render::EnvironmentOptions>` and pass it to ``create()``.

Add loaders and filters before the first render.
Their setup is not thread-safe, while rendering with a configured environment is thread-safe.
This makes it practical to keep one environment and use it from several requests, with a separate local context for each
request.

Find the Layout by Name
=======================

The :cpp:class:`FileSystemLoader <erbsland::text::render::FileSystemLoader>` maps a logical layout name to a file below
an existing absolute directory.
In the demo, the name ``rhythm.txt`` finds the file at the root of the temporary directory.
A name such as ``lessons/rhythm.txt`` would look in its ``lessons`` subdirectory.
The name is a portable identifier, not an arbitrary path: it uses lowercase ASCII letters, digits, ``-``, ``_``, ``.``,
and ``/``, with no leading or trailing slash or empty path component.

``addLayoutLoader()`` registers the loader with the environment.
You can register more than one loader, for example to let a directory of local layouts take precedence over compiled
defaults.
Higher priority is searched first; loaders with equal priority keep their registration order.
The loader concept and the compiled-resource option are explained further in the
:doc:`rendering reference </reference/text/documents_and_rendering>`.

Supply Values for This Document
===============================

A :cpp:class:`Context <erbsland::text::render::Context>` maps names to immutable
:cpp:class:`Value <erbsland::text::render::Value>` objects.
In the demo, ``set()`` adds the title, tempo, and practice flag.
The value constructors accept Core strings, integers, floating-point values, booleans, lists, and maps, so the ordinary
scalar arguments in this example convert naturally.

The context passed to ``render()`` belongs to that call.
For values shared by many documents, you can also set an environment-wide global context.
When a name exists in both contexts, the local value wins.
This lets a shared default coexist with one document's specific data without copying every shared value into each local
context.

Render and Handle Failures
==========================

``render()`` takes a logical layout name and a context and returns the complete output string.
There is also an overload without a local context for a layout that only uses global values or literal text.
Each call resolves the named layout and evaluates it with that call's values.

A missing layout, invalid layout syntax, or an error during evaluation raises
:cpp:class:`RenderError <erbsland::text::render::RenderError>`.
The demo catches the error around the operation that may fail and prints the layout name from its context.
For a diagnostic to show a user or log, inspect the error's ``context()`` or ``diagnostic()``; the context carries the
category, layout name, source origin, and location when available.

Once this workflow is familiar, the next topics explore shared context, loader choices, and the layout language in more
depth.
The :doc:`template_feature_cheat_sheet` is a quick lookup while writing a layout.
