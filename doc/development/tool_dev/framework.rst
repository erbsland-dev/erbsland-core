..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

******************************
Maintenance Framework Overview
******************************

The framework handles command-line parsing, configuration validation, logging, project file access, and rendering.
A command defines the work to perform; shared data sources provide verified inputs that several commands can use.

.. _development-tool-framework-where-the-code-lives:

Where the Code Lives
====================

The application derives from :cpp:class:`Application <erbsland::core::Application>`.
Its shared framework lives in ``tool::frame``, under ``tool/src/tool/frame``.
Each command has its own namespace and matching source directory under ``tool/src/tool``.

The build separates the entry point from reusable implementation:

.. list-table::
    :header-rows: 1
    :widths: auto

    * - Target
      - Contents
    * - ``erbsland-core-tool-lib``
      - Framework, data sources, commands, and compiled template resources
    * - ``erbsland-core-tool``
      - ``main.cpp``, linked to the implementation library
    * - ``erbsland-core-tool-unittest``
      - Tests linked to the same implementation library

Code Style when Writing Tools
=============================

The maintenance tool is an Erbsland Core *consumer*, and therefore should follow all rules for such applications.

* Only use public headers, like ``#include <erbsland/Application.hpp>``
* Use the convenient shortened ``el::`` namespace and not ``erbsland::``.
* Use Erbsland Core strings, containers, paths, streams, formatting, and regular expressions.
* Use the standard library types only where no Erbsland Core alternatives exist.

Commands and Sources
====================

``tool::frame::Tool`` defines a command's identity, options, configuration, and execution callbacks.
The application selects one command for each invocation.
Follow :doc:`write_command` to add one.

``tool::frame::DataSource`` separates input configuration and loading from command-specific output generation.
The application configures the command before initializing its requested sources.
Read :doc:`data_sources` for the existing sources and :doc:`write_data_source` to add another.

Application Services
====================

Commands and sources access the active application through ``tool::frame::app()`` during their callbacks.
The application supplies ``ProjectFiles`` for checked project paths and file operations, and a render environment for
compiled templates.
Both base classes expose a protected ``log()`` method for progress and diagnostics.

See :doc:`../tool_reference/framework_reference` for startup order, configuration rules, file access, and rendering.

Pre-commit Tasks
================

``tool::frame::PreCommitTask`` is an independent interface for explicitly selected maintenance operations.
A command can inherit both ``Tool`` and ``PreCommitTask``: one override each of ``identifier()``, ``title()``, and
``execute()`` satisfies both interfaces.
Keep the actual maintenance algorithm in that shared execution body or its existing helper.
Task-only implementations need these three methods without command-line metadata.

Register a participating command with ``app.registerPreCommitTask<MyTool>()`` instead of ``registerTool<MyTool>()``.
The application creates one instance and registers both interfaces.
Task registration order is execution order; duplicate identities are rejected.
The pre-commit command adds concrete Git-capture and Git-verification tasks around the registered sequence.

Use the existing ``Tool::log()`` stream for dual-interface commands.
Task-only implementations use ``taskContext().log``.
The framework assigns these services before preparation; constructors and metadata methods must not use them.
Throw Core runtime errors for failed checks, and log actionable findings at error severity.
Warnings remain visible without failing the task.

Pass ``taskContext().progress`` into helpers that process files.
An optional ``TaskProgress`` value parameter preserves existing helper callers: a default handle discards updates
without locking, while an active handle shares synchronized counters with the renderer.
Call ``beginPhase(description, total)`` before starting workers and ``advance()`` after an item completes.
Omit the total during discovery, and finish all workers before switching phases.
Progress updates never write terminal output or log one entry per file.
Standalone commands receive an inactive handle and retain their usual log output.

For commands that aggregate failures after reporting them, override ``Tool::exitCode()``.
The framework queries it after normal completion of ``execute()``; its default is success.
See :doc:`../tool_reference/pre_commit_reference` for execution and logging contracts.
