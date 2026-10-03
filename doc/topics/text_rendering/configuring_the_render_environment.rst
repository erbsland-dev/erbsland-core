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

Environment Reuse, Concurrent Rendering, and Auto-Reload
========================================================

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

Global Render Context and Shared Values
=======================================

Suppose several observations share the same current and coastline.
Place those values in a global :cpp:class:`Context <erbsland::text::render::Context>` and render without passing a local
context.
``setGlobalContext()`` replaces the whole global context with a new snapshot, so later renders can see updated shared
values.

.. erbsland-demo::
    :source: text/RenderLayouts/EnvironmentExamples.cpp
    :show: f:sharedValues
    :exec: text/render_layouts --demo SharedValues
    :sha256: 32728e0c6deebfdd67e1d7dd6ae81778aaea9684ce4647b5c40b0995938b81ef

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

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mCorrente: Ligure; costa: Italia
        Corrente: Adriatica; costa: Croazia␛[0m

.. erbsland-demo-end::

A global context works well for values that genuinely belong to every document in this environment, such as a common
site title or a set of stable labels.
Replacing the snapshot is also safe while other threads render: a render call uses one consistent snapshot.
If a value differs for each document, pass it locally instead.

Local Context Overrides of Global Values
========================================

A local context belongs to one ``render()`` call.
A name found there wins over the same name in the global snapshot; names absent locally still come from the global
context.
Here the local station replaces the shared station, while ``season`` remains shared.

.. erbsland-demo::
    :source: text/RenderLayouts/EnvironmentExamples.cpp
    :show: f:localValues
    :exec: text/render_layouts --demo LocalValues
    :sha256: fccdaaec6749421fb2d59ae6e61494b2776e165582ee25c0c1b43d6bd2b0fdb9

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

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mPunto sud: Ligure (estate)␛[0m

.. erbsland-demo-end::

This separation lets you keep broad defaults in one place without making one document's values affect another.
If a name is absent from both contexts, it resolves to null and inserts empty text.
The :doc:`rendering reference </reference/text/documents_and_rendering>` lists the value types and lookup rules.

Registering Custom Render Filters
=================================

A layout may need a small transformation that belongs to your application rather than to the layout language.
Register a named :cpp:type:`FilterFn <erbsland::text::render::FilterFn>` with ``addFilter()``.
The callback receives a :cpp:type:`ValueList <erbsland::text::render::ValueList>`: element zero is the piped value, and
up to two further elements are positional arguments from the layout.

.. erbsland-demo::
    :source: text/RenderLayouts/EnvironmentExamples.cpp
    :show: f:customFilter
    :exec: text/render_layouts --demo CustomFilter
    :sha256: 5e851746cb12e7a857fc89fd27f3a2ba6b5f1070052c82716787f6e8ab3f8157

.. code-block:: cpp

    void customFilter() {
        const auto directory = el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
        (directory->path() / "current.txt"_el).content().writeTextOrThrow("{{ current | label('Corrente: ') }}\n"_el);

        const auto environment = el::render::Environment::create();
        environment->addLayoutLoader(el::render::FileSystemLoader::create(directory->path()));
        environment->addFilter("label"_el, [](const el::render::ValueList &values) -> el::render::Value {
            if (values.count() != ItemCount{2U} || !values.getRef(ItemIndex{0U}).isText() ||
                !values.getRef(ItemIndex{1U}).isText()) {
                throw el::render::RenderError{el::render::RenderErrorContext{
                    el::render::RenderErrorCategory::Runtime,
                    "Invalid label filter arguments"_el,
                    "The label filter needs text and one text prefix."_el}};
            }
            return el::StringList{values.getRef(ItemIndex{1U}).asText(), values.getRef(ItemIndex{0U}).asText()}.join();
        });
        el::io::print(environment->render("current.txt"_el, el::render::Context{}.set("current"_el, "Ligure"_el)));
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mCorrente: Ligure␛[0m

.. erbsland-demo-end::

Validate the count and types inside the callback, as the demo does, before using a value.

These arguments can come from user-supplied layouts and context data; prefer ``RenderError`` with category ``Runtime``
to reject invalid values.
Supply a title and description and leave the source context unset, as the demo does.

Filter names are ASCII identifiers, and each application filter name is registered once.
The output modifiers ``escape``, ``e``, and ``safe`` have reserved names.
A list of the built-in filters and output modifiers appears in the :doc:`template_feature_cheat_sheet`.
A registered filter may run on several threads at the same time; any mutable state captured by its callback needs its
own synchronization.

Configuring Layout Delimiters
=============================

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
    :show: f:customSyntax
    :exec: text/render_layouts --demo CustomSyntax
    :sha256: 24c250517bd27cb6592d96d19004bc4b083970016c59ea5383da3ef406ae0910

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

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mLigure␛[0m

.. erbsland-demo-end::

Each setting takes a :cpp:class:`Delimiters <erbsland::text::render::Delimiters>` pair with begin and end markers.
A delimiter can also carry an optional line marker for line statements.
Choose distinct markers that do not collide with ordinary source text or one another.

Configuring Output Escaping
===========================

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
    :show: f:escapingOptions
    :exec: text/render_layouts --demo EscapingOptions
    :sha256: 42422f4a57f0ce8958443981511382671e1717dfdd5c6551efe81e7485b7c326

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

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mMare &amp; vento
        Mare & vento
        Mare & vento␛[0m

.. erbsland-demo-end::

``escapeFormatForLayout()`` lets you inspect the choice for a logical name.
``removeEscapeFormatForSuffix()`` removes one assignment, while ``clearEscapeFormats()`` removes all assignments,
including the defaults.
A suffix begins with ``.`` and uses ASCII letters, digits, ``.``, ``_``, or ``-``.
Keep ordinary automatic escaping in place for documents that may contain values from outside your application.

Rendering Work and Output Limits
================================

A layout can generate much more text than its source contains, especially with loops and includes.
``setRenderLimits()`` replaces the :cpp:class:`RenderLimits <erbsland::text::render::RenderLimits>` used by the
environment; ``renderLimits()`` reads the current limits.
The following example sets a deliberately tiny output limit to show the resulting error boundary.

.. erbsland-demo::
    :source: text/RenderLayouts/EnvironmentExamples.cpp
    :show: f:renderingLimit
    :exec: text/render_layouts --demo RenderingLimit
    :sha256: c2a26e9d544ddfb88cf2cf1f0648c6890091b57e9b019c869a4973d1f827877a

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

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mThe output limit was reached.␛[0m

.. erbsland-demo-end::

The limit object also controls executed instructions, runtime nesting, callback resolution depth, lexical scopes, call
frames, value stack depth, and static dependency depth.
The defaults fit ordinary documents; adjust a limit when your expected layout complexity calls for it.
A limit failure raises :cpp:class:`RenderError <erbsland::text::render::RenderError>`, just like other render failures.

Once the environment is configured, :doc:`loading_render_layouts` shows how its loaders find files, compiled resources,
and application-owned sources.
