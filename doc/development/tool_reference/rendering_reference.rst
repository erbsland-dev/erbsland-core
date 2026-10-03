..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

***********************
Rendering Configuration
***********************

Settings
========

For practical customization, see :doc:`../tool_usage/rendering_configuration`.

The required ``[rendering]`` section configures generated headers.
The main configuration includes ``tool/conf/misc/rendering.elcl``, which contains an empty section and commented
examples.
Validation supplies these defaults:

.. code-block:: elcl

    [rendering]
    copyright_line: "Copyright (c) ${var:year} Tobias Erbsland - https://erbsland.dev"
    license_line: "SPDX-License-Identifier: Apache-2.0"

Expansion and Validation
========================

The application captures the current UTC year once at startup and exposes it as ``${var:year}``.
Core's ELCL parser expands placeholders in configuration text and included files before validation.
Validation rules supply omitted settings automatically, with the copyright default using the captured UTC year.
Expanded headers must be nonempty single-line text; unknown rendering settings and placeholder errors fail before
command execution.

Templates add their own comment prefixes and have globals ``current_year``, ``copyright_line``, and ``license_line``.
Local rendering context values override these globals.

Environment Variables and Filters
=================================

Built-in variable and environment sources and all text filters are enabled.
Use ``${env:NAME}`` for an environment variable; an absent value expands to ``undefined``.
``${env:NAME,required}`` reports an absent variable as a parser error.
Filters follow the source with ``|``, for example ``${env:OWNER,required|trim|upper:ascii}``.

The built-in filters include ``safe``, ``trim``, ``slice``, ``remove``, ``replace``, ``escape``, ``if``, ``error_if``,
``default``, ``required``, ``lower``, and ``upper``.
See :doc:`/topics/text_placeholders/built_in_sources` and
:doc:`/topics/text_placeholders/built_in_filters` for parameters.

Literal Placeholders
====================

Within ELCL quoted text, escape the dollar sign as ``\${var:year}`` to retain the literal placeholder text.
ELCL handles that escape before placeholder expansion.
Ordinary ELCL escapes still apply.

Reproducible Output
===================

The default ``lower`` and ``upper`` modes and explicit ``:unicode`` modes depend on linked Core's Unicode tables.
``lower:ascii`` and ``upper:ascii`` do not.
Defaults use no Unicode-sensitive transformations.
For reproducibility, keep the captured year, environment, and resolved configuration constant.

Source manifests are read without placeholder providers and retain literal text.
For the C++ rendering interface, see :ref:`development-tool-reference-template-rendering`.
