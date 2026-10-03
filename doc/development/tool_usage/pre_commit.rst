...
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

*******************************
Check the Repository for Commit
*******************************

Run ``pre-commit`` before committing library changes.
It updates generated headers, normalizes sources, checks API rules, maintains reference pages, and updates security
hashes.
Git must be available on ``PATH``; code cleanup also needs the configured formatter.

.. code-block:: console

    ./erbsland-core-tool pre-commit

The command runs every task, even if an earlier check fails.
Interactive terminals show an updating progress row and permanent completion rows.
Redirected output uses plain start and completion lines.
Warnings remain visible without failing a task; error details appear below failed tasks.

Review Generated Changes
========================

The command may update repository files using the normal defaults of its individual maintenance commands.
It compares Git state before and after the run, including staged changes and untracked file contents.
An already-dirty checkout is accepted when its state remains unchanged.
If pre-commit changes files, review the diff and rerun the command before committing.
It never stages or commits changes.

Use ``--verbose`` to see routine output under each task, or ``--log-file`` to retain detailed task-labelled logs.
Choose an ignored log-file destination when running pre-commit; a changed tracked or untracked log file is itself a
repository change.
For shared invocation options, see :doc:`usage`.

When developing the tool itself, use the newly built Debug executable:

.. code-block:: console

    cmake-build-debug/tool/erbsland-core-tool pre-commit

See :doc:`../tool_reference/pre_commit_reference` for task order and failure semantics.
