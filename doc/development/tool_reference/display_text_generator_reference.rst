..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

********************************
Display Text Generator Reference
********************************

``generate-display-texts`` regenerates Core's checked-in base catalog.
It shares ``DisplayTextCompiler`` with the consumer CMake compiler.
See :doc:`../tool_usage/generate_display_texts` for editing, regeneration, and check workflows.
The runtime catalog representation and lookup behavior are described in
:doc:`/reference/i18n/internationalization`.

Configuration
=============

The command uses ``[Generate Display Texts]`` in ``tool/conf/tools/generate_display_texts.elcl``.
The section is required; an empty section supplies all defaults below.
Every setting is text, and there are no command-line path, class, or namespace overrides.

.. list-table::
    :header-rows: 1
    :widths: auto

    * - Setting
      - Default
    * - ``Input File``
      - ``src/erbsland/i18n/impl/DisplayTextCore.elcl``
    * - ``Output Header``
      - ``src/erbsland/i18n/impl/DisplayTextCore.hpp``
    * - ``Output Source``
      - ``src/erbsland/i18n/impl/DisplayTextCore.cpp``
    * - ``Class Name``
      - ``DisplayTextCore``
    * - ``Namespace``
      - ``erbsland::i18n::impl``

Paths resolve from the selected project root and must remain inside it.
Input and outputs must be distinct; output parent directories must already exist.
``Class Name`` must be a non-reserved C++ identifier.
``Namespace`` accepts a qualified sequence of such identifiers without a leading separator, or empty text for a global
class.
The shared rendering settings supply copyright and license notices; see :doc:`rendering_reference`.

Rendering and Output
====================

``--check`` (``-c``) compares both outputs without writing and returns failure if either is missing or differs.
Without this flag, generation replaces changed files through the project file service.
Comparisons ignore line-ending and first-line copyright-year differences.
Catalog parsing rejects includes, non-text values, and unsupported section types before rendering either output.

The compiler converts sorted domains and entries into structured render values, escaping text as C++ literals.
The shared header and implementation templates live in ``cmake/tools/display-text-compiler/data``.
Both executables embed these files as compiled resources; neither needs template files at runtime.
The maintenance tool uses the project file service for confined reads and replacement writes.

Core builds consume the checked-in generated files directly.
There is no bootstrap library and no automatic regeneration of Core's base catalog.
Consumer applications and demos use ``erbsland_core_display_text`` to generate their own build-directory artifacts with
the normal display-text compiler target.
