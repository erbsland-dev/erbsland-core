..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    !single: Text
    single: Text; Overview
    single: String
    single: StringEditor
    single: AnyStringBuilder
    single: Unicode

*********************
Text in Erbsland Core
*********************

Text rarely stays in one form for the lifetime of an application.
It enters through literals, files, network protocols, or user input; it is searched and validated; parts are retained;
new values are assembled; and eventually it crosses another encoding boundary.
The text domain provides one consistent Unicode model for that complete journey.

Most application code begins with :cpp:type:`String <erbsland::text::String>`, the owning read-only UTF-8 value.
From there, focused tools support the places where text must be mutated, parsed, formatted, or converted without giving
up safe ownership.
The sections below introduce each text topic and help you choose where to continue.

Using Strings as Everyday Values
================================

A ``String`` is the natural value for parameters, return values, and stored text.
Copies and slices can share backing storage, so clear value semantics do not require character data to be copied at
every boundary.
:doc:`using_strings` explains literals, function parameters, storage, and the lifetime of shared slices.

Understanding String Attributes
===============================

A string has an encoded length, a decoded character count, an encoding-validity state, and a relationship to its backing
storage.
These questions have different costs, especially for variable-width encodings.
:doc:`string_attributes` explains what each measurement means and when obtaining it requires a scan.

Transforming Read-only Strings
==============================

Ordinary changes are easiest to follow when the source remains unchanged and the operation returns the requested value.
That model works well for replacement, removal, trimming, case conversion, and normalization.
:doc:`transforming_strings` shows how copy-returning operations fit together and when they can reuse storage.

Editing One Working Value in Place
==================================

Some algorithms are genuinely mutable: each edit changes the positions or content needed by the next step.
:cpp:type:`StringEditor <erbsland::text::StringEditor>` provides an owning local value for that workflow.
:doc:`editing_strings_in_place` follows an editor through insertion, replacement, range changes, truncation, reuse, and
normalization before the result becomes a read-only ``String`` again.

Managing StringEditor Storage
=============================

Editors normally manage capacity automatically, but predictable growth and long-lived reuse sometimes justify more
control.
:doc:`managing_string_editor_storage` explains native capacity units, exact reservation, copy-on-write detachment,
``clear()``, ``reset()``, ``shrinkToFit()``, and the diagnostics that reveal retained storage.

Finding Text Positions
======================

Searching is often only the first half of an operation; the resulting position may feed another search, a slice, a
replacement, or a diagnostic.
:doc:`finding_text_positions` shows how native search indexes remain useful across those steps and how to avoid rescanning
text merely to change coordinate systems.

Slicing and Splitting Strings
=============================

A slice keeps one range, while a split turns a value into several fields.
Both can produce safe ``String`` values that share the source storage instead of copying their characters.
:doc:`slicing_and_splitting_strings` explains range coordinates, boundary helpers, empty fields, and the memory lifetime
of shared results.

Accessing Characters and Writing Parsers
========================================

Iteration, indexed access, and parsing each move through text differently.
A range-based loop suits a complete inspection, a native index supports forward or reverse sequential reading, and
:cpp:class:`StringCharReader <erbsland::text::StringCharReader>` carries the richer state needed by a parser.
:doc:`character_access_and_parsing` explains these choices and the distinction between storage and code-point
positions.

Converting Text and Scalar Values
=================================

Configuration and protocol values often cross a boundary between text and Booleans, integers, or floating-point numbers.
:doc:`converting_text_and_scalar_values` explains fallback and throwing conversion, accepted input syntax, overflow,
and the formatting options that create text for a particular destination.

Comparing Whole and Partial Text
================================

Exact equality, case-insensitive matching, and identifier comparison answer different domain questions.
:doc:`comparing_strings` begins with deterministic exact comparison, then introduces character comparison functions,
prefix and suffix tests, contained-text checks, and validation at untrusted boundaries.

Normalizing Unicode Strings
===========================

Unicode can represent visually equivalent text with different code-point sequences.
Normalization provides a deliberate representation for interchange, comparison, or indexing, but compatibility forms can
also change meaning.
:doc:`normalizing_strings` explains how to choose NFC, NFD, NFKC, or NFKD and why a concatenation boundary may require
another normalization pass.

Working with Character Sets
===========================

Many validators and cleanup passes are naturally described by a set of accepted or unwanted Unicode code points.
:doc:`working_with_character_sets` shows how to construct and reuse those sets, combine policies, validate text, and
apply the same policy to searching, trimming, removal, and transformation.

Building Strings Efficiently
============================

A fixed set of fragments, a dynamically collected list, a mutable editor, and a width-independent builder have different
allocation behavior even when they produce identical text.
:doc:`building_strings` compares these approaches directly and explains when one exact allocation can replace repeated
growth and copying.

Formatting Structured Text
==========================

When output has a stable shape, a format pattern keeps the surrounding text and value positions visible in one place.
:doc:`using_string_format` explains reusable patterns, automatic and positional placeholders, supported argument types,
and the choice between building a result and appending it to an existing builder.

Describing Values with Named Format Specifications
==================================================

Formatting choices are easier to review when they name both the expected value family and each requested behavior.
:doc:`format_specifications` introduces the primary named syntax and covers every text, integer, floating-point,
Boolean, and byte-block option.

Maintaining Compact Compatibility Specifications
================================================

Compact C++-style specifications remain useful for existing patterns and familiar small fields.
:doc:`compatibility_format_specifications` documents the supported subset, its Core escaping extensions, and the limits
that distinguish it from both ``std::format`` and the named syntax.

Recognizing Small Text Patterns
===============================

Some boundaries are more expressive than a fixed prefix but do not justify a full parser.
:cpp:class:`StringPattern <erbsland::text::StringPattern>` describes these small shapes with literals, character ranges,
and a front/back divider.
:doc:`using_string_patterns` explains where patterns fit and how they match, trim, split, and return native positions.

Converting Strings and Encoded Data
===================================

Changing string width, encoding text into bytes, reading a text stream, and retaining a width-erased value are distinct
boundary operations.
:doc:`conversion` separates those workflows and explains ``StringConverter``, encoders and decoders, byte-order marks,
strict and tolerant handling, streams, ``AnyString``, and width-independent output.

Encoding Internationalized Names
================================

Internationalized domain names require more than a Unicode-to-ASCII codec.
IDNA2008 adds normalization, contextual, directionality, and DNS rules around the underlying Punycode representation.
:doc:`encoding_internationalized_names` explains when raw Punycode is appropriate and how to preserve useful failure
information at a domain-name boundary.
