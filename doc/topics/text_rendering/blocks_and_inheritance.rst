..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Text Rendering; Blocks
    single: Text Rendering; Layout Inheritance
    single: Layouts; Extending a Layout
    single: Layouts; Super Blocks

********************************
Sharing Layouts with Inheritance
********************************

Related documents often have the same shape but need different passages.
For example, several sound observation notes can share a heading and conclusion while each records a different
observation.
Blocks give those passages names; inheritance lets another layout replace them without copying the whole document.
This page shows how to build that family of layouts and how to retain content from a parent block.

The examples use the default markers: ``{{ ... }}`` inserts a value and ``{% ... %}`` contains a statement.
These markers are configurable for the entire render environment.
If your document format uses braces already, see :doc:`configuring_the_render_environment` before writing the layouts.

Give a Passage a Name
=====================

A base layout should read as a useful document on its own.
Surround a passage with ``{% block observation %}`` and ``{% endblock %}`` to name a place that another layout may
replace.
When you render the base directly, the content between those markers is its default.
The ordinary text before and after the block renders as usual.

.. erbsland-demo::
    :source: text/RenderLayouts/LayoutInheritance.cpp
    :function-blocks: blockDefaults
    :function-blocks-sha256: 35b7bbd29e47ace69dae65bd454f31052926a51c18d984be0eca92bb26387276
    :exec: text/render_layouts --demo BlockDefaults
    :source-sha256: d1878c5ffc2d3c1a5927a16bbbffd8b1e159a9ec3bcfd1a31b97a46595ca3b10

.. code-block:: cpp

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

.. erbsland-ansi::
    :escape-char: ␛

    声学笔记: 山谷回声
    观察: 尚无记录。
    结论: 请比较声源与回声。

.. erbsland-demo-end::

Here the default observation makes the note complete even before a specialized layout exists.
The ``subject`` value still comes from the render context, just as it would in any other layout.
A block is therefore a choice about *which passage to render*, while an expression supplies a value *within* a passage.

Extend the Base Layout
======================

Now a note about an echo can keep the base layout's heading and replace only its observation.
The derived layout begins with ``{% extends "base.txt" %}`` and declares a block with the same name.
Render the derived layout by its name; the renderer uses the base document as the outer shape and selects the derived
version of each overridden block.
Blocks without an override keep their base content.

.. erbsland-demo::
    :source: text/RenderLayouts/LayoutInheritance.cpp
    :function-blocks: layoutInheritance
    :function-blocks-sha256: 27a04e2e287b4f225a9247e0d6ce5b83187cfa4a772967584a39d38222f33701
    :exec: text/render_layouts --demo LayoutInheritance
    :source-sha256: d1878c5ffc2d3c1a5927a16bbbffd8b1e159a9ec3bcfd1a31b97a46595ca3b10

.. code-block:: cpp

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

.. erbsland-ansi::
    :escape-char: ␛

    声学笔记: 山谷回声
    观察: 拍手后听到两次回声。
    解释: 声波遇到表面会反射。
    补充: 两次反射来自不同的岩壁。

.. erbsland-demo-end::

The output starts with the heading from ``base.txt``, then uses the observation from ``echo.txt``.
Both layouts read the same context, so the derived passage can refer to values supplied for the render call.
The parent name in ``extends`` is a fixed logical layout name resolved by the environment's loaders.
This makes a file loader, a resource loader, or another registered loader equally suitable for a layout family; see
:doc:`loading_render_layouts` for the loader workflow.

Keep the Parent's Wording
=========================

Sometimes a derived block needs to add detail rather than replace everything.
The ``interpretation`` block above starts with ``{{ super() }}``.
That call renders the next implementation of the *same block*, then the derived layout writes its additional line.
The result contains both the general explanation from the base and the observation-specific detail.

Place ``super()`` inside the block that owns the passage.
The parser rejects it outside a block, and asking for an earlier implementation that does not exist fails while
rendering.
If you need to transform the inherited text before inserting it, a ``super()`` result can also participate in an
expression such as a filter or a ``set`` assignment.
For the usual case of adding a line, direct ``{{ super() }}`` keeps the layout easier to read.

Follow a Longer Inheritance Chain
=================================

A layout can extend another derived layout.
In that case, ``super()`` refers to the next block implementation in the chain, rather than always jumping to the oldest
base.
The next example adds an outdoor note between the general base and a valley note.
The valley block first keeps the outdoor wording, then uses ``super.super()`` to reach the base wording directly.

.. erbsland-demo::
    :source: text/RenderLayouts/LayoutInheritance.cpp
    :function-blocks: chainedInheritance
    :function-blocks-sha256: 60be7cd1a58b1a049c04dcca038396f871e55349c242d1fb589de005613fadc5
    :exec: text/render_layouts --demo ChainedInheritance
    :source-sha256: d1878c5ffc2d3c1a5927a16bbbffd8b1e159a9ec3bcfd1a31b97a46595ca3b10

.. code-block:: cpp

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

.. erbsland-ansi::
    :escape-char: ␛

    基础: 声波可以反射。
    户外: 山坡也会反射声波。
    直达基础: 基础: 声波可以反射。

.. erbsland-demo-end::

Notice that the base line appears twice: once through the outdoor block's own ``super()`` call and once through the
valley block's direct ``super.super()`` call.
The example makes the chain visible; in a real layout, choose the call that produces the wording you want.
Only these two depths are available, and asking for a block implementation that is not there is a render error.

Keep Each Layout's Role Clear
=============================

Put the shared document structure in the base and the varying passages in named blocks.
An extending layout can contain block declarations, top-level ``set`` assignments, comments, and whitespace outside
those blocks.
It cannot write ordinary text or expressions there: the base layout owns the outer document, so such content has no
unambiguous place in the result.
Give each block a unique ASCII identifier within its layout, and close it with ``{% endblock %}``.

For values and local assignments used inside blocks, see :doc:`preparing_render_contexts` and
:doc:`template_language_syntax`.
The :doc:`rendering reference </reference/text/documents_and_rendering>` lists the exact syntax and error conditions for
inheritance, blocks, and ``super()``.
The :doc:`template_feature_cheat_sheet` keeps their short forms alongside the other layout statements.
