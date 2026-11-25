..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

*******************
Write a New Command
*******************

This guide takes a command from its class definition through configuration, execution, registration, and tests.
Start with :doc:`build_and_test` and the :doc:`framework` overview.
Try an existing command using :doc:`../tool_usage/usage` before adding your own.

Use ``tool/lib/core/src/tool/idna/IdnaTool.hpp`` and ``tool/lib/core/src/tool/idna/IdnaTool.cpp`` as a complete example
alongside these steps.

.. _development-tool-framework-1-define-the-command:

1. Define the Command
=====================

Derive your command class from ``tool::frame::Tool`` (``tool/lib/frame/src/tool/frame/Tool.hpp``).
Four methods are required:

.. list-table::
    :header-rows: 1
    :widths: auto

    * - Method
      - Purpose
    * - ``identifier()``
      - The configuration section name; its hyphen spelling supplies the command name
    * - ``title()``
      - A short title for the command overview
    * - ``description()``
      - The description displayed in command help
    * - ``execute()``
      - The command's work

Choose an identifier made of lowercase ASCII letter groups separated by single underscores, such as
``generate_idna_data``.
The example identifier becomes ``generate-idna-data`` on the command line.
Digits, leading or trailing underscores, and consecutive underscores are not accepted.
Titles must be nonempty and at most 60 characters; descriptions must also be nonempty.

The base class already supplies an option module, no configuration rules, no data source dependencies, and no-op
``configure()`` and ``initialize()`` methods.
A command that needs no options or configuration can keep these defaults.
If it needs shared inputs, override ``usedDataSources()`` to list their identifiers.
See :doc:`data_sources` and the dependency example in :doc:`write_data_source`.

.. _development-tool-framework-2-add-options-and-configuration:

2. Add Options and Configuration
================================

Override ``optionModule()`` when the command needs options.
Call ``Tool::optionModule()`` and add options to the returned module, as ``IdnaTool::optionModule()`` does.
This retains the command name and help metadata.
``Tool`` owns command behavior; the separate :cpp:class:`OptionModule <erbsland::options::OptionModule>` describes how
to parse its options.

If your command needs configuration, override ``configurationRules()`` to return validation rules relative to its
section.
Add that section to a file in ``tool/conf/tools``; the main configuration already includes files there.
The section name must equal the command identifier.

The framework validates the section with rule version 1 and applies defaults before calling
``configure(configuration, values)``.
Use that method to read the validated settings and parsed options, apply any command-specific overrides, and store the
settings needed for execution.
For example, IDNA generation reads its configured output path and then replaces it with an output option when supplied.
This precedence is implemented by the command, not automatically by the framework.

A command with configuration rules requires a regular section even when all its values have defaults.
A command without rules must have no matching section and receives ``nullptr`` as its configuration argument.

.. _development-tool-framework-3-perform-the-work:

3. Perform the Work
===================

The framework selects the project and validates all required configuration sections before calling command
``configure()``.
It then configures every required source, initializes sources in dependency order, and calls command ``initialize()``
and ``execute()``.
Keep inexpensive option and path checks in ``configure()``; access source data only after source initialization.
A command overriding ``requiresProject()`` to return ``false`` runs independently: project discovery, shared
configuration, rendering settings and data sources are skipped.
It receives a null configuration and is responsible for its own validated configuration and filesystem inputs.
It must not access ``app().files()``.

All three methods can use ``tool::frame::app()`` to access application services and the protected ``log()`` stream to
report progress.
Do not use these services from constructors or metadata methods; the project and logging are not ready there.

Read project inputs through ``frame::app().files()``.
Its ``checked()``, ``readText()``, and ``readData()`` methods keep paths inside the project.
Check output paths during ``configure()`` before preparing or writing generated output.
See :ref:`project file access <development-tool-framework_reference-project-file-access>` for details.

Returning normally from ``execute()`` reports success.
Throw :cpp:class:`ApplicationError <erbsland::core::ApplicationError>` for a command failure or to choose a specific
exit code.
For configuration or option errors, use :cpp:class:`ConfError <erbsland::conf::ConfError>` or
:cpp:class:`OptionError <erbsland::options::OptionError>` so diagnostics can identify the setting that caused the
problem.

Use ``log()`` for command output.
Information messages describe progress and results; trace messages can show detailed settings or intermediate steps.
The application's
:ref:`logging options <development-tool-usage-see-more-detail>` control where these messages appear.

.. _development-tool-reference-diagnostics:

Diagnostics
-----------

Give every error a title describing what failed and a description explaining why.

If you have many similar errors, or repeating error titles, keep failure sites short by moving construction into a
private ``throw...()`` method marked ``[[noreturn]]``.

Use :cpp:class:`LogicFailure <erbsland::err::LogicFailure>` for mistakes by the tool developer, such as invalid
registration or accessing services before initialization.
Use the logic error :cpp:class:`ParameterFailure <erbsland::err::ParameterFailure>` for invalid parameter values that
indicate mistakes by the tool developer.
Its reason text should explain both what failed and why.

For generic runtime errors, build an :cpp:class:`ApplicationErrorContext <erbsland::core::ApplicationErrorContext>` and
throw
:cpp:class:`ApplicationError <erbsland::core::ApplicationError>` from the helper.
Attach the source filename with ``setSourcePath()`` when a file caused the failure, and add a
:cpp:class:`CodeLocation <erbsland::unit::CodeLocation>` when its line is known.
Retain original line indices in parsed records so later validation can still identify the source line.

When wrapping another exception, pass ``std::current_exception()`` as the cause; source information already supplied by
the cause does not need to be repeated.
See ``UcdReader::throwError()`` for an example.

.. _development-tool-framework-4-render-and-save-generated-files:

4. Render and Save Generated Files
==================================

For a generator used only by the maintenance tool, keep templates in ``tool/lib/<part>/resources``.
The build compiles ``.cpp`` templates from this directory into ``erbsland-core-tool-<part>``, including templates in
subdirectories.
When another executable shares the generator, keep one template source beside that generator and embed it into both
targets, as the display-text compiler does with its ``data`` directory.

Create a renderer with ``frame::app().createRenderEnvironment(frame::TemplateLanguage::Cpp)``, populate an
:cpp:class:`Context <erbsland::text::render::Context>`, and render the template by its resource path.
The existing ``IdnaTool::render()`` method shows the complete sequence.
See :ref:`development-tool-reference-template-rendering` for delimiters, resource loading, and header globals.

Render and validate the complete result before calling ``ProjectFiles::updateText()``.
It leaves files with equivalent line endings and first-line copyright years untouched and replaces changed files through
a temporary file in the destination directory.
The destination directory must already exist.

For a check mode, pass ``true`` as the ``checkOnly`` argument of ``updateText()``.
The method returns whether the output differs and does not write in this mode.
Your command must add the check option and turn a difference into a failure if that is the desired behavior.
``IdnaTool::execute()`` demonstrates this; check mode is not automatic.

.. _development-tool-framework-5-connect-the-command-to-the-application:

5. Connect the Command to the Application
=========================================

Add the command's sources to ``erbsland-core-tool-<part>`` through a ``CMakeLists.txt`` in its source directory,
following the existing IDNA target sources (``tool/lib/core/src/tool/idna/CMakeLists.txt``).
Add the directory in ``tool/lib/core/src/tool/CMakeLists.txt``.

Include the command header in ``tool/app/src/main.cpp`` and register the class before ``app.run()``.
The existing registration is ``app.registerTool<tool::idna::IdnaTool>()``; use your own command class in the same way.
Registration rejects duplicate identifiers, collisions with sources or reserved sections, and invalid metadata.

Add a command guide to ``doc/development/tool_usage`` and link it from the :doc:`usage index <../tool_usage/index>`.
Describe its purpose, normal workflow, output, and common failures so users can run it without reading the
implementation.
Put exhaustive options, configuration contracts, formats, and algorithms in a matching ``tool_reference`` page.
Keep command-specific content out of the generic ``usage.rst``.
Follow :doc:`/guidelines/development_tool_pages` for the page boundaries and review checklist.

.. _development-tool-framework-6-verify-the-command:

6. Verify the Command
=====================

Add tests under ``tool/test/unittest/src`` and include them in that directory's CMake sources.
Test the command's own behavior, such as malformed inputs, option overrides, deterministic output, and preserving
existing output on failure.
Existing framework tests cover shared startup and path handling.

Helpers that call ``frame::app()`` need an active tool application in tests as well.
``ProjectFixture`` (``tool/test/unittest/src/ProjectFixture.hpp``) and the existing IDNA tests show how to provide that
lifecycle.

Follow :ref:`Build the version you are developing <development-tool-setup-build-the-version-you-are-developing>` and
:ref:`Run the tool tests <development-tool-setup-run-the-tool-tests>` to build and verify your changes.
Use the Debug executable to try the command's help and a representative invocation.
For changes to shared startup or configuration behavior, consult the
:doc:`framework reference <../tool_reference/framework_reference>` and the Python process tests too.
