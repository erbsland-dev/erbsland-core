
****************************
Guidelines for Writing Demos
****************************

Purpose of Demos
================

Demos in this library serve multiple purposes.
They...

1. demonstrate the **user view** of the library.
2. explain the API using a practical example of **real-world code** that is useful for a user.
3. provide **verified and compiled** example code that can be used in the documentation.
4. show the **intended** usage of the API.
5. are embedded in the documentation to support its explanations.

Demo Categories and Layout
==========================

Organize demos by domain, then by their teaching purpose:

.. code-block:: text

    demos/<domain>/
        apps/<ApplicationName>/
        snippets/<CollectionName>/
        templates/<TemplateCollection>/<Variant>/

**Apps** are realistic working applications that demonstrate one or more topics.
They include the business functionality needed to observe their behavior.
Build and link them, run their relevant tests, and validate bounded documentation commands.
They must be safe to start from the command line and provide ``--help`` or ``-h``.
Use ``erbsland_core_add_demo`` to register each executable.
Explain complete applications under ``doc/demo_apps/<domain>`` following :doc:`demo_app_page`.
Topic pages may reuse focused excerpts inside practical blocks and link to the complete walkthrough.
Snippet collections, compile-only templates, and shared demo framework documentation do not belong in that section.

**Snippets** are runnable collections of focused examples using ``DemoApplication`` from ``demos/_common``.
Each example is a separate registered function.
Build and link the collection, then execute the relevant examples and synchronize their captured output.
The command ``create-demo <domain>/<DemoName>/<DemoPart>`` creates new collections under ``snippets``.
It extends an existing legacy collection at ``demos/<domain>/<DemoName>`` in place.
It rejects ambiguous legacy/categorized locations and names already used by ``apps`` or ``templates``.

**Templates** are compilable starting points with business functionality deliberately omitted.
They contain real entry points, lifecycle hooks, resource ownership, required configuration handling, and completion
hooks.
Comments identify where the reader supplies operations and connects their completion.
Do not add simulated work, artificial output, or unrelated facilities merely to make a template terminate.
A template may keep running until its missing business functionality is implemented.

Templates **must compile and link, but must never be executed for documentation or validation**, including with
``--help``.
Register each independent variant with ``erbsland_core_add_demo_template`` and include it in normal demo compilation.
This helper writes executables to ``demo-templates/<domain>`` in the build tree, outside the runnable
``demo-apps/<domain>`` tree used by apps and snippets.
Do not register templates as executable tests or include them in demo-running scripts.
Documentation uses source-only ``erbsland-demo`` blocks for templates.

The event domain uses this categorized layout.
Other domains retain their existing locations until explicitly migrated; do not reorganize them as a side effect of
adding a snippet.
Changing a source directory must preserve existing target names and runnable executable locations, and update all source
references and documentation selectors.

Demo Entry Points
=================

The generated snippet entry point imports the ``demo`` namespace and defines only the required global ``main`` function.
A forwarding global ``main`` may be added when the namespaced implementation itself is embedded in the documentation and
the wrapper must be omitted from the displayed source.
Apps and templates use the entry points appropriate to the application design they teach.

Terminal Capture
================

Use ``demo::DemoTerminal::enable(application)`` wherever a demo enables terminal output.
The shared ``DemoApplication`` and ``TerminalApplication`` creation hooks also apply these settings when logging enables
a terminal implicitly.
Preserve explicit choices between plain and styled output by enabling the terminal only in the styled branch.
The ``doc_process_rst.demo_text_replacement`` configuration presents these helper calls as ordinary ``enableTerminal()``
calls in generated documentation excerpts.

When ``ERBSLAND_VIRTUAL_TTY`` is present, shared demo support selects ``TerminalFlag::VirtualTty`` and a 90×40 screen.
``ERBSLAND_VIRTUAL_TTY_SIZE=WIDTHxHEIGHT`` overrides these dimensions with positive decimal values bounded to the
terminal's supported size.
Malformed dimensions are reported as application errors.
An explicitly supplied virtual-terminal flag and size take precedence over environment settings.
The virtual terminal does not read stdin; documentation commands must finish without user input.

Structured Demo Applications
============================

A demo application with at least three compilation units and/or at least three classes shall keep its production sources
in a ``src`` subdirectory.
This rule applies to application examples whose units implement cooperating components; it does not apply to collections
of snippets created by the demo generator.
Count the handwritten ``cpp`` compilation units, independently of unity-build settings and generated sources.
Count primary classes with a declaration in a ``hpp`` file.

These applications may have an independent unit-test executable in a ``test`` subdirectory.
Keep its suites separate from the library's main unit-test executable.
Shared application components can be built as a private support target linked by the application and its tests.
Tests must compile only when both ``ERBSLAND_CORE_ENABLE_DEMOS`` and ``ERBSLAND_CORE_ENABLE_TESTS`` are enabled.
Guard the test subdirectory with both options, and ensure the Erbsland Unit Test framework is available before
configuring its target.

.. code-block:: text

    DemoApplication/
        CMakeLists.txt
        src/
            CMakeLists.txt
            main.cpp
            Application.hpp
            Application.cpp
            Worker.hpp
            Worker.cpp
        test/
            CMakeLists.txt
            src/
                CMakeLists.txt
                main.cpp
                WorkerTest.cpp

Each source directory follows :doc:`cmake_files`.
Document component responsibilities, ownership, execution contexts, and shutdown dependencies so a reader can understand
a selected component without having to inspect the entire application first.

Demo Code in Documentation Pages
================================

For topic pages, follow :ref:`topic-page-practical-separation` to keep demos and their discussion inside practical
blocks.
Use :doc:`demo_integration` to select source excerpts, capture commands and output, synchronize managed blocks, and
review their rendered presentation.
That guide documents all ``erbsland-demo`` options and the ``doc-process-rst`` workflow.

Language and Themes in Demos
============================

When to use a theme and language?

* Only use a theme for small functions like demos to make them more entertaining.
* Don't use a theme for application demos or compile-only templates.
* Templates use neutral names and comments to identify the business functionality the reader must supply.

How to use a theme and language?

* Non-english natural language must only be used in data, values or input an example/demo processes.
  It is a good method to separate user-input from application logic.
* Error messages, diagnostic output, and any output that explains the behavior of the example/demo must be English.
* Only if you choose a theme and language manually, see :doc:`example_themes` for a comprehensive list.

Code Style, API Usage and Theming
=================================

* See :doc:`examples` for details how to format and write example code.
* Only if you choose a theme and language manually, see :doc:`example_themes` for a comprehensive list.
