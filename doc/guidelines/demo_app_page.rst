..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Guidelines; Demo Application Page

********************************
Demo Application Page Guidelines
********************************

``Demo Applications Explained`` connects complete working applications to the concepts taught in topic pages.
A walkthrough helps a reader understand how the application's important parts work together, rather than documenting
every method in source order.

Scope and Layout
================

Place pages under ``doc/demo_apps/<domain>/<application>.rst`` with a domain index linked from
``doc/demo_apps/index.rst``.
Include realistic applications from the ``apps`` category only.
Runnable snippet collections, compile-only templates, and demo framework documentation belong in their existing topic or
development documentation.

Give each application its own page.
Open with a short explanation of why the demo exists, which concepts it demonstrates, and what the reader will learn.
The title identifies the application and may add its teaching focus.
Link to concept topics when readers need their general explanation; do not repeat a topic tutorial inside the
walkthrough.

Follow the Application's Logic
==============================

Choose a sequence that makes this particular application easy to understand.
There is no required set of section headings: a small monitor, a service part, and a multi-stage workflow need different
explanations.
Start with enough context to locate the work, then follow the important state and control transfers.

Make execution contexts visible, including lifecycle hooks before dispatch and callbacks inside a loop.
Distinguish an event thread from an ordinary ``std::thread`` and from an application part's managed loop.
Explain who owns retained resources, where mutable state is accessed, how values cross contexts, and when the owner may
read results or release workers.

Follow the successful operation through completion and shutdown, and cover the failure paths that affect that protocol.
Explain which callback is required for progress, which notification is optional, and what makes a wait or join finish.
Do not imply that a successful example run establishes a stronger lifetime or delivery guarantee than the API provides.

Focus on the concepts the application was built to teach.
For example, a calculation demo may explain batching and cancellation without teaching its mathematical algorithm.
An archive demo may explain stage transitions without reproducing ZIP encoding details.
Skip unrelated code instead of giving every method equal commentary.

Source Excerpts and Runs
========================

Use focused, synchronized ``erbsland-demo`` blocks following :doc:`demo_integration`.
Select enough declarations and code to understand ownership and execution, then explain why the important transitions
occur.
Prefer several purposeful excerpts to one complete listing.
Readers can expand a source file when they need surrounding code.

Use ``erbsland-demo`` directly, **without** ``erbsland-in-practice-begin/end`` wrappers.
The entire page is already a practical application explanation.
On topic pages, the practical boundary still applies; a short focused discussion can link here for the complete
application design.
See :doc:`topic_page`.

Include useful bounded run examples that demonstrate the described behavior.
Use disposable fixtures and output directories for generated data, finite work limits or timeouts, and expected exit
codes for failure examples.
Identify time-dependent or platform-dependent output as an example run.
Keep template execution prohibited as defined in :doc:`demo`; templates are not application walkthroughs.

Validation and Review
=====================

Check the explanation against the current source, build the application, run relevant existing tests, and validate the
bounded commands.
Synchronize excerpts, process the RST, build the documentation, and inspect the rendered page.
Check navigation and links in both directions between the application and the topics using its code.

Read the narrative without expanding any full-source panels.
The reader should still be able to explain where work starts, where callbacks run, who owns the important state, how
results arrive, and how success or failure reaches cleanup.
When a page cannot answer those questions, improve its selected code and teaching order before adding unrelated detail.
