..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

**************************
Run and Configure Commands
**************************

This guide describes invocation, project selection, configuration files, and logging shared by all maintenance commands.
For command-specific workflows, choose a guide from :doc:`index`.
Follow :doc:`repository setup <../setup>` if the executable is not installed yet.

.. _development-tool-usage-find-and-run-a-command:

Find and Run a Command
======================

List commands and inspect the installed version:

.. code-block:: console

    ./erbsland-core-tool --help
    ./erbsland-core-tool --version

Put the command name before its options.
In the examples below, replace ``COMMAND`` with a name from the command overview:

.. code-block:: console

    ./erbsland-core-tool COMMAND --help
    ./erbsland-core-tool COMMAND

Help, version information, and the command overview work without a project or configuration file.
A command's help describes its own options; options such as ``--check`` are not available on every command.

The examples use the macOS/Linux root shortcut.
On Windows, replace ``./erbsland-core-tool`` with ``tool\bin\erbsland-core-tool.exe``.
The shortcut runs the installed Release copy; see :doc:`../tool_dev/build_and_test` to run a version under development.

.. _development-tool-usage-select-the-repository:

Select the Repository
=====================

By default, the tool searches upward from your working directory for the Erbsland Core project root.
You can invoke it from a repository subdirectory and still use project-relative input and output paths.
To select a checkout explicitly, use ``--project``:

.. code-block:: console

    ./erbsland-core-tool COMMAND --project /path/to/erbsland-core

The selected directory must itself be the project root.
An explicit selection does not search its parent directories.

.. _development-tool-usage-configure-a-command:

Configure a Command
===================

The default configuration is ``tool/conf/erbsland-core-tool.elcl`` in the selected project.
It includes shared settings from ``tool/conf/misc``, source settings from ``tool/conf/source``, and command settings
from ``tool/conf/tools``.
Each command's usage guide identifies the files you normally edit and links to its complete configuration reference.
For generated copyright and license notices, see :doc:`rendering_configuration`.

Use ``--config`` to select another configuration file inside the project:

.. code-block:: console

    ./erbsland-core-tool COMMAND --config tool/conf/erbsland-core-tool.elcl

.. list-table::
    :header-rows: 1
    :widths: auto

    * - Path
      - Relative to
    * - ``--project`` and ``--config``
      - Your current working directory
    * - Command input and output paths
      - The selected project root
    * - Configuration includes
      - The configuration file containing the include
    * - ``--log-file``
      - Your current working directory

Configuration and command data must remain inside the selected project.
Ordinary input and output paths cannot traverse symlinks or Windows reparse points; output directories must exist.
Log files are independent of project data and may be outside the project.
See :doc:`../tool_reference/framework_reference` for exact access checks and include handling.

The complete configuration must parse, but only shared settings, the selected command, and its required sources are
validated for execution.

.. _development-tool-usage-see-more-detail:

See More Detail
===============

The console normally shows information, warnings, and errors.
Use ``--verbose`` (``-v``) to include trace messages, or ``--log-file`` to append all log levels to a file:

.. code-block:: console

    ./erbsland-core-tool COMMAND --verbose
    ./erbsland-core-tool COMMAND --log-file tool-run.log

File log entries include a timestamp.
Trace messages reach the log file even without ``--verbose``.
Successful execution returns exit status zero; failures produce a diagnostic and a nonzero status.
The command's usage guide explains failures that require a particular follow-up, such as regenerating stale output.
