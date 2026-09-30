..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: JSON Values

***********
JSON Values
***********

Introduction
============

``JsonValue`` is a copy-on-write tree for JSON null, Boolean, integer, floating-point, string, array, and object values.
``JsonParseOptions`` controls limits and ``JsonFormatOptions`` controls compact or indented output.
See :doc:`/topics/data/parsing_and_rendering_json` for parsing, traversal, and serialization examples.

Interface
=========

.. doxygentypedef:: erbsland::data::json::JsonArray

.. doxygentypedef:: erbsland::data::json::JsonObject
.. doxygenclass:: erbsland::data::json::JsonFormatOptions
    :members:
.. doxygenclass:: erbsland::data::json::JsonParseOptions
    :members:
.. doxygenenum:: erbsland::data::json::JsonType
.. doxygenclass:: erbsland::data::json::JsonValue
    :members:
