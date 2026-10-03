
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
5. are embedded in the documentation, and become part of the text.

Demo Executable Behavior
========================

Regular demos shall make use of the common demo framework in `demos/_common`, that provides a special
``DemoApplication`` class.
This framework allows to create small demo snippets, each in a separate function that can be registered in the
application.
When run, the demo functions are executed and produce output in the terminal.

The utility ``create-demo`` automatically creates new demos, or add new parts to existing demos.

The generated executable entry point imports the ``demo`` namespace and defines only the required global ``main``
function.
A forwarding global ``main`` may be added when the namespaced implementation itself is embedded in the documentation and
the wrapper must be omitted from the displayed source.

If this framework is not suitable for the demo:

* A demo must be safe to be started from the command line.
* The command line option ``--help`` or ``-h`` shall display command line help.

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

Use :doc:`demo_integration` to select source excerpts, capture commands and output, synchronize managed blocks, and
review their rendered presentation.
That guide documents all ``erbsland-demo`` options and the ``doc-process-rst`` workflow.

Language and Themes in Demos
============================

When to use a theme and language?

* Only use a theme for small functions like demos to make them more entertaining.
* Don't use a theme for large application like demos that resemble real-life applications.

How to use a theme and language?

* Non-english natural language must only be used in data, values or input an example/demo processes.
  It is a good method to separate user-input from application logic.
* Error messages, diagnostic output, and any output that explains the behavior of the example/demo must be English.
* Only if you choose a theme and language manually, see :doc:`example_themes` for a comprehensive list.

Code Style, API Usage and Theming
=================================

* See :doc:`examples` for details how to format and write example code.
* Only if you choose a theme and language manually, see :doc:`example_themes` for a comprehensive list.
