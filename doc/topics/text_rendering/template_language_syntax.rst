..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Text Rendering; Template Language
    single: Layouts; Expressions and Filters
    single: Layouts; Conditions
    single: Layouts; Iteration

*****************************
Writing the Template Language
*****************************

A layout can do more than insert names into fixed text.
It can calculate a small value, choose a passage, or repeat a passage for each item in a collection.
This page shows how to write those parts of a layout and what values they expect from the render context.

Read the Markers in a Layout
============================

The examples on this page use the default syntax.
Text outside a marker is copied to the result as written.
An expression between ``{{`` and ``}}`` inserts a value, a statement between ``{%`` and ``%}`` controls rendering, and a
comment between ``{#`` and ``#}`` disappears from the result.
For example, ``{{ quest.title }}`` inserts a title, while ``{% if ready %}`` begins a conditional passage.

These markers belong to the :cpp:class:`EnvironmentOptions <erbsland::text::render::EnvironmentOptions>` used when
creating the :cpp:class:`Environment <erbsland::text::render::Environment>`.
If your document format already uses braces, you can choose other expression, statement, and comment delimiters.
The :doc:`environment page <configuring_the_render_environment>` shows that setup.
Every layout in an environment then uses its configured markers; the examples below assume the defaults.

Here an environment uses ``[[ ... ]]`` for expressions and ``<% ... %>`` for statements.
The layout uses those markers throughout, including the ``if`` statement around its inserted value.

.. erbsland-demo::
    :source: text/RenderLayouts/EnvironmentExamples.cpp
    :function-blocks: customSyntax
    :function-blocks-sha256: f0cda2c83cef245427c7c5e07e4f3b819383c0a23d4a7990ec503d5b4e5f2d6f
    :exec: text/render_layouts --demo CustomSyntax
    :source-sha256: 3ffc62e74c29eec2e183be0aa4d4b10a1d11a55e493e328fef7144f6f6fb82da

.. code-block:: cpp

    void customSyntax() {
        const auto directory = el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
        (directory->path() / "current.txt"_el)
            .content()
            .writeTextOrThrow("(# note #)<% if active %>[[ current ]]<% endif %>\n"_el);

        auto options = el::render::EnvironmentOptions{};
        options.setExpressionDelimiters(el::render::Delimiters{"[["_el, "]]"_el})
            .setStatementDelimiters(el::render::Delimiters{"<%"_el, "%>"_el})
            .setCommentDelimiters(el::render::Delimiters{"(#"_el, "#)"_el});
        const auto environment = el::render::Environment::create(options);
        environment->addLayoutLoader(el::render::FileSystemLoader::create(directory->path()));
        el::io::print(environment->render(
            "current.txt"_el, el::render::Context{}.set("active"_el, true).set("current"_el, "Ligure"_el)));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Ligure

.. erbsland-demo-end::

The language will look familiar if you know Jinja, but it is a deliberately smaller, Jinja-like language.
Its familiar ``{{ ... }}``, ``if``, ``for``, filter, include, and inheritance forms make ordinary layouts easy to read.
Do not assume that an arbitrary Jinja template will work unchanged: this renderer has its own supported expressions,
statements, and value rules.
The :doc:`rendering reference </reference/text/documents_and_rendering>` gives the exact language boundary.

Insert Values and Apply Filters
===============================

An expression can read a name from the context, then follow map members with dots.
In the next example, ``quest.title`` reads ``title`` from the ``quest`` map.
The ``completed + 1`` expression calculates the next step number, while ``steps | length`` gets the size of a list.
A pipe passes a value to a filter; ``trim`` removes surrounding whitespace and ``join(' → ')`` turns list entries into
one line.

.. erbsland-demo::
    :source: text/RenderLayouts/LanguageExpressions.cpp
    :exec: text/render_layouts --demo LanguageExpressions
    :source-sha256: c12e8ae1aeb8d3b8f9d0f776d8f1ebac00ca66828795a65d960dda9d1d28d994

.. code-block:: cpp

    /// Combine named values, expressions, and filters in a quest note.
    ///
    /// Dotted lookup reads a member from a map. An expression can calculate a value before insertion, and a filter can
    /// transform the result. Missing names resolve to null, which the default filter can replace with useful text.
    /// @notest{Demo function verified by the documentation executable.}
    void languageExpressions() {
        using el::render::Value;
        using el::render::ValueList;
        using el::render::ValueMap;

        const auto directory = el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
        (directory->path() / "quest.txt"_el)
            .content()
            .writeTextOrThrow(
                "依頼: {{ quest.title | trim }}\n"
                "次の段階: {{ completed + 1 }} / {{ steps | length }}\n"
                "手順: {{ steps | join(' → ') }}\n"
                "担当: {{ owner | default('未定') }}\n"_el);

        // The layout reads a nested map, a list, and an integer from one local context.
        auto quest = ValueMap{};
        quest.set("title"_el, Value{"  星空を調べる  "_el});
        const auto steps = ValueList{Value{"望遠鏡を準備"_el}, Value{"月を観察"_el}, Value{"星図を記録"_el}};
        const auto context = el::render::Context{}.set("quest"_el, quest).set("steps"_el, steps).set("completed"_el, 1);

        const auto environment = el::render::Environment::create();
        environment->addLayoutLoader(el::render::FileSystemLoader::create(directory->path()));
        el::io::print(environment->render("quest.txt"_el, context));
    }

.. erbsland-ansi::
    :escape-char: ␛

    依頼: 星空を調べる
    次の段階: 2 / 3
    手順: 望遠鏡を準備 → 月を観察 → 星図を記録
    担当: 未定

.. erbsland-demo-end::

The context supplies structured values, and the layout decides how to present them.
``owner`` is absent here, so lookup produces null; ``default('未定')`` supplies text for that case.
Without the filter, null would insert an empty string.
A list or map cannot be inserted directly: select a scalar member, join suitable list entries, or iterate instead.

You can also write string, number, Boolean, null, list, and map literals in an expression.
Parentheses clarify a calculation, and comparisons such as ``>=`` or ``in`` let a condition examine a value.
The language supports ``and``, ``or``, and ``not`` for combining conditions; ``and`` and ``or`` stop as soon as the
result is decided.
Filters run from left to right, so ``title | trim | upper`` trims before it changes case.
The :doc:`template_feature_cheat_sheet` lists every supported operator, test, and built-in filter with a compact
example.
The :doc:`rendering reference </reference/text/documents_and_rendering>` supplies their exact value rules.

Choose a Passage with a Condition
=================================

When only one passage belongs in the result, keep the choice in the layout.
An ``if`` tests an expression; optional ``elif`` branches test further cases, and ``else`` provides a final case.
The first matching branch is rendered, and ``endif`` closes the statement.

.. erbsland-demo::
    :source: text/RenderLayouts/LanguageConditions.cpp
    :exec: text/render_layouts --demo LanguageConditions
    :source-sha256: 2d33fbef93496d09d65607060fe7a12f671447fb89fc0e1ba78431c0ba7a55f3

.. code-block:: cpp

    /// Select one status line from an if, elif, and else chain.
    ///
    /// Conditions inspect context values without requiring the caller to choose or assemble layout fragments.
    /// @notest{Demo function verified by the documentation executable.}
    void languageConditions() {
        const auto directory = el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
        (directory->path() / "status.txt"_el)
            .content()
            .writeTextOrThrow(
                "{% if completed >= required %}達成"
                "{% elif completed > 0 %}進行中"
                "{% else %}未着手{% endif %}\n"_el);

        const auto environment = el::render::Environment::create();
        environment->addLayoutLoader(el::render::FileSystemLoader::create(directory->path()));

        // Reuse the same layout for three states of the quest.
        for (const auto completed : {0, 1, 3}) {
            const auto context = el::render::Context{}.set("completed"_el, completed).set("required"_el, 3);
            el::io::print(environment->render("status.txt"_el, context));
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    未着手
    進行中
    達成

.. erbsland-demo-end::

The same layout renders three states because only the context values change.
Conditions use the renderer's truth rules: null, false, numeric zero, empty text, and empty collections are false.
This makes ``{% if steps %}`` useful for a nonempty list, while a comparison states the intended threshold more clearly
for a numeric progress value.
Missing names resolve to null, so a missing optional value can be checked with ``is none``.

If a calculation deserves a name inside a layout, ``{% set remaining = required - completed %}`` creates a render-local
value.
It does not change the caller's :cpp:class:`Context <erbsland::text::render::Context>`.
An assignment inside a branch runs only when that branch runs.
For exact assignment and scope rules, see the :doc:`rendering reference </reference/text/documents_and_rendering>`.

Repeat a Passage for a Collection
=================================

A list loop gives one name to each item.
Here ``step`` is a text value, and ``loop.index`` numbers the lines starting at one.
The optional ``else`` passage runs when the list is empty.
The second layout shows the two-name form for an ordered map: one name receives the key, the other its value.

.. erbsland-demo::
    :source: text/RenderLayouts/LanguageIterations.cpp
    :exec: text/render_layouts --demo LanguageIterations
    :source-sha256: 30d646a5e8875065a4412e5551dfd53af31a62b3591b11f5471028a9a29389bf

.. code-block:: cpp

    /// Render a list and an ordered map with for loops.
    ///
    /// A list loop exposes one item and loop position data. A map loop exposes a key and value. The optional else branch
    /// gives a useful result when a collection is empty.
    /// @notest{Demo function verified by the documentation executable.}
    void languageIterations() {
        using el::render::Value;
        using el::render::ValueList;
        using el::render::ValueMap;

        const auto directory = el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
        (directory->path() / "steps.txt"_el)
            .content()
            .writeTextOrThrow(
                "{% for step in steps %}{{ loop.index }}. {{ step }}\n"
                "{% else %}手順なし\n{% endfor %}"_el);
        (directory->path() / "rewards.txt"_el)
            .content()
            .writeTextOrThrow("{% for name, count in rewards %}{{ name }}: {{ count }}\n{% endfor %}"_el);

        const auto environment = el::render::Environment::create();
        environment->addLayoutLoader(el::render::FileSystemLoader::create(directory->path()));

        // List entries appear in their original order; an empty list selects the else branch.
        const auto steps = ValueList{Value{"望遠鏡を準備"_el}, Value{"月を観察"_el}};
        el::io::print(environment->render("steps.txt"_el, el::render::Context{}.set("steps"_el, steps)));
        el::io::print(environment->render("steps.txt"_el, el::render::Context{}.set("steps"_el, ValueList{})));

        // A map supplies two loop targets: its key and its value.
        auto rewards = ValueMap{};
        rewards.set("星図"_el, Value{1}).set("観測記録"_el, Value{2});
        el::io::print(environment->render("rewards.txt"_el, el::render::Context{}.set("rewards"_el, rewards)));
    }

.. erbsland-ansi::
    :escape-char: ␛

    1. 望遠鏡を準備
    2. 月を観察
    手順なし
    星図: 1
    観測記録: 2

.. erbsland-demo-end::

A map is visited in :cpp:type:`StringMap <erbsland::text::StringMap>` key order.
The ``loop`` value also offers ``index0``, ``first``, ``last``, ``length``, and reverse indexes when the presentation
needs them.
Each pass has its own scope, so a loop item or an assignment made inside one pass does not replace a name in the
caller's context or leak into the next pass.
If a value might be null, prepare a list or map in the context before iterating: ``for`` expects the collection type
that matches its target names.

Keep Layouts Readable
=====================

Statements and expressions affect only the text they surround; ordinary spaces and line breaks in the source are
preserved.
When a control line leaves an unwanted blank line, a ``-`` whitespace marker beside a delimiter removes adjacent ASCII
whitespace.
Use it where the result needs it, since it also removes source line breaks that may make a rendered document easier to
read.

For longer documents, :doc:`loading_render_layouts` shows how to split reusable passages into named layouts.
Blocks and inheritance let related layouts share a document structure; :doc:`blocks_and_inheritance` introduces that
workflow.
When you need to recall a spelling while editing a layout, use the :doc:`template_feature_cheat_sheet`.
