..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Guidelines; Demo Integration

****************************
Integrating Demos into Pages
****************************

A page can show a short excerpt from a compiled demo and the output of running it.
The ``doc-process-rst`` utility copies selected source and captured output into the RST file; Sphinx presents that saved
content with a source header, expandable full file, and separate panels for each command and its output.
Sphinx never runs demo programs or rewrites the RST.

Start with the Reader's Task
============================

Choose an example that answers the section's question, then build its executable.
Select enough code to explain execution and ownership, including stored state when it matters.
Avoid hiding essential declarations merely to shorten the excerpt.
A handwritten fragment of roughly one to five lines is reasonable for a small variation; larger examples should come
from compiled source.
See :doc:`demo` for creating demos and :doc:`topic_page` for choosing a teaching sequence.

Write the surrounding explanation knowing that readers can expand the demo block to inspect its complete source file.
Do not add a duplicate full listing, download link, or repository pointer merely to provide the rest of that same file.
There is usually no need to list omitted includes, namespace wrappers, or other scaffolding in the prose.
Keep the explanation focused on what the selected code teaches.

Expansion is optional context: declarations needed to understand execution or ownership still belong in the visible
excerpt.
A reference to a different source file can be useful when that file contains relevant supporting code, because expanding
a demo block shows only its referenced file, not its includes or other files in the demo.

Assume readers have access to the repository and its demos.
Do not append generic invitations to explore the source or reminders that demo blocks can be expanded.
Add source links or run instructions only when they serve a specific teaching purpose, such as locating a supporting
implementation discussed in the text or demonstrating behavior the reader needs to observe.

Add a managed block, synchronize it, and inspect the rendered result.
On topic pages, place this block inside a practical discussion as described below:

.. code-block:: rst

    .. erbsland-demo::
        :source: network/HostLookupDomain/main.cpp
        :show: c:Keep the lookup(+0,+1), f:resolveDocumentationHost(+0,+7)
        :exec: network/host_lookup_domain
        :show-cmd-line:

    .. erbsland-demo-end::

``:source:`` is relative to ``demos/``.
The generated source and output appear between these two directives.
Do not edit that generated content: update the demo, the selectors, or the execution options instead.
This demo has both a namespaced and a global ``main``; ``f:main`` would therefore be ambiguous.
The example selects the setup function and seven following lines to include the application entry point.
Review offsets when the surrounding code changes.

Source-Only Templates
=====================

Sources below ``demos/<domain>/templates/`` are compile-only application starting points.
Compile and link each template before extracting its source, but never execute it for documentation or validation.
Use ``:source:``, optional ``:show:`` and ``:hide:``, and the generated ``:sha256:`` metadata only.
For example:

.. code-block:: rst

    .. erbsland-demo::
        :source: event/templates/ApplicationStartup/DirectInitialization/main.cpp
        :show: f:main

    .. erbsland-demo-end::

Execution options (``exec``, numbered ``exec-N``, their exit-code options, ``files``, and ``show-cmd-line`` are
forbidden on template-source blocks.
The synchronizer rejects them before execution or cached-block reuse, even if a stored fingerprint matches.
Sphinx reports the same misuse when validating saved documentation.
Template executables live outside ``demo-apps`` so the normal demo runner cannot select them.

Place the source excerpts in a labeled practical block and identify both the business code the reader must add and the
lifecycle supplied by the framework.
A template's missing business operation may leave its loop running indefinitely; do not add a capture command to make
its documentation look like an executed example.
Apps and snippets remain runnable examples, with source paths under ``apps`` and ``snippets`` in categorized domains.
Legacy paths in other domains remain supported.

Separate Practical Discussion
=============================

Use ``erbsland-in-practice-begin`` and ``erbsland-in-practice-end`` to set apart a practical walkthrough from the
surrounding explanation of an API.
Topic pages must follow :ref:`topic-page-practical-separation`: every ``erbsland-demo`` block belongs inside a practical
block, together with the complete discussion of that example.
The whole discussion, including its demo panels, appears in a lightly tinted blue box with a small blue label.

Pages under ``doc/demo_apps`` are complete application walkthroughs and use ``erbsland-demo`` directly, without
practical wrappers.
Follow :doc:`demo_app_page` for their teaching structure.
On a topic page, link from a focused practical block to the full application walkthrough when the surrounding
architecture would distract from the concept being taught.

Open the practical block before introducing the concrete example, and close it after interpreting its code, output, and
design choices.
Use the same boundary for a walkthrough written with ordinary code blocks or prose alone.
Explain general API requirements in the regular topic text; repeat them beside the example when needed for safe use.
A section without a concrete walkthrough or demo discussion does not need a practical block.

.. code-block:: rst

    .. erbsland-in-practice-begin:: How the File-Size Monitor Initializes

    The monitor stores its timer so repeated callbacks remain active.

    .. erbsland-demo::
        :source: event/apps/FileSizeMonitor/main.cpp
        :show: f:initialize, f:poll

    .. erbsland-demo-end::

    The stored timer survives after initialization returns.

    .. erbsland-in-practice-end::

The title after ``begin::`` is optional; omitting it uses "In Practice".
Use the title to briefly summarize the practical discussion in a few words, allowing the reader to skip the block
entirely while skimming the page.
Keep the discussion at the same indentation as the surrounding text, with blank lines around each directive.
Both markers must belong to the same section and parent container; close the discussion before starting a new section.
Use ``rubric`` for a heading inside the box.
Paragraphs, lists, code blocks, and complete demo blocks can appear between the markers.
Missing or unmatched markers produce a build warning.
Non-HTML output retains the label and discussion text.

Choose Source Lines
===================

With neither selector option, extraction starts at the first ``///`` comment and ends before the closing demo namespace
and global entry-point wrapper.
``:show:`` replaces that default with a comma-separated list of selections in the order you want to explain them.
``:hide:`` removes selected original source lines after ``show``; without ``show`` it operates on the default excerpt.
Neither option requires special markers in the source.

Each selector has this form:

.. code-block:: text

    type:id[-type:id][(+front,+back)]

The available types are:

.. list-table::
    :header-rows: 1

    * - Selector
      - Selected lines
    * - ``f:name``
      - A complete named ``auto`` or ``void`` function definition, excluding preceding comments.
    * - ``a:name``
      - A simple assignment or initialized declaration ending in a semicolon, possibly across several lines.
        The name immediately to the left of ``=`` must match.
    * - ``c:text``
      - One line whose ``//`` or ``///`` comment contains the case-sensitive text.

Selectors match the original file, including code outside the default excerpt.
Missing or ambiguous matches produce diagnostics; choose a more specific comment or use a range and offsets.
Matching is best effort, not a C++ parser.
Complex assignments such as lambdas, overloaded functions, and unusual syntax may need comment selections instead.
Authors must inspect the rendered result.

A range includes both endpoint selections and everything between them, for example ``c:Prepare-f:run``.
The end must not precede the start.
Offsets expand with ``+`` and trim with ``-``: ``(+2)`` expands both ends by two lines, ``(-1)`` trims both ends by one,
and ``(+2,-1)`` adds two lines at the front and removes one at the back.
Offsets outside the file or those that empty the selection are errors.

Quote identifiers containing punctuation belonging to the selector grammar:

.. code-block:: rst

    :show: c:"Create, configure, and start"(+0,+2), f:run
    :hide: c:"Diagnostic output"

Single and double quotes are supported, with backslash escapes inside them.
Commas inside quotes or offset parentheses do not separate selectors.
Overlapping selections emit a source line only at its first occurrence.
Fragments are dedented before hidden lines are removed; internal spacing is retained and exactly one empty line
separates fragments, including gaps left by hiding lines.

Configured ``Demo Text Replacement`` rules run after source selection to simplify the displayed code.
They do not modify demo sources or captured output; the repository rules present ``DemoTerminal`` helper calls as the
public ``enableTerminal()`` API.
The rules are excluded from demo fingerprints.
After changing them, manually review affected excerpts and force synchronization of the selected pages.

Capture Commands and Output
===========================

``:exec:`` runs one built demo from the configured ``Demo Executable Root``, which defaults to
``cmake-build-debug/demo-apps/`` on every platform.
Use ``:exec-2:``, ``:exec-3:``, up to ``:exec-99:`` for further invocations, displayed in numeric order.
Each invocation receives its own output panel.
Multiple invocations always show their command; ``:show-cmd-line:`` also displays it for a single invocation.
Otherwise the panel header says "Output".
Successful commands that print nothing show "No output."

.. code-block:: rst

    :exec: text/string_format --demo IntegerFormats
    :exec-2: text/string_format --demo IntegerFormats --help
    :show-cmd-line:

Commands are argument lists, not shell programs: pipes, redirection, command substitution, and arbitrary shell syntax
are not supported.
Expected failures can be captured with ``:exec-exit-code:`` or ``:exec-N-exit-code:``; the default is zero.
Unexpected exit status or a timeout is a synchronization error.
Capture requires a built executable and runs through ``Subprocess`` on macOS, Linux, and Windows.
Shared demo support selects a virtual ANSI terminal when ``ERBSLAND_VIRTUAL_TTY`` is present; no pseudo-terminal is
needed.
Each run is limited to ten seconds, 1 MiB of output, and 100 output lines; source and explicit input files are limited
to 1 MiB.
Captured terminal output is normalized to static ANSI before embedding it in ``erbsland-ansi`` blocks.
The visible ``␛`` placeholder represents escape characters in the generated RST.

Inputs and Temporary Files
--------------------------

``:files:`` lists document-relative input files, with shell-style quoting for paths containing spaces.
Refer to their unique basenames in commands.
Their contents participate in the block hash, and captured output displays basenames rather than temporary or absolute
paths.
Files must remain inside the project and obey the utility's file-access restrictions.
Existing read-only fixtures can also be passed directly using paths beginning with ``demos/``.

Whole-argument placeholders create disposable inputs:

* ``{file:text}``: a five-line sample text file.
* ``{file:empty}``: an empty file.
* ``{file:none}``: a path to a missing file.
* ``{directory:name}``: a temporary directory with the requested readable name.

Repeated file placeholders receive distinct filenames.
Repeated directory placeholders with the same name share one temporary directory during that invocation.
Use the displayed command and filenames when explaining the captured output.
Network or time-dependent results may vary; identify captured output as an example run instead of promising fixed
values.

Synchronize and Review
======================

.. code-block:: console

    ./erbsland-core-tool doc-process-rst --no-cleanup --demos sync doc/topics/event/using_event_apis.rst
    ./erbsland-core-tool doc-process-rst --no-cleanup --demos validate doc/topics/event/using_event_apis.rst
    ./erbsland-core-tool doc-process-rst --no-cleanup --demos refresh doc/topics/event

``sync`` updates changed blocks; ``validate`` reports needed updates without writing RST, but may execute demos when
inputs have changed.
``refresh`` recursively regenerates all blocks in the target regardless of their hashes.
Use ``--force`` with ``sync`` or ``validate`` for the same forced regeneration behavior, and ``-r`` or ``--recursive``
when processing a directory recursively in other modes.

``migrate`` converts legacy function selections to ``show`` and removes the old hash fields without running demos or
changing cached excerpts and output.
A migrated block receives its new hash on its next explicit synchronization.

The sole generated ``:sha256:`` covers all author options, the complete source, and referenced input files.
It excludes generated content, captured output, and executable bytes.
A matching hash leaves the entire block unchanged and does not run its commands.
Use refresh after rebuilding binaries or changing the environment when the hashed inputs have not changed.
Failed generation does not receive a valid hash.
Do not add hashes by hand or edit cached output expecting synchronization to detect that edit.

After synchronization, run ``doc-process-rst`` on the edited pages and build the documentation.
Inspect the code boundaries, ownership declarations, command/output association, and full-source expansion.
The full-source view is read directly during the Sphinx build and can be newer than a cached excerpt until you
synchronize it; source dependency tracking updates this view in incremental builds.
Automatic stale-example enforcement in builds, CI, and pre-commit is deferred until 1.0.

HTML readers can expand the complete source inline without losing their place.
Without JavaScript, the source link leads to the complete file at the bottom of the page.
Non-HTML builders retain the excerpt and readable command/output content.
Only explicitly referenced source files are expanded, not their transitive includes or command fixtures.
