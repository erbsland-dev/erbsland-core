
****************************
Guidelines for Writing Demos
****************************

Purpose of Demos
================

Demos in this library serve multiple purposes. They...

1. demonstrate the **user view** of the library.
2. provide **verified and compiled** example code that can be used in the documentation.
3. show the **intended** usage of the API.
4. allow evaluating the impact of API changes in the library.
5. are embedded in the documentation, and become part of the text.

Demo Executable Behavior
========================

Regular demos shall make use of the common demo framework in `demos/_common`,
that provides a special `DemoApplication` class.
This framework allows to create small demo snippets, each in a separate function that can be registered
in the application.
When run, the demo functions are executed and produce output in the terminal.

The utility ``create_demo`` automatically creates new demos, or add new parts to existing demos.

If this framework is not suitable for the demo:
- A demo must be safe to be started from the command line.
- The command line option `--help` or `-h` shall display command line help.

Demo Code in Documentation Pages
================================

Once a demo is created, in can be inserted using the ``..erbsland-demo::`` and ``..erbsland-demo-end::``
directives:

.. code-block:: rst

    .. erbsland-demo::
        :source: text/StringFormat/IntegerFormats.cpp
        :exec: string_format --demo IntegerFormats

    (demo code will be automatically synchronized here)

    .. erbsland-demo-end::

The utility ``demo_doc`` synchronizes source code and the output from the demo in this block.

Code is inserted starting from the first ``///`` line in the demo code.
Place unrelated code, like declarations and helper functions before the first ``///`` line.

The parameter ``:exec:`` is optional. When specified, it executes the demo and inserts a console output block
below the source code.

After synchronization, a hash is added to the block.
If this hash matches the referenced source file, no changes are made to the block.
To force resynchronization, delete the line with the hash and run ``demo_doc`` again.

Code Style, API Usage and Theming
=================================

See :doc:`examples` for details how to format and write example code.
Also, what kind of example shall be chosen for the library.
