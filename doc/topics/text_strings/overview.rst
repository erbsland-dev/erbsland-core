..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Text Strings; Overview

*********************
Text Strings Overview
*********************

A ``String`` is an owning Unicode value that can move safely through application code.
The topics below follow that value from basic use through searching, editing, and comparison.
Choose the question closest to the task at hand.

Using Strings as Everyday Values
================================

A ``String`` is the natural value for parameters, return values, and stored text.
Copies and slices can share backing storage, so clear value semantics do not require character data to be copied at
every boundary.
:doc:`/topics/text_strings/using_strings` explains literals, function parameters, storage, and the lifetime of shared slices.

Understanding String Attributes
===============================

A string has an encoded length, a decoded character count, an encoding-validity state, and a relationship to its backing
storage.
These questions have different costs, especially for variable-width encodings.
:doc:`/topics/text_strings/string_attributes` explains what each measurement means and when obtaining it requires a scan.

Transforming Read-only Strings
==============================

Ordinary changes are easiest to follow when the source remains unchanged and the operation returns the requested value.
That model works well for replacement, removal, trimming, case conversion, and normalization.
:doc:`/topics/text_strings/transforming_strings` shows how copy-returning operations fit together and when they can reuse storage.

Editing One Working Value in Place
==================================

Some algorithms are genuinely mutable: each edit changes the positions or content needed by the next step.
:cpp:type:`StringEditor <erbsland::text::StringEditor>` provides an owning local value for that workflow.
:doc:`/topics/text_strings/editing_strings_in_place` follows an editor through insertion, replacement, range changes, truncation, reuse, and
normalization before the result becomes a read-only ``String`` again.

Managing StringEditor Storage
=============================

Editors normally manage capacity automatically, but predictable growth and long-lived reuse sometimes justify more
control.
:doc:`/topics/text_strings/managing_string_editor_storage` explains native capacity units, exact reservation, copy-on-write detachment,
``clear()``, ``reset()``, ``shrinkToFit()``, and the diagnostics that reveal retained storage.

Finding Text Positions
======================

Searching is often only the first half of an operation; the resulting position may feed another search, a slice, a
replacement, or a diagnostic.
:doc:`/topics/text_strings/finding_text_positions` shows how native search indexes remain useful across those steps and how to avoid rescanning
text merely to change coordinate systems.

Slicing and Splitting Strings
=============================

A slice keeps one range, while a split turns a value into several fields.
Both can produce safe ``String`` values that share the source storage instead of copying their characters.
:doc:`/topics/text_strings/slicing_and_splitting_strings` explains range coordinates, boundary helpers, empty fields, and the memory lifetime
of shared results.

Accessing Characters and Writing Parsers
========================================

Iteration, indexed access, and parsing each move through text differently.
A range-based loop suits a complete inspection, a native index supports forward or reverse sequential reading, and
:cpp:class:`StringCharReader <erbsland::text::StringCharReader>` carries the richer state needed by a parser.
:doc:`/topics/text_strings/character_access_and_parsing` explains these choices and the distinction between storage and code-point
positions.

Comparing Whole and Partial Text
================================

Exact equality, case-insensitive matching, and identifier comparison answer different domain questions.
:doc:`/topics/text_strings/comparing_strings` begins with deterministic exact comparison, then introduces character comparison functions,
prefix and suffix tests, contained-text checks, and validation at untrusted boundaries.

Normalizing Unicode Strings
===========================

Unicode can represent visually equivalent text with different code-point sequences.
Normalization provides a deliberate representation for interchange, comparison, or indexing, but compatibility forms can
also change meaning.
:doc:`/topics/text_strings/normalizing_strings` explains how to choose NFC, NFD, NFKC, or NFKD and why a concatenation boundary may require
another normalization pass.

Working with Character Sets
===========================

Many validators and cleanup passes are naturally described by a set of accepted or unwanted Unicode code points.
:doc:`/topics/text_strings/working_with_character_sets` shows how to construct and reuse those sets, combine policies, validate text, and
apply the same policy to searching, trimming, removal, and transformation.

Finding Similar Text by Edits
=============================

When a user mistypes a short name, a nearby candidate can be more useful than a bare lookup failure.
:doc:`/topics/text_strings/fuzzy_matching` shows how to rank a known list by Unicode edit distance, tune the comparison policy, and read
stable, deduplicated suggestions.
