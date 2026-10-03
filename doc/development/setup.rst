Set Up the Repository
=====================

This guide prepares a checkout for development on Erbsland Core.
The setup script installs the repository's maintenance tool as part of that process, so you can use it while working on
the library.
You do not need to set up tool development unless you plan to change the maintenance tool itself.

Before You Start
----------------

You need Python 3.14, Git, CMake 3.28 or later, Ninja, and a C++20 compiler available in your terminal.
Python must support virtual environments and pip.
Setup downloads Python dependencies and initializes Git submodules, so it also needs network access.
On Windows, use a terminal with your compiler environment initialized.

If you also want to build the documentation, install Doxygen.
The setup script installs the required Python documentation packages.
See :doc:`/addendum/requirements` for the library's general requirements.

Prepare the Checkout
--------------------

Open a terminal in the repository root and run:

.. code-block:: console

    python3 setup.py

On Windows, use ``python setup.py`` if that is the name of your Python interpreter.
The script performs these steps:

1. Creates ``.venv``, or reuses the existing environment, and installs the Python
   development requirements from ``requirements-dev.txt`` and documentation requirements from
   ``doc/requirements-doc.txt``.
2. Initializes the unit-test framework and language-configuration test submodules.
3. Configures a Ninja Debug build in ``cmake-build-debug`` with unity builds enabled.
4. Builds a separate Release version of the maintenance tool in
   ``cmake-build-erbsland-core-tool-release``.
5. Installs the executable in ``tool/bin``.

The Debug build is configured but not compiled by setup.
The installed Release tool is ready to use without activating ``.venv``.
On macOS and Linux, setup also creates the root shortcut ``./erbsland-core-tool``.
On Windows, run ``tool\bin\erbsland-core-tool.exe``.

Setup stops when a step fails.
Read the reported command and its error, resolve the problem, and run setup again.
The installer refuses to replace an unrelated file or symlink at the root shortcut path.
If that path is already in use, inspect it before moving it aside.
The installed binary and shortcut are ignored by Git.

Build and Test the Library
--------------------------

Build the library and the enabled development targets:

.. code-block:: console

    cmake --build cmake-build-debug

Then run the registered tests from the build directory:

.. code-block:: console

    ctest --test-dir cmake-build-debug --output-on-failure

The ordinary test run leaves out tests marked for a full run.
When you need the extended library tests, run the unit-test executable with the additional tag:

.. code-block:: console

    cmake-build-debug/test/unittest/erbsland-core-unittest +tag:FullRun

On Windows, add ``.exe`` to the executable name.
Use separate build directories on different operating systems; a CMake cache from one host cannot be reused on another.
For available build settings, see :doc:`/usage/build-configuration`.

Use the Maintenance Tool
------------------------

You can now inspect the installed tool's commands:

.. code-block:: console

    ./erbsland-core-tool --help

On Windows, use ``tool\bin\erbsland-core-tool.exe --help``.
Continue with
:doc:`tool_usage/usage` when you need to run a maintenance command. Re-running ``setup.py``
rebuilds and installs the Release tool, for example after updating your checkout.

If you want to change the tool's C++ implementation, continue with :doc:`tool_dev/build_and_test`.
That guide uses the same checkout and adds a build-and-test workflow for the tool.

Build the Documentation
-----------------------

From the repository root, build the documentation incrementally:

.. code-block:: console

    ./erbsland-core-tool doc-build

See :doc:`tool_usage/doc_build` for command options and warning-suppression configuration.
On Windows, use ``tool\bin\erbsland-core-tool.exe doc-build``.

The output is written to ``_build``.
The first build generates the API index and all pages.
Later builds reuse the Sphinx environment and Doxygen XML: editing a paragraph does not regenerate the API index.
Changes to source headers, the source preprocessor, or Doxygen settings refresh the API index automatically.
Use ``--no-filter`` to see all Sphinx output, including the number of documents reread and written.

Direct Sphinx builds use the same cache:

.. code-block:: console

    .venv/bin/sphinx-build doc _build

For a live preview, install ``sphinx-autobuild`` in the virtual environment and run:

.. code-block:: console

    .venv/bin/sphinx-autobuild doc _build

Keep the watched source directory at ``doc``; watching the repository root also watches generated output and can cause
rebuild loops.
Use ``--watch src`` as well if you want header edits to trigger a rebuild.

If incremental output becomes stale, or before publishing a release, discard all generated documentation caches:

.. code-block:: console

    ./erbsland-core-tool doc-build --full-rebuild

This recreates processed headers, Doxygen XML, Sphinx doctrees and HTML.
It can take several minutes.
After changing installed documentation tools, use a full rebuild to refresh output generated by the older versions.

After editing reStructuredText, normalize the affected files and resolve type links:

.. code-block:: console

    ./erbsland-core-tool doc-process-rst --recursive doc/development

Replace ``doc/development`` with the file or directory you changed.
If you introduced new API symbols, run ``doc-build`` first so the type index includes them.

Before Committing
-----------------

Run the repository checks from the root:

.. code-block:: console

    ./erbsland-core-tool pre-commit

On Windows, use ``.venv\Scripts\python.exe`` for Python commands and ``.venv\Scripts\sphinx-build.exe`` or
``.venv\Scripts\sphinx-autobuild.exe`` for direct Sphinx commands.
