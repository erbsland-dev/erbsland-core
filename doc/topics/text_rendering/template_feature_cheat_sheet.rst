..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Text Rendering; Template Feature Cheat Sheet
    single: Layouts; Syntax Cheat Sheet
    single: Layouts; Built-In Filters

****************************
Template Feature Cheat Sheet
****************************

Keep this page beside a layout when you need to recall a marker, expression, filter, or statement.
The examples use the default delimiters and a bird nesting record with names such as ``nest`` and ``eggs``.
The :doc:`template language topic <template_language_syntax>` explains how these pieces work together; the
:doc:`rendering reference </reference/text/documents_and_rendering>` gives the exact value and error rules.

Markers and Whitespace
======================

Ordinary text is copied to the result.
The three marker pairs have distinct jobs:

.. list-table::
    :header-rows: 1
    :widths: 25 45 30

    * - Purpose
      - Example
      - Result
    * - Insert a scalar value
      - ``{{ nest.species }}``
      - The species name
    * - Control rendering
      - ``{% if eggs %}Occupied{% endif %}``
      - ``Occupied`` when ``eggs`` is true
    * - Leave a source comment
      - ``{# checked in spring #}``
      - Nothing

Add ``-`` just inside a marker to strip adjacent ASCII whitespace, as in ``{{- nest.species -}}`` or ``{%- endif -%}``.
Without a whitespace marker, source spaces and line breaks remain in the result.
An environment can choose different delimiter pairs; see :doc:`configuring_the_render_environment`.

Expressions at a Glance
=======================

An expression may appear inside ``{{ ... }}``, in a condition, on the right of ``set``, or after ``in`` in a loop.
Names read the active context, and dots read members of a map.
Missing names resolve to null.

.. list-table::
    :header-rows: 1
    :widths: 29 71

    * - Form
      - Compact example
    * - Name and map member
      - ``nest.species``, ``nest.site.region``
    * - Literals
      - ``'kirjosieppo'``, ``"kirjosieppo"``, ``3``, ``-4``, ``2.5``, ``1.2e3``, ``true``, ``false``, ``none``
    * - Collections
      - ``['muna', 'poikanen']``, ``{'species': 'kirjosieppo', 'eggs': 3}``
    * - Grouping and signs
      - ``(eggs + 1) * 2``, ``-eggs``, ``+eggs``
    * - Arithmetic
      - ``eggs + 1``, ``eggs - 1``, ``eggs * 2``, ``eggs / 2``
    * - Text concatenation
      - ``nest.species ~ ' / ' ~ nest.site.region``
    * - Comparison
      - ``eggs == 3``, ``eggs != 0``, ``eggs > 2``, ``eggs >= 2``, ``eggs < 5``, ``eggs <= 5``
    * - Membership
      - ``'kirjosieppo' in species``, ``'species' in nest``, ``'varis' not in species``
    * - Value test
      - ``nest.species is text``, ``missing is not none``
    * - Boolean logic
      - ``not closed``, ``eggs > 0 and active``, ``active or planned``
    * - Filter chain
      - ``nest.species | trim | capitalize``

String literals accept ``\\``, ``\'``, ``\"``, ``\b``, ``\f``, ``\n``, ``\r``, ``\t``, and ``\uXXXX`` escapes.
Lists and maps can be nested and may have one trailing comma.
Filters take at most two positional arguments after their input value.
They run from left to right.

From tightest to loosest, binding is: members and collections, filters, unary ``+`` and ``-``, ``*`` and ``/``, ``+``
and ``-`` and ``~``, comparisons and tests, ``not``, ``and``, then ``or``.
Use parentheses when that order is not immediately clear.
``and`` and ``or`` stop early and return an operand; comparisons and ``not`` return a Boolean.
Combine comparisons with ``and`` rather than chaining them.

Available ``is`` Tests
----------------------

Use ``value is test`` or ``value is not test``.
The tests are ``none`` (also ``null`` ), ``true``, ``false``, ``boolean``, ``integer``, ``float``, ``number``, ``text``
(also ``string`` ), ``list`` (also ``sequence`` ), ``map`` (also ``mapping`` ), ``iterable``, ``scalar``, ``even``, and
``odd``.
For example, ``eggs is odd`` and ``nest is mapping`` ask different questions about the same record.

Built-In Filters
================

A pipe sends the expression on its left to a filter.
For example, ``{{ nest.species | trim | capitalize }}`` first removes surrounding whitespace and then changes the
initial letter.
These examples show every built-in filter and its optional arguments; :doc:`template_language_syntax` gives a longer
example of filters in a rendered layout.

Text
----

.. list-table::
    :header-rows: 1
    :widths: 25 75

    * - Filter
      - Example
    * - ``capitalize``
      - ``{{ nest.species | capitalize }}``
    * - ``lower``
      - ``{{ nest.species | lower }}``
    * - ``upper``
      - ``{{ nest.species | upper }}``
    * - ``trim([characters])``
      - ``{{ nest.species | trim }}``, ``{{ '...pesä...' | trim('.') }}``
    * - ``replace(old, new)``
      - ``{{ nest.species | replace(' ', '-') }}``

Collections
-----------

.. list-table::
    :header-rows: 1
    :widths: 33 67

    * - Filter
      - Example
    * - ``first``, ``last``
      - ``{{ sightings | first }}``, ``{{ sightings | last }}``
    * - ``join([separator [, member]])``
      - ``{{ species | join(', ') }}``, ``{{ nests | join(', ', 'species') }}``
    * - ``length``, ``count``
      - ``{{ sightings | length }}``, ``{{ sightings | count }}``
    * - ``reverse``
      - ``{{ species | reverse | join(', ') }}``
    * - ``sort([reverse [, case_sensitive]])``
      - ``{{ species | sort | join(', ') }}``, ``{{ species | sort(true, true) | join(', ') }}``
    * - ``keys``, ``values``, ``items``
      - ``{{ nest | keys | join(', ') }}``, ``{{ nest | values | length }}``, ``{{ nest | items | length }}``

``first``, ``last``, ``min``, and ``max`` return null for an empty collection.
A list or map cannot be inserted directly; select a scalar, join suitable entries, or use a loop.

Numbers
-------

.. list-table::
    :header-rows: 1
    :widths: 30 70

    * - Filter
      - Example
    * - ``abs``
      - ``{{ change | abs }}``
    * - ``round([precision [, mode]])``
      - ``{{ average | round(1) }}``, ``{{ average | round(1, 'ceil') }}``
    * - ``sum([start])``
      - ``{{ clutch_sizes | sum(0) }}``
    * - ``min``, ``max``
      - ``{{ clutch_sizes | min }}``, ``{{ clutch_sizes | max }}``

The ``round`` mode is ``'common'`` (the default), ``'ceil'``, or ``'floor'``.

Defaults and JSON
-----------------

.. list-table::
    :header-rows: 1
    :widths: 34 66

    * - Filter
      - Example
    * - ``default`` or ``d``
      - ``{{ nest.owner | default('tuntematon') }}``, ``{{ eggs | d('tuntematon', true) }}``
    * - ``tojson([indent])``
      - ``{{ nest | tojson(2) | safe }}``

``default`` replaces null; its second argument also replaces other false values when true.
``tojson`` accepts an indentation from zero through 16 and returns ordinary text.
The ``safe`` modifier in the example prevents a second escaping pass when the JSON is inserted directly.

Statements and Layout Structure
===============================

Statements control which text is written and where values are available.
The examples below can be read as small layout fragments; :doc:`blocks_and_inheritance` develops the shared-layout
workflow, and :doc:`loading_render_layouts` explains how names such as ``parts/nest.txt`` are loaded.

.. list-table::
    :header-rows: 1
    :widths: 28 72

    * - Feature
      - Compact form
    * - Conditional
      - ``{% if eggs > 0 %}Occupied{% elif planned %}Planned{% else %}Empty{% endif %}``
    * - Local assignment
      - ``{% set next_count = eggs + 1 %}{{ next_count }}``
    * - List loop
      - ``{% for bird in species %}{{ bird }} {% else %}No birds{% endfor %}``
    * - Map loop
      - ``{% for name, count in clutches %}{{ name }}: {{ count }} {% endfor %}``
    * - Include
      - ``{% include "parts/nest.txt" %}``
    * - Optional include
      - ``{% include "parts/nest.txt" ignore missing %}``
    * - Isolated include
      - ``{% include "parts/nest.txt" without context %}``
    * - Inheritance
      - ``{% extends "base/nest.txt" %}``
    * - Named block
      - ``{% block observation %}No observation{% endblock %}``
    * - Parent block content
      - ``{% block observation %}{{ super() }}New detail{% endblock %}``

An include uses the current context by default; ``with context`` states that choice explicitly.
``ignore missing`` and one context modifier can be combined in either order.
Include and extends names are fixed string literals, not expressions.
An extending layout puts its output inside blocks; see :doc:`blocks_and_inheritance` for the exact outer-layout rules.

Inside a list loop, ``loop.index`` starts at one and ``loop.index0`` starts at zero.
``loop.revindex`` and ``loop.revindex0`` count from the end; ``loop.first``, ``loop.last``, and ``loop.length`` describe
the current iteration.
A loop's ``else`` runs when its collection is empty.
Each iteration has its own local scope, and ``set`` never changes the caller's context.
Within a block, ``super()`` renders the next parent implementation and ``super.super()`` reaches one further level.

Output Escaping
===============

The layout name selects automatic escaping: ``.html``, ``.xml``, ``.elcl``, ``.md``, and ``.json`` have built-in
formats; other suffixes write scalars without automatic escaping.
For an individual direct insertion, put an output modifier last:

.. list-table::
    :header-rows: 1
    :widths: 33 67

    * - Modifier
      - Example
    * - ``safe``
      - ``{{ trusted_fragment | safe }}``
    * - ``escape`` or ``e``
      - ``{{ nest.species | escape }}``, ``{{ nest.species | e }}``
    * - Explicit format
      - ``{{ nest.species | escape(json) }}``

``escape`` without an argument uses the format selected by the suffix, or HTML if no format was selected.
``safe`` and ``escape`` apply only to a direct output expression, not to assignments or conditions.
Keep the intended output format in mind when using ``safe``; it inserts the value without automatic escaping.
See :doc:`configuring_the_render_environment` for suffix mappings and custom escaping options.

A Working Layout
================

This compiled demo brings together map lookup, arithmetic, tests, filters, conditions, and a loop.
Its context supplies the data, while the layout decides how to present it.

.. erbsland-demo::
    :source: text/RenderLayouts/FeatureCheatSheet.cpp
    :exec: text/render_layouts --demo FeatureCheatSheet
    :source-sha256: cfedd793d0f6fd72d2552689373d969e38cfc173821c22c76de44e03561c2f9b

.. code-block:: cpp

    /// Render a bird nesting record from a compact layout.
    ///
    /// A map supplies named fields, a list supplies observations, and the layout combines expressions, tests, filters,
    /// conditions, local assignments, and iteration without changing the caller's context.
    /// @notest{Demo function verified by the documentation executable.}
    void featureCheatSheet() {
        using el::render::Value;
        using el::render::ValueList;
        using el::render::ValueMap;

        const auto directory = el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
        (directory->path() / "nest.txt"_el)
            .content()
            .writeTextOrThrow(
                "Pesä: {{ nest.species | capitalize }}\n"
                "{% set next_count = nest.eggs + 1 %}Munia: {{ nest.eggs }}; seuraava: {{ next_count }}\n"
                "{% if nest.eggs is odd %}Pariton määrä\n{% endif %}"
                "{% for sighting in sightings %}{{ loop.index }}. {{ sighting | trim }}\n{% endfor %}"
                "Merkitsijä: {{ keeper | default('tuntematon') }}\n"_el);

        // Prepare the record and its observations for one render call.
        auto nest = ValueMap{};
        nest.set("species"_el, Value{"kirjosieppo"_el}).set("eggs"_el, Value{3});
        const auto sightings = ValueList{Value{"  aamulla  "_el}, Value{"illalla"_el}};
        const auto context = el::render::Context{}.set("nest"_el, nest).set("sightings"_el, sightings);

        const auto environment = el::render::Environment::create();
        environment->addLayoutLoader(el::render::FileSystemLoader::create(directory->path()));
        el::io::print(environment->render("nest.txt"_el, context));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Pesä: Kirjosieppo
    Munia: 3; seuraava: 4
    Pariton määrä
    1. aamulla
    2. illalla
    Merkitsijä: tuntematon

.. erbsland-demo-end::
