..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Configuration; Value Trees
    single: Configuration; Name Paths
    single: Value; Tree Navigation

********************************
Working with Configuration Trees
********************************

Every parsed ELCL document is a tree of :cpp:class:`Value <erbsland::conf::Value>` objects.
Most applications can read settings directly with the typed ``get...()`` methods described in
:doc:`parsing-documents`.
Tree navigation becomes useful when the document contains repeated sections, when code processes an unknown set of
children, or when diagnostics need the identity and location of the original value.

This page explains how ELCL structure maps to the tree, how name paths address its nodes, and how to move through both
named sections and lists.

How ELCL Becomes a Tree
=======================

The :cpp:class:`Document <erbsland::conf::Document>` is the root value.
An ELCL section becomes a container below that root, and each assignment becomes a child of its current section.
Nested section names create further levels.
A value list contains scalar child values, while a section list contains one section value for each repeated section.

For example, the path ``patch.filter.cutoff`` describes three steps from the document root: the ``patch`` section, its
``filter`` subsection, and the ``cutoff`` integer.
The path ``patch.oscillators[1].waveform`` includes a list index and reaches the waveform in the second oscillator
section.
Each node knows its type, its children, its parent, and its absolute path in this structure.

Address Values with Name Paths
==============================

A :cpp:class:`NamePath <erbsland::conf::NamePath>` is an ordered sequence of
:cpp:class:`Name <erbsland::conf::Name>` objects.
Regular names select named children and indexes select entries in value or section lists.
Text names and their indexes use the same model, so one path type can represent every route through a document.

Lookup methods accept :cpp:type:`NamePathLike <erbsland::conf::NamePathLike>`.
This variant lets you pass a Core :cpp:type:`String <erbsland::text::String>`, one ``Name``, a complete ``NamePath``, or
a ``std::size_t`` index.
Use a string for a concise literal path, an index while traversing a list, and explicit name objects when code
constructs or reuses paths dynamically.

.. erbsland-demo::
    :source: conf/ValueTrees/NamePaths.cpp
    :exec: conf/value_trees --demo NamePaths
    :source-sha256: 8dd59eb5c6ac8b107d534248433255f11f35903d7805380397c838e9d7b0fa34

.. code-block:: cpp

    /// Address values with strings, names, name paths, and indexes.
    ///
    /// Value-tree methods accept `NamePathLike`, so simple code can pass a Core string while code that reuses or constructs
    /// paths can pass `Name` or `NamePath`. Lists additionally accept an index directly.
    void namePaths() {
        const auto configuration = "[patch]\n"
                                   "name: \"Gökyüzü\"\n"
                                   "*[patch.oscillators]*\n"
                                   "waveform: \"sine\"\n"_el;
        const auto document = el::conf::Parser{}.parseTextOrThrow(configuration);

        // A string is the concise choice for a complete path.
        const auto patchName = document->getTextOrThrow("patch.name"_el);

        // Name and NamePath objects are useful when paths are assembled or reused.
        const auto patch = document->valueOrThrow(el::conf::Name::createRegular("patch"_el));
        const auto oscillatorPath = el::conf::NamePath::fromText("oscillators"_el);
        const auto oscillators = patch->valueOrThrow(oscillatorPath);

        // Lists accept a numeric index as a path-like value.
        const auto firstOscillator = oscillators->valueOrThrow(std::size_t{0});
        el::io::printLine(patchName, " waveform: "_el, firstOscillator->getTextOrThrow("waveform"_el));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Gökyüzü waveform: sine

.. erbsland-demo-end::

String paths use the same normalized regular names as ELCL documents.
They also support list indexes, for example ``oscillators[0]``, and quoted text names where required.
:cpp:func:`NamePath::fromText() <erbsland::conf::NamePath::fromText>` is useful when a parsed, reusable path is more
appropriate than passing the string directly.

Identify a Value in the Document
================================

:cpp:func:`name() <erbsland::conf::Value::name>` returns the local name of a value: the final name or index that selects
it from its parent.
:cpp:func:`namePath() <erbsland::conf::Value::namePath>` returns the complete path from the document root.
The distinction matters in repeated structures, where several values may share the local name ``waveform`` but have
different indexed paths.

.. erbsland-demo::
    :source: conf/ValueTrees/ValueNames.cpp
    :exec: conf/value_trees --demo ValueNames
    :source-sha256: bbf7dd5fdc03f32c3c040c2a3a5451acd65814608484dc4c7f8b2951166fa941

.. code-block:: cpp

    /// Inspect a value's local name and absolute name path.
    ///
    /// `name()` identifies a value among its siblings. `namePath()` describes the route from the document root, including
    /// indexes introduced by value lists and section lists.
    void valueNames() {
        const auto configuration = "*[patch.oscillators]*\n"
                                   "waveform: \"square\"\n"
                                   "*[patch.oscillators]*\n"
                                   "waveform: \"triangle\"\n"_el;
        const auto document = el::conf::Parser{}.parseTextOrThrow(configuration);
        const auto waveform = document->valueOrThrow("patch.oscillators[1].waveform"_el);

        el::io::printLine("Local name: "_el, waveform->name().toPathText());
        el::io::printLine("Absolute path: "_el, waveform->namePath().toText());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Local name: waveform
    Absolute path: patch.oscillators[1].waveform

.. erbsland-demo-end::

Use the local name when processing siblings in one known container.
Use the absolute path in diagnostics, maps, and logs where the value must remain unambiguous outside that immediate
context.

Navigate Between Parents and Children
=====================================

:cpp:func:`hasValue() <erbsland::conf::Value::hasValue>` answers whether a child exists at a relative name path.
:cpp:func:`value() <erbsland::conf::Value::value>` returns that child or ``nullptr`` when it cannot be resolved.
This is a natural fit for optional branches.
For required structure, :cpp:func:`valueOrThrow() <erbsland::conf::Value::valueOrThrow>` reports a configuration error
when the path is malformed or absent.

Lookups are relative to the value on which they are called.
The document can therefore resolve ``patch.filter.cutoff`` in one step, while the ``patch`` section resolves the shorter
``filter.cutoff`` path.
This makes it possible to hand one branch to a component without teaching it the branch's absolute location.

Every value below the root reports ``true`` from :cpp:func:`hasParent() <erbsland::conf::Value::hasParent>` and returns
its container from :cpp:func:`parent() <erbsland::conf::Value::parent>`.
The document itself has no parent and returns ``nullptr``.

.. erbsland-demo::
    :source: conf/ValueTrees/NavigatingTree.cpp
    :exec: conf/value_trees --demo NavigatingTree
    :source-sha256: 2e38a6f9bb49c13f5cd542d3785573bcfca3b1e1f866d4be866892dabf0f5698

.. code-block:: cpp

    /// Navigate down to child values and back to their parents.
    ///
    /// Use `hasValue()` for an inexpensive existence test, `value()` for optional branches, and `valueOrThrow()` for
    /// required branches. Every non-root value keeps a parent link, making it possible to return to its container.
    void navigatingTree() {
        const auto configuration = "[patch]\n"
                                   "name: \"Sessiz Kıyı\"\n"
                                   "[patch.filter]\n"
                                   "mode: \"low-pass\"\n"
                                   "cutoff: 2400\n"_el;
        const auto document = el::conf::Parser{}.parseTextOrThrow(configuration);
        const auto patch = document->valueOrThrow("patch"_el);

        if (patch->hasValue("filter"_el)) {
            const auto filter = patch->value("filter"_el);
            const auto cutoff = filter->valueOrThrow("cutoff"_el);
            el::io::printLine("Cutoff: "_el, cutoff->asIntegerOrThrow(), " Hz"_el);
            el::io::printLine("Container: "_el, cutoff->parent()->namePath().toText());
        }

        el::io::printLine("Document has parent: "_el, el::BooleanFormat::yesNo(), document->hasParent());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Cutoff: 2400 Hz
    Container: patch.filter
    Document has parent: no

.. erbsland-demo-end::

Prefer a single ``value()`` call and a ``nullptr`` test when you need the optional value itself.
Calling ``hasValue()`` first is most useful when existence alone changes the control flow; otherwise it performs the
same lookup twice.

Iterate over Lists and Sections
===============================

Every container value supports range-based iteration through
:cpp:class:`ValueIterator <erbsland::conf::ValueIterator>`.
For a named section, iteration visits its named children in document order.
For a value list or section list, it visits the indexed entries.
The loop variable is a shared value pointer, so each entry supports the same lookup, conversion, and metadata API as any
other node.

.. erbsland-demo::
    :source: conf/ValueTrees/IteratingLists.cpp
    :exec: conf/value_trees --demo IteratingLists
    :source-sha256: c4307ef9dbb2def0ba8723a676c342e29d45a53c5492a366ee1a56b8d9d23d9b

.. code-block:: cpp

    /// Iterate over section-list entries in document order.
    ///
    /// Every container supports range-based iteration. When random access is more convenient, combine `size()` with
    /// `valueOrThrow(index)`; both approaches return the same child values.
    void iteratingLists() {
        const auto configuration = "*[patch.oscillators]*\n"
                                   "waveform: \"sine\"\n"
                                   "octave: 0\n"
                                   "*[patch.oscillators]*\n"
                                   "waveform: \"triangle\"\n"
                                   "octave: 1\n"_el;
        const auto document = el::conf::Parser{}.parseTextOrThrow(configuration);
        const auto oscillators = document->valueOrThrow("patch.oscillators"_el);

        // Range-based iteration is the clearest choice when every entry is processed.
        for (const auto &oscillator : *oscillators) {
            el::io::printLine(
                oscillator->getTextOrThrow("waveform"_el), " at octave "_el, oscillator->getIntegerOrThrow("octave"_el));
        }

        // Indexed access is available when position matters.
        const auto last = oscillators->valueOrThrow(oscillators->size() - 1);
        el::io::printLine("Last oscillator: "_el, last->getTextOrThrow("waveform"_el));
    }

.. erbsland-ansi::
    :escape-char: ␛

    sine at octave 0
    triangle at octave 1
    Last oscillator: triangle

.. erbsland-demo-end::

When position matters, :cpp:func:`size() <erbsland::conf::Value::size>` reports the number of children and ``value()``
or ``valueOrThrow()`` accepts a numeric index.
Range-based iteration is usually clearer for processing every entry, while indexed access is useful for selected
positions or algorithms that compare neighboring values.

Continue with Individual Values
===============================

Navigation locates the value; the next step is interpreting its content.
:doc:`parsing-documents` already introduces the recommended typed getters for ordinary configuration loading.
:doc:`individual-values` continues with direct ``as...()`` conversion, uniform lists and matrices, template getters,
and type tests in more detail.
