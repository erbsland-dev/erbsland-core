..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Text Rendering; Context Values
    single: Render Context; Preparing Values
    single: Render Values; Lazy Callbacks

**************************************
Preparing Values for a Rendered Layout
**************************************

A layout describes the shape of a document; a context supplies the names and values that make one rendering specific.
This page shows how to prepare those values, group related data, and defer a calculation until a layout needs it.

Give Names to the Data
======================

A :cpp:class:`Context <erbsland::text::render::Context>` is a collection of named
:cpp:class:`Value <erbsland::text::render::Value>` objects.
You can assemble it before calling ``render()``, inspect what it contains, and replace a name when a later reading is
more accurate.
For example, a terrain report might receive a region and an elevation measured during a survey.

.. erbsland-demo::
    :source: text/RenderLayouts/ContextValues.cpp
    :exec: text/render_layouts --demo ContextValues
    :source-sha256: c9c9e01f637044297184c6eb95ce435fb9a97bc9333427469ba14350874a9bb7

.. code-block:: cpp

    /// Prepare and inspect named values for one rendered document.
    ///
    /// A context holds the names a layout can read. Setting an existing name replaces its value; testing a name before
    /// reading it distinguishes an absent entry from an explicitly stored null value.
    /// @notest{Demo function verified by the documentation executable.}
    void contextValues() {
        auto context = el::render::Context{};
        context.set("region"_el, "sierra"_el).set("elevation"_el, 1840);

        // Replace a reading and inspect the prepared values before rendering.
        context.set("elevation"_el, 1920);
        el::io::printLine("Region: "_el, context.get("region"_el).asText());
        el::io::printLine("Elevation: "_el, context.get("elevation"_el).asInteger(), " m"_el);
        el::io::printLine("Named values: "_el, context.values().count().toSizeT());

        // An unknown name returns null; contains() tells whether a name was supplied at all.
        if (!context.contains("slope"_el)) {
            el::io::printLine("No slope classification was supplied."_el);
        }
        el::io::printLine("Missing value is null: "_el, context.get("slope"_el).isNull() ? "yes"_el : "no"_el);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Region: sierra
    Elevation: 1920 m
    Named values: 2
    No slope classification was supplied.
    Missing value is null: yes

.. erbsland-demo-end::

``set()`` accepts a name and a value and returns the context, so several assignments can be chained.
``get()`` returns the named value, or a null ``Value`` if no entry exists.
Use ``contains()`` when you need to distinguish a missing entry from a name deliberately assigned null.
``values()`` gives read-only access to the underlying :cpp:type:`ValueMap <erbsland::text::render::ValueMap>`; it is
useful when you need to inspect or pass along all prepared entries.

When rendering, the context passed to ``render()`` is local to that call.
The environment can also hold shared values; a local name takes precedence over the same global name.
The :doc:`render environment page <configuring_the_render_environment>` shows how to set those shared defaults.

Choose the Shape of Each Value
==============================

``Value`` keeps the type of the data you provide.
It accepts Core text, Boolean values, signed integers and other integral types that fit in 64 bits, floating-point
numbers, lists, and maps.
A default-constructed ``Value`` is null.
The scalar constructors make simple context assignments natural: ``set("height"_el, 1920)`` creates an integer value
without a separate conversion step.

For data with several fields, use a :cpp:type:`ValueMap <erbsland::text::render::ValueMap>`.
A layout can reach its members with dotted names such as ``site.name``.
A :cpp:type:`ValueList <erbsland::text::render::ValueList>` holds a sequence that a layout can iterate.
The next example uses both shapes in a small terrain classification report.

.. erbsland-demo::
    :source: text/RenderLayouts/ValueShapes.cpp
    :exec: text/render_layouts --demo ValueShapes
    :source-sha256: 105253b24660afcbcf7fcdb7cf380bb4e6230a1913e97d1d252670cd40fe1928

.. code-block:: cpp

    /// Build scalar, list, and map values for a terrain report.
    ///
    /// Values preserve their types. A list supplies repeated entries, while a map groups related named fields that a
    /// layout can reach through dotted names.
    /// @notest{Demo function verified by the documentation executable.}
    void valueShapes() {
        using el::render::Value;
        using el::render::ValueList;
        using el::render::ValueMap;

        // Create one nested record and an ordered list of terrain labels.
        auto site = ValueMap{};
        site.set("name"_el, Value{"Valle Clara"_el}).set("height"_el, Value{1920});
        const auto terrain = ValueList{Value{"valle"_el}, Value{"meseta"_el}, Value{"sierra"_el}};
        const auto context = el::render::Context{}
                                 .set("site"_el, site)
                                 .set("terrain"_el, terrain)
                                 .set("surveyed"_el, Value{true})
                                 .set("ratio"_el, Value{0.75})
                                 .set("note"_el, Value{});

        // The same values can be inspected in C++ before they reach a layout.
        const auto siteValue = context.get("site"_el);
        el::io::printLine("First terrain: "_el, context.get("terrain"_el).get(el::unit::ItemIndex{0U}).asText());
        el::io::printLine("Site height: "_el, siteValue.get("height"_el).asInteger(), " m"_el);

        const auto directory = el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
        (directory->path() / "terrain.txt"_el)
            .content()
            .writeTextOrThrow(
                "{{ site.name }} ({{ site.height }} m)\n"
                "{% for label in terrain %}- {{ label }}\n{% endfor %}"
                "Surveyed: {{ surveyed }}; ratio: {{ ratio }}\n"_el);
        const auto environment = el::render::Environment::create();
        environment->addLayoutLoader(el::render::FileSystemLoader::create(directory->path()));
        el::io::print(environment->render("terrain.txt"_el, context));
    }

.. erbsland-ansi::
    :escape-char: ␛

    First terrain: valle
    Site height: 1920 m
    Valle Clara (1920 m)
    - valle
    - meseta
    - sierra
    Surveyed: true; ratio: 0.75

.. erbsland-demo-end::

In C++, ``isText()``, ``isInteger()``, ``isList()``, and the other type tests let you check a value before using a typed
accessor such as ``asText()`` or ``asInteger()``.
A mismatched ``as...()`` call throws a logic error.
``get(index)`` and ``get(name)`` read children of a list or map and return null for an invalid index, absent member, or
the wrong collection type.
Use ``itemCount()`` when the number of list or map entries matters.

Keep lists and maps as structured data for iteration and member lookup.
Rendering either collection directly as an expression is an error; render a scalar member or iterate over the list
instead.
Null and missing names render as empty text and are false in a layout condition.

Compute a Value When It Is Needed
=================================

Sometimes a value is expensive to obtain, or only some layouts need it.
A :cpp:type:`ValueCallbackFn <erbsland::text::render::ValueCallbackFn>` is a callable that returns a ``Value`` when the
renderer reads the name.
The context can therefore expose a classification without calculating it while the context is assembled.

.. erbsland-demo::
    :source: text/RenderLayouts/LazyValue.cpp
    :exec: text/render_layouts --demo LazyValue
    :source-sha256: 6f492bb88136dbb0b9ac6816d60466851ce6dad76a5f50b99cb3facfa0e464a2

.. code-block:: cpp

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

.. erbsland-ansi::
    :escape-char: ␛

    Calculations before render: 0
    altiplano: meseta
    Calculations after render: 1

.. erbsland-demo-end::

Here, the layout reads ``classification`` once, so its callback runs once.
It never reads ``unused``, so that callback does not run.
If a layout reads a callback value more than once, the callable can run more than once; keep the computation safe to
repeat or cache its result in your application when needed.
Captured references must remain valid until rendering finishes, and callbacks used by concurrent renders must handle
concurrent calls safely.

The :doc:`render workflow <using_the_render_engine>` shows where a local context enters a complete render call.
The :doc:`layout introduction <rendering_text_layouts>` shows how layouts use names, expressions, and statements.
The :doc:`template_feature_cheat_sheet` lists their supported forms, and the
:doc:`rendering reference </reference/text/documents_and_rendering>` has the exact API details.
