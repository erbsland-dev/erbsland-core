..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Text Rendering; Environment
    single: Render Context; Global Values
    single: Text Rendering; Custom Filters
    single: EnvironmentOptions; Layout Syntax

**********************************
Configuring the Render Environment
**********************************

An :cpp:class:`Environment <erbsland::text::render::Environment>` holds the choices that apply to a family of layouts:
where to find them, which filters they may use, and how their syntax and output should behave.
Create and configure it once, then render many documents with separate local contexts.
This page shows how shared values and local values meet, and how to choose the environment options for your layouts.

Keep the Environment for Repeated Renders
=========================================

The environment is the entry point for :cpp:func:`render() <erbsland::text::render::Environment::render>`.
It takes a *logical layout name*, finds the source through its registered loaders, and returns a complete
:cpp:type:`String <erbsland::text::String>`.
Register loaders and filters during setup, before the first render.
After setup, the same environment can be used by concurrent render calls, provided application filters and custom
loaders also support concurrent use.
The :doc:`using_the_render_engine` page shows that basic sequence with one file-backed layout.

When layout files change during development, ``enableAutoReload()`` asks loaders to check them again on each render.
This is useful for a live editing cycle, although it costs extra lookup work.
Enable it during setup; once enabled, it stays on for that environment.
For a deployed set of stable layouts, the ordinary cached behavior avoids those checks.

Share Values Across Documents
=============================

Suppose several observations share the same current and coastline.
Place those values in a global :cpp:class:`Context <erbsland::text::render::Context>` and render without passing a local
context.
``setGlobalContext()`` replaces the whole global context with a new snapshot, so later renders can see updated shared
values.

.. erbsland-demo::
    :source: text/RenderLayouts/EnvironmentExamples.cpp
    :function-blocks: sharedValues
    :function-blocks-sha256: f8225b19856d9d554251b41a6d57df0e0ee890f08d5f24fb5d068c62310656a1
    :exec: text/render_layouts --demo SharedValues
    :source-sha256: 3ffc62e74c29eec2e183be0aa4d4b10a1d11a55e493e328fef7144f6f6fb82da

.. code-block:: cpp

    void sharedValues() {
        const auto directory = el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
        (directory->path() / "current.txt"_el)
            .content()
            .writeTextOrThrow("Corrente: {{ current }}; costa: {{ coast }}\n"_el);

        const auto environment = el::render::Environment::create();
        environment->addLayoutLoader(el::render::FileSystemLoader::create(directory->path()));
        environment->setGlobalContext(el::render::Context{}.set("current"_el, "Ligure"_el).set("coast"_el, "Italia"_el));
        el::io::print(environment->render("current.txt"_el));
        environment->setGlobalContext(
            el::render::Context{}.set("current"_el, "Adriatica"_el).set("coast"_el, "Croazia"_el));
        el::io::print(environment->render("current.txt"_el));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Corrente: Ligure; costa: Italia
    Corrente: Adriatica; costa: Croazia

.. erbsland-demo-end::

A global context works well for values that genuinely belong to every document in this environment, such as a common
site title or a set of stable labels.
Replacing the snapshot is also safe while other threads render: a render call uses one consistent snapshot.
If a value differs for each document, pass it locally instead.

Let One Document Override Shared Values
=======================================

A local context belongs to one ``render()`` call.
A name found there wins over the same name in the global snapshot; names absent locally still come from the global
context.
Here the local station replaces the shared station, while ``season`` remains shared.

.. erbsland-demo::
    :source: text/RenderLayouts/EnvironmentExamples.cpp
    :function-blocks: localValues
    :function-blocks-sha256: 33a05f2b70979ea18ca44a3a977d79a80ad8cbbc86375a48e63fd546b872676b
    :exec: text/render_layouts --demo LocalValues
    :source-sha256: 3ffc62e74c29eec2e183be0aa4d4b10a1d11a55e493e328fef7144f6f6fb82da

.. code-block:: cpp

    void localValues() {
        const auto directory = el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
        (directory->path() / "observation.txt"_el)
            .content()
            .writeTextOrThrow("{{ station }}: {{ current }} ({{ season }})\n"_el);

        const auto environment = el::render::Environment::create();
        environment->addLayoutLoader(el::render::FileSystemLoader::create(directory->path()));
        environment->setGlobalContext(
            el::render::Context{}.set("station"_el, "Punto nord"_el).set("season"_el, "estate"_el));
        const auto local = el::render::Context{}.set("station"_el, "Punto sud"_el).set("current"_el, "Ligure"_el);
        el::io::print(environment->render("observation.txt"_el, local));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Punto sud: Ligure (estate)

.. erbsland-demo-end::

This separation lets you keep broad defaults in one place without making one document's values affect another.
If a name is absent from both contexts, it resolves to null and inserts empty text.
The :doc:`rendering reference </reference/text/documents_and_rendering>` lists the value types and lookup rules.

Add a Filter for Application Vocabulary
=======================================

A layout may need a small transformation that belongs to your application rather than to the layout language.
Register a named :cpp:type:`FilterFn <erbsland::text::render::FilterFn>` with ``addFilter()``.
The callback receives a :cpp:type:`ValueList <erbsland::text::render::ValueList>`: element zero is the piped value, and
up to two further elements are positional arguments from the layout.

.. erbsland-demo::
    :source: text/RenderLayouts/EnvironmentExamples.cpp
    :function-blocks: customFilter
    :function-blocks-sha256: 1650f46ee1ecc932d3a802cd7eba48227f494666f067301b28bb314750eda76e
    :exec: text/render_layouts --demo CustomFilter
    :source-sha256: 3ffc62e74c29eec2e183be0aa4d4b10a1d11a55e493e328fef7144f6f6fb82da

.. code-block:: cpp

    void customFilter() {
        const auto directory = el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
        (directory->path() / "current.txt"_el).content().writeTextOrThrow("{{ current | label('Corrente: ') }}\n"_el);

        const auto environment = el::render::Environment::create();
        environment->addLayoutLoader(el::render::FileSystemLoader::create(directory->path()));
        environment->addFilter("label"_el, [](const el::render::ValueList &values) -> el::render::Value {
            if (values.count() != el::unit::ItemCount{2U} || !values.getRef(el::unit::ItemIndex{0U}).isText() ||
                !values.getRef(el::unit::ItemIndex{1U}).isText()) {
                throw el::err::ParameterError{"The label filter needs text and one text prefix."_el, "values"_el};
            }
            return el::StringList{
                values.getRef(el::unit::ItemIndex{1U}).asText(), values.getRef(el::unit::ItemIndex{0U}).asText()}
                .join();
        });
        el::io::print(environment->render("current.txt"_el, el::render::Context{}.set("current"_el, "Ligure"_el)));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Corrente: Ligure

.. erbsland-demo-end::

Validate the count and types inside the callback, as the demo does, before using a value.
Filter names are ASCII identifiers, and each application filter name is registered once.
The output modifiers ``escape``, ``e``, and ``safe`` have reserved names.
A list of the built-in filters and output modifiers appears in the :doc:`template_feature_cheat_sheet`.
A registered filter may run on several threads at the same time; any mutable state captured by its callback needs its
own synchronization.

Choose the Layout Syntax
========================

:cpp:class:`EnvironmentOptions <erbsland::text::render::EnvironmentOptions>` is passed to
``Environment::create()``.
Without custom options, expressions use ``{{ ... }}``, statements use ``{% ... %}``, and comments use ``{# ... #}``.
Change these choices before creating the environment, then write every layout for that environment with the chosen
syntax.

Expression Delimiters
---------------------

``setExpressionDelimiters()`` changes how inserted values are marked.
This matters when the surrounding document format already uses braces.
The example below uses ``[[ current ]]``; ``expressionDelimiters()`` returns the configured pair.

Statement Delimiters
--------------------

``setStatementDelimiters()`` changes control statements such as ``if``, ``for``, and ``include``.
The example uses ``<% if active %>``.
``statementDelimiters()`` returns that pair.

Comment Delimiters
------------------

``setCommentDelimiters()`` chooses text that the renderer discards without evaluating it.
The example's ``(# note #)`` is absent from the result.
``commentDelimiters()`` returns this pair.

The three settings work together in one layout:

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

Each setting takes a :cpp:class:`Delimiters <erbsland::text::render::Delimiters>` pair with begin and end markers.
A delimiter can also carry an optional line marker for line statements.
Choose distinct markers that do not collide with ordinary source text or one another.

Choose Escaping for the Output
==============================

Automatic Escaping
------------------

By default, inserted values are escaped according to the *layout name's suffix*.
For example, ``.html`` selects HTML escaping; ``.xml``, ``.json``, ``.md``, and ``.elcl`` have their own default
mappings.
A suffix without a mapping produces unescaped output.
``automaticEscapingEnabled()`` reports whether this feature is on, and ``setAutomaticEscapingEnabled(false)`` switches
it off for the entire environment.
The third render in the example below turns off automatic escaping for a second environment and shows the original
ampersand.
This broad switch is appropriate only when every layout in that environment is intended to emit trusted, unescaped text.

Escape Formats for Suffixes
---------------------------

A more focused choice is ``setEscapeFormatForSuffix()``.
The renderer picks the longest matching suffix, so ``.raw.html`` can take precedence over ``.html``.
The example keeps ordinary HTML escaping and gives one specially named layout an
:cpp:class:`EscapeFormat <erbsland::text::EscapeFormat>` of ``None``.

.. erbsland-demo::
    :source: text/RenderLayouts/EnvironmentExamples.cpp
    :function-blocks: escapingOptions
    :function-blocks-sha256: f7fa8a819423c4122f0ae779ebd9f2e4b9bad5b4cb5cd3cbb8ad39e40cfddbc0
    :exec: text/render_layouts --demo EscapingOptions
    :source-sha256: 3ffc62e74c29eec2e183be0aa4d4b10a1d11a55e493e328fef7144f6f6fb82da

.. code-block:: cpp

    void escapingOptions() {
        const auto directory = el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
        (directory->path() / "note.html"_el).content().writeTextOrThrow("{{ title }}\n"_el);
        (directory->path() / "note.raw.html"_el).content().writeTextOrThrow("{{ title }}\n"_el);

        auto options = el::render::EnvironmentOptions{};
        options.setEscapeFormatForSuffix(".raw.html"_el, el::EscapeFormat::None);
        const auto environment = el::render::Environment::create(options);
        environment->addLayoutLoader(el::render::FileSystemLoader::create(directory->path()));
        const auto context = el::render::Context{}.set("title"_el, "Mare & vento"_el);
        el::io::print(environment->render("note.html"_el, context));
        el::io::print(environment->render("note.raw.html"_el, context));

        auto plainOptions = el::render::EnvironmentOptions{};
        plainOptions.setAutomaticEscapingEnabled(false);
        const auto plainEnvironment = el::render::Environment::create(plainOptions);
        plainEnvironment->addLayoutLoader(el::render::FileSystemLoader::create(directory->path()));
        el::io::print(plainEnvironment->render("note.html"_el, context));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Mare &amp; vento
    Mare & vento
    Mare & vento

.. erbsland-demo-end::

``escapeFormatForLayout()`` lets you inspect the choice for a logical name.
``removeEscapeFormatForSuffix()`` removes one assignment, while ``clearEscapeFormats()`` removes all assignments,
including the defaults.
A suffix begins with ``.`` and uses ASCII letters, digits, ``.``, ``_``, or ``-``.
Keep ordinary automatic escaping in place for documents that may contain values from outside your application.

Bound Rendering Work
====================

A layout can generate much more text than its source contains, especially with loops and includes.
``setRenderLimits()`` replaces the :cpp:class:`RenderLimits <erbsland::text::render::RenderLimits>` used by the
environment; ``renderLimits()`` reads the current limits.
The following example sets a deliberately tiny output limit to show the resulting error boundary.

.. erbsland-demo::
    :source: text/RenderLayouts/EnvironmentExamples.cpp
    :function-blocks: renderingLimit
    :function-blocks-sha256: 4d57dbbd60cb7095d696753fddf02072f8491016463e45175c5c54521dda78c2
    :exec: text/render_layouts --demo RenderingLimit
    :source-sha256: 3ffc62e74c29eec2e183be0aa4d4b10a1d11a55e493e328fef7144f6f6fb82da

.. code-block:: cpp

    void renderingLimit() {
        const auto directory = el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
        (directory->path() / "current.txt"_el).content().writeTextOrThrow("Corrente Ligure"_el);

        auto limits = el::render::RenderLimits{};
        limits.setGeneratedOutputBytes(8U);
        auto options = el::render::EnvironmentOptions{};
        options.setRenderLimits(limits);
        const auto environment = el::render::Environment::create(options);
        environment->addLayoutLoader(el::render::FileSystemLoader::create(directory->path()));
        try {
            el::io::print(environment->render("current.txt"_el));
        } catch (const el::render::RenderError &) {
            el::io::printLine("The output limit was reached."_el);
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    The output limit was reached.

.. erbsland-demo-end::

The limit object also controls executed instructions, runtime nesting, callback resolution depth, lexical scopes, call
frames, value stack depth, and static dependency depth.
The defaults fit ordinary documents; adjust a limit when your expected layout complexity calls for it.
A limit failure raises :cpp:class:`RenderError <erbsland::text::render::RenderError>`, just like other render failures.

Once the environment is configured, :doc:`loading_render_layouts` shows how its loaders find files, compiled resources,
and application-owned sources.
