..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Guidelines; Maintenance Tool Documentation

*****************************************
Maintenance Tool Documentation Guidelines
*****************************************

The pages under ``doc/development/tool_*`` serve three different readers: someone running a command, someone extending
the maintenance tool, and someone looking up an exact technical contract.
Keep each page focused on one of these purposes.
Follow :doc:`rst_style` and :doc:`writing_style` for formatting and prose.

Choose the Destination
======================

.. list-table::
    :header-rows: 1
    :widths: auto

    * - Destination
      - Reader's question
      - Content
    * - ``tool_usage/usage.rst``
      - How do I invoke the maintenance tool?
      - Shared command syntax, help, executable selection, project and configuration selection, logging, and exit status
    * - ``tool_usage/<command>.rst``
      - How do I use this command to complete a task?
      - Purpose, prerequisites, ordinary workflow, common options, outputs to review, and actionable failure guidance
    * - ``tool_reference/<subject>_reference.rst``
      - What exactly does this setting, format, or implementation do?
      - Exhaustive options, configuration defaults and validation, data formats, algorithms, runtime contracts, and verification
    * - ``tool_dev/*.rst``
      - How do I build, extend, or test the tool?
      - Contributor workflows, framework concepts, authoring instructions, and examples of implementing commands or sources

The generic usage page must remain command-independent.
Use ``COMMAND`` as an explicitly explained placeholder in generic invocation examples.
Do not append a new command's recipe, output paths, configuration fields, or check-mode behavior there.
Discoverability belongs in the usage index and the command's own page.

Write a Command Usage Page
==========================

Every registered command has one user-facing page named after its canonical command, with hyphens replaced by
underscores: ``generate-display-texts`` uses ``generate_display_texts.rst``.
Start with what the command does and when to use it.
Show the normal invocation, identify the files it reads and writes, and explain what the user should review afterward.
Describe common variations, such as checking without writing or selecting another output, when the command supports
them.
Link to the generic usage page for shared invocation details rather than repeating them.

A usage page may name the configuration file and show a useful example.
Keep complete field inventories, parsing rules, encoding layouts, internal classes, and implementation limits in the
reference page.
Link to that reference where a user may need more detail.
Shared user tasks, such as customizing generated notices or updating source snapshots, may have their own usage guides.
They do not replace the individual command pages.

Write Technical Reference Pages
===============================

A reference page specifies behavior rather than giving a step-by-step task recipe.
Document each command-specific option and configuration field, including its type, required or optional status, default,
validation, path base, and precedence where applicable.
Specify input and output formats, integrity checks, and meaningful failure or compatibility rules.
Algorithms and implementation details belong here when they explain behavior or help maintainers verify it.

Keep shared contracts in one reference page and link to them from the commands that use them.
For example, both Unicode consumers should point to the same Unicode configuration contract.
Use configuration or format examples to illustrate a specification; put regeneration and update sequences in usage
guides.
Reference pages may link to those guides, but must not become a second copy of them.
Instructions for implementing a command belong in ``tool_dev``.

Maintain Navigation and Boundaries
==================================

When adding or changing a command:

1. Add or update its usage page and the ``tool_usage/index.rst`` toctree.
2. Add or update its technical reference and the ``tool_reference/index.rst`` toctree.
3. Link the usage and reference pages to each other.
4. Update contributor guides only if the extension workflow or framework changes.
5. Verify documented names, defaults, and paths against the command implementation and checked-in ELCL configuration.
6. Remove duplicated details from other pages instead of adding another explanation beside them.

Preserve useful explicit reference labels when relocating sections, and update page links that should now lead to a more
appropriate destination.
Keep index pages short: explain their audience and provide navigation rather than command recipes.
Run ``doc-process-rst`` for edited pages and an incremental documentation build to check links and rendering.
