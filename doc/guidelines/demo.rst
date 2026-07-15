
****************************
Guidelines for Writing Demos
****************************

Purpose of Demos
================

Demos in this library serve multiple purposes.
They...

1. demonstrate the **user view** of the library.
2. provide **verified and compiled** example code that can be used in the documentation.
3. show the **intended** usage of the API.
4. allow evaluating the impact of API changes in the library.
5. are embedded in the documentation, and become part of the text.

Demo Executable Behavior
========================

Regular demos shall make use of the common demo framework in `demos/_common`, that provides a special
``DemoApplication`` class.
This framework allows to create small demo snippets, each in a separate function that can be registered in the
application.
When run, the demo functions are executed and produce output in the terminal.

The utility ``create_demo`` automatically creates new demos, or add new parts to existing demos.

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

The parameter ``:exec:`` is optional.
When specified, it executes the demo and inserts a console output block below the source code.
Reference the executable as ``<domain>/<target>``; for example, ``text/string_format``.
Use ``:exec-2:``, ``:exec-3:``, and further numbered entries to capture additional invocations of the same demo block.
When multiple executions are defined, the generated output includes a rubric with the command line before each output
block.
For a single execution, add ``:show-cmd-line:`` when the generated output should also show the command line.
If a command is expected to fail, add ``:exec-exit-code:`` or ``:exec-N-exit-code:`` with the expected process exit
code.

.. code-block:: rst

    .. erbsland-demo::
        :source: option/ApplicationRegistration/main.cpp
        :exec: option/option_application --help
        :exec-2: option/option_application --instrument Prisma-7 --gain 4 cristal-azul
        :exec-3: option/option_application --gain 4 cristal-azul
        :exec-3-exit-code: 1

    (demo code and all output blocks will be automatically synchronized here)

    .. erbsland-demo-end::

After synchronization, a hash is added to the block.
If this hash matches the referenced source file, no changes are made to the block.
To force resynchronization, delete the line with the hash and run ``demo_doc`` again, or run ``demo_doc`` with
``--force``.
To refresh every demo block in a documentation directory and its subdirectories, run
``.venv/bin/python3 utilities/run.py demo_doc refresh doc/topics``.

Code Style, API Usage and Theming
=================================

See :doc:`examples` for details how to format and write example code.
Also, what kind of example shall be chosen for the library.
