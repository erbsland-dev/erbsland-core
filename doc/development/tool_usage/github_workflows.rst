..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

*****************************
Audit GitHub Workflow Actions
*****************************

``github-workflows`` checks external action SHA pins and the adjacent stable-version comments in the repository's
workflow files.
The default audit does not write workflows.
It requires trusted Git and access to remote GitHub tags.
See :doc:`usage` for shared invocation options.

.. code-block:: console

    ./erbsland-core-tool github-workflows
    ./erbsland-core-tool github-workflows --fix
    ./erbsland-core-tool github-workflows --fix --update actions/checkout@latest

Use ``--fix`` to repair version comments while retaining pins.
Use repeatable ``--update ACTION@VERSION`` to select ``latest`` or an exact stable tag.
An explicit update can pin an action that currently uses a branch or version tag.
It repairs the selected action's comment as well.
Review the resulting workflow diff before committing.

The audit fails when an action is unpinned, a version comment is missing or incorrect, or remote verification cannot
complete.
A newer stable version is informational.
Unresolved failures prevent all planned workflow edits.
Local actions and Docker references are excluded from this check.
Trusted ``zizmor``, when available, also runs an offline audit; its findings are advisory.

Settings live in ``tool/conf/tools/github_workflows.elcl``.
If executable discovery fails, install the command and configure its physical executable directory under
``Safe Binary Directories``.
On Windows, configure the installed Git directory explicitly.
See :doc:`../tool_reference/github_workflows_reference` for exact parsing and error rules.
