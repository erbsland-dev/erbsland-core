
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

The utility ``create_demo`` automatically creates new demos, or add new parts to existing demos.

The generated executable entry point imports the ``demo`` namespace and defines only the required global ``main``
function.
A forwarding global ``main`` may be added when the namespaced implementation itself is embedded in the documentation and
the wrapper must be omitted from the displayed source.

If this framework is not suitable for the demo:
- A demo must be safe to be started from the command line.
- The command line option `--help` or `-h` shall display command line help.

Demo Code in Documentation Pages
================================

Once a demo is created, in can be inserted using the ``..erbsland-demo::`` and ``..erbsland-demo-end::`` directives:

.. code-block:: rst

    .. erbsland-demo::
        :source: text/StringFormat/IntegerFormats.cpp
        :exec: text/string_format --demo IntegerFormats

    (demo code will be automatically synchronized here)

    .. erbsland-demo-end::

The utility ``demo_doc`` synchronizes source code and the output from the demo in this block.

Code is inserted starting from the first ``///`` line in the demo code.
Place unrelated code, like declarations and helper functions before the first ``///`` line.

To show only the parts that are relevant to one explanation, add ``:function-blocks:`` followed by one or more function
names.
The utility finds complete ``auto`` or ``void`` function definitions, preserves their requested order, joins them into
one code block, and removes their shared indentation.
This keeps a topic page focused while the shown code remains synchronized with the complete demo.
The utility also stores a hash for the selected names, so changing the selection regenerates the code block.

.. code-block:: rst

    .. erbsland-demo::
        :source: network/TimeClient/TimeClientApp.hpp
        :function-blocks: onResolved onBound onDatagram

    (the selected functions will be automatically synchronized here)

    .. erbsland-demo-end::

The parameter ``:exec:`` is optional.
When specified, it executes the demo and inserts a console output block below the source code.
Reference the executable as ``<domain>/<target>``; for example, ``text/string_format``.
Use ``:exec-2:``, ``:exec-3:``, and further numbered entries to capture additional invocations of the same demo block.
When multiple executions are defined, the generated output includes a rubric with the command line before each output
block.
For a single execution, add ``:show-cmd-line:`` when the generated output should also show the command line.
If a command is expected to fail, add ``:exec-exit-code:`` or ``:exec-N-exit-code:`` with the expected process exit
code.

Use ``:files:`` to make one or more files available to the demo command.
Each entry is a path relative to the reStructuredText file, while ``:exec:`` references it by basename only.
The utility resolves the basename to the actual path for execution and replaces that path with the basename in captured
output, preventing temporary, source, or build directories from leaking into the generated documentation.
All listed files must have unique basenames.
Their combined content hash is stored in ``:files-sha256:``, so changing an input file regenerates the demo output.

.. code-block:: rst

    .. erbsland-demo::
        :source: conf/ConfigurationDocuments/ManualValidation.cpp
        :files: ../topics/conf/examples/valid-patch.elcl ../topics/conf/examples/invalid-patch.elcl
        :exec: conf/configuration_documents --demo ManualValidation valid-patch.elcl
        :exec-2: conf/configuration_documents --demo ManualValidation invalid-patch.elcl
        :exec-2-exit-code: 1

    (demo code and all output blocks will be automatically synchronized here)

    .. erbsland-demo-end::

After synchronization, a hash is added to the block.
If this hash matches the referenced source file, no changes are made to the block.
To force resynchronization, delete the line with the hash and run ``demo_doc`` again, or run ``demo_doc`` with
``--force``.
To refresh every demo block in a documentation directory and its subdirectories, run
``.venv/bin/python3 utilities/run.py demo_doc refresh doc/topics``.

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
