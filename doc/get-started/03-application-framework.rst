..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Getting started; Application
    single: Application framework
    single: Command-line options

*************************
The Application Framework
*************************

Define the Application Class
============================

Derive from :cpp:class:`Application <erbsland::core::Application>` when options and startup behavior belong to the
executable as a whole.
The ``ElGrepApp`` class owns the search workflow and a pair of private result structures used to prepare output.

.. literalinclude:: files/elgrep/src/ElGrepApp.hpp
    :language: cpp
    :caption: <project>/elgrep/src/ElGrepApp.hpp
    :linenos:

The class overrides the three lifecycle methods needed by this tool:

``initialize()``
    Sets metadata used by generated help and error output.

``registerCommandLineOptions()``
    Describes one flag and two required positional arguments.

``main()``
    Runs only after Core has parsed and validated the command line.

Initialize Metadata and Options
===============================

The first part of ``ElGrepApp.cpp`` supplies the application metadata and option definitions.

.. literalinclude:: files/elgrep/src/ElGrepApp.cpp
    :language: cpp
    :caption: ElGrepApp.cpp — initialization and options
    :start-at: void ElGrepApp::initialize()
    :end-before: auto ElGrepApp::main()

Core automatically adds standard help and version handling.
The option lookup names without dashes, such as ``recursive``, are the stable names used when reading parsed values.

Let Application Handle Failures
===============================

The main method converts and validates the path, compiles the expression once, and chooses between a file search and a
directory walk.

.. literalinclude:: files/elgrep/src/ElGrepApp.cpp
    :language: cpp
    :caption: ElGrepApp.cpp — main workflow
    :start-at: auto ElGrepApp::main()
    :end-before: auto ElGrepApp::searchFile(

There is deliberately no ``try`` /``catch`` block here.
Invalid paths and patterns already throw exceptions from the Erbsland Core error hierarchy.
For application-specific validation, ``elgrep`` throws :cpp:class:`ApplicationError <erbsland::core::ApplicationError>`.
:cpp:func:`Application::run() <erbsland::core::Application::run>` catches these errors, renders a consistent diagnostic,
and returns the corresponding process exit code.

.. button-ref:: 04-searching-files
    :ref-type: doc
    :color: success
    :align: center
    :expand:
    :class: sd-fs-5 sd-font-weight-bold sd-p-2 sd-my-4

    Search Files and Directories →
