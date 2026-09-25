..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Text Rendering; Introduction
    single: Layouts; Rendering
    single: Templates; Text Rendering

***************************
Rendering Text with Layouts
***************************

Some documents have a stable shape but change their content each time you create them.
A practice note, for example, might always have a title and an instruction, while the rhythm and repetition count come
from the current exercise.
A layout lets you keep that shape in a text file and supply the changing values when you render it.

This page introduces the render engine and its language through one small example.
The next page, :doc:`using_the_render_engine`, follows the setup and rendering workflow in more detail.

From a Layout to a Document
===========================

A layout is ordinary UTF-8 text with a few embedded instructions.
The renderer copies the ordinary text into the result and evaluates the instructions against a
:cpp:class:`Context <erbsland::text::render::Context>`.
You can therefore change the values without editing the layout, or improve the wording without recompiling the
application that supplies the values.

In the following demo, ``practice.txt`` is a layout named by its path below a temporary directory.
The application writes it there to keep the example self-contained; in a real application, the file would normally live
alongside your other layouts.
The same layout produces two different plain text practice notes.

.. erbsland-demo::
    :source: text/RenderLayouts/Introduction.cpp
    :exec: text/render_layouts --demo Introduction
    :source-sha256: 231eb9443b9cb6ed7ef5304270437dca492c79f219b312958256f9e046c55d6e

.. code-block:: cpp

    /// Render a short practice note from a layout and a context.
    ///
    /// A layout holds the stable wording and template syntax. The context supplies values that change for each
    /// rendering. A file-system loader gives the environment a named source to render.
    void introduction() {
        const auto temporary = el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
        const auto layout = temporary->path() / "practice.txt"_el;
        layout.content().writeTextOrThrow(
            "{# A note for the practice leader. #}"
            "Esercizio: {{ pattern }}\n"
            "{% if repeat %}Ripeti {{ count }} volte.{% else %}Una volta.{% endif %}\n"_el);

        // Register the directory before rendering any named layout.
        const auto environment = el::render::Environment::create();
        environment->addLayoutLoader(el::render::FileSystemLoader::create(temporary->path()));

        // Render the same layout with two different local contexts.
        auto context = el::render::Context{};
        context.set("pattern"_el, "ta-ta-TUM"_el).set("repeat"_el, true).set("count"_el, 4);
        el::io::print(environment->render("practice.txt"_el, context));
        context.set("pattern"_el, "TUM-ta"_el).set("repeat"_el, false);
        el::io::print(environment->render("practice.txt"_el, context));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Esercizio: ta-ta-TUM
    Ripeti 4 volte.
    Esercizio: TUM-ta
    Una volta.

.. erbsland-demo-end::

The ``{{ pattern }}`` expression inserts the rhythm from the context.
The ``{% if repeat %}`` statement selects an instruction based on a Boolean value.
The second render changes ``pattern`` and ``repeat`` but leaves ``practice.txt`` untouched.
The text outside those markers stays exactly where it appears in the layout, including its line breaks.

Reading the Layout Language
===========================

The default syntax uses three kinds of markers.
An expression such as ``{{ count }}`` inserts a value, a statement such as ``{% if repeat %}`` controls what is
rendered, and a comment between ``{#`` and ``#}`` is omitted from the result.
Whitespace around these markers remains significant unless you ask the renderer to trim it.

Expressions can read nested values, combine scalars, and pass a value through a filter.
Statements cover choices, iteration, and local assignments.
Larger layouts can :doc:`include named layouts <template_feature_cheat_sheet>` or
:doc:`inherit a base layout with replaceable blocks <blocks_and_inheritance>`.
The language follows familiar Jinja conventions, but it is a defined subset rather than a promise that every Jinja
template will run unchanged.
The supported forms and their exact behavior are listed in the
:doc:`rendering reference </reference/text/documents_and_rendering>`.
For a compact list of the supported forms, keep the :doc:`template_feature_cheat_sheet` beside your layout.

The Main Pieces
===============

The :cpp:class:`Environment <erbsland::text::render::Environment>` is the configured renderer.
It owns the layout loaders and filters and provides ``render()``.
An application usually configures an environment once, then reuses it for many documents.

A layout, sometimes called a *template*, is the named UTF-8 source that the environment renders.
A :cpp:class:`Loader <erbsland::text::render::Loader>` finds that source by its logical name.
The :cpp:class:`FileSystemLoader <erbsland::text::render::FileSystemLoader>` in the demo searches a directory; another
loader can obtain layouts from compiled resources.

A context is the named set of values for one render call.
Its values can be text, numbers, booleans, lists, maps, or null, so a layout can express more than simple replacement.
If a name is missing, it resolves to null; null inserts empty text and is false in a condition.
Supplying each document's values in a local context makes it clear which data belongs to that document.

Where to Go Next
================

:doc:`using_the_render_engine` shows how to configure an environment, register a loader, prepare a context, and
handle a rendering failure.
For short strings whose structure fits in one format pattern,
:doc:`/topics/text_formatting/using_string_format` may be a simpler fit.
For text that only needs source and filter substitution, :doc:`/topics/text_placeholders/placeholders` describes the
placeholder replacer.
