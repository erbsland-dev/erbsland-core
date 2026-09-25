..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Text Formatting; Overview

************************
Text Formatting Overview
************************

A short result may be assembled from fragments or shaped with a format pattern.
These topics show how to build strings and describe the appearance of individual values.
For longer documents with conditions or repeated sections, see :doc:`/topics/text_rendering/index`.
For substituting named sources and filters in text, see :doc:`/topics/text_placeholders/index`.

Building Strings Efficiently
============================

A fixed set of fragments, a dynamically collected list, a mutable editor, and a width-independent builder have different
allocation behavior even when they produce identical text.
:doc:`building_strings` compares these approaches and explains when one exact allocation can replace repeated growth
and copying.

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
