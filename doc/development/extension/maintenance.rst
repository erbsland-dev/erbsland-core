..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Extensions; Development setup
    single: Extensions; Maintenance tool

*********************
Maintain an Extension
*********************

A public header has several readers.
The compiler needs to find it, the unit test needs to call the code behind it, and the developer using your extension
needs to know what the call means.
When you add a method, one local development setup can check all three.
Core supplies the maintenance commands, while your extension decides which commands to run and which files they own.

Choose What the Tool Maintains
==============================

Give your repository its own executable and configuration.
The framework looks for your CMake project name to locate the checkout, then reads your configuration relative to that
root.
The display name and executable name appear in help and generated files, so someone reading a wrapper can tell which
tool will regenerate it.

.. erbsland-in-practice-begin:: Register the commands for sample

Save the :download:`tool entry point <example/main.cpp>` as ``tool/app/src/main.cpp``.
Its ``main()`` constructs the application and registers the commands.

.. literalinclude:: example/main.cpp
    :language: cpp
    :start-at: auto app =
    :end-at: return app.run();
    :dedent: 4

``registerPreCommitTask`` makes a command available on its own and includes it in pre-commit, in registration order.
Header generation comes first because cleanup reads the mapping from source files to public includes.
``registerTool`` makes the documentation commands available when you want to build the site.

.. code-block:: cmake
    :caption: tool/CMakeLists.txt

    cmake_minimum_required(VERSION 3.28...3.29)

    add_executable(erbsland-sample-tool app/src/main.cpp)
    erbsland_core_setup_application(TARGET erbsland-sample-tool)
    target_link_libraries(erbsland-sample-tool PRIVATE
            erbsland-core-tool-frame
            erbsland-core-tool-headers
            erbsland-core-tool-cleanup
            erbsland-core-tool-pre_commit
            erbsland-core-tool-doc
    )

The standalone CMake file enables ``ERBSLAND_CORE_ENABLE_TOOL_LIBRARIES`` before adding Core.
That gives this executable the implementations and compiled templates it needs.
The application's ordinary submodule build only needs the library.

.. erbsland-in-practice-end::

Your extension shares the ``erbsland/`` include prefix with Core.
Its header generator should publish just its own domain.
Its cleanup command needs to recognize both a local include that it can normalize and a Core include that must keep its
public spelling.

.. erbsland-in-practice-begin:: Keep the public headers in their domain

Save the :download:`complete configuration <example/erbsland-sample-tool.elcl>` as
``tool/conf/erbsland-sample-tool.elcl`` and copy Core's ``.clang-format`` to the extension root.

* ``Flatten: No`` keeps its wrapper at ``<erbsland/sample/Reading.hpp>``.
* ``Generate Namespace Headers: No`` leaves Core in charge of the common namespace headers.
* ``Shared Include Prefix: Yes`` tells cleanup to preserve an external include such as ``<erbsland/String.hpp>``
  when the extension's public-header map has no entry for it.

The config selects the library, ``test/unittest/src``, and ``tool/app/src`` separately.
Apply the same ownership rule to any additional scans, including anti-pattern checks and security hashes.
Scanning all of ``test`` would also select someone else's code when your framework or Core is checked out there.

.. erbsland-in-practice-end::

See the :doc:`header generation </development/tool_reference/public_headers_reference>` and
:doc:`cleanup </development/tool_reference/code_cleanup_reference>` references for the available settings.
As the project grows, register :doc:`anti-pattern checks </development/tool_usage/anti_patterns>`,
:doc:`API checks </development/tool_reference/documentation_tools_reference>`, or
:doc:`security hashes </development/tool_usage/security_hashes>` in the same way.
Place security hashes after any commands that write files covered by the manifest.

Generate the Header and Run the Test
====================================

Let setup prepare a development checkout.
It prefers the sibling Core you are working on and reuses a local checkout under ``test/erbsland-core`` if you have one
there instead.
If neither exists, setup clones the latest tagged Core release into that local directory.
If your extension needs tooling that first appears in a development revision of Core, keep that revision in the sibling
checkout while you work on the extension.
Keeping downloads in setup gives a regular CMake configure a predictable input and lets you keep developing offline once
the dependencies are present.

.. erbsland-in-practice-begin:: Build and test the first public method

Save :download:`setup.py <example/setup.py>` in the repository root.
It imports ``prepare_development`` from the selected Core checkout's ``tool/python`` directory.
The helper initializes the test submodule, creates ``.venv``, configures the Debug tree, and installs a Release build of
``erbsland-sample-tool`` in ``tool/bin``.
Put the development checkout and local build products in the extension's ignore file so pre-commit can show you changes
to the files you intend to publish.

.. code-block:: text
    :caption: .gitignore

    /.venv/
    /cmake-build-*/
    /.cache/
    /tool/bin/
    /erbsland-sample-tool
    /test/erbsland-core/
    /_build/
    /_doxygen_input/

With Git, Python, CMake, Ninja, and a C++20 compiler installed, you can now build the tool and run the test.
Install clang-format for cleanup and Doxygen for the reference build.

.. code-block:: console

    $ python3 setup.py
    $ ./erbsland-sample-tool update-public-headers
    $ cmake --build cmake-build-debug --target erbsland-sample-unittest
    $ ctest --test-dir cmake-build-debug --output-on-failure

On Windows, invoke the installed tool as ``tool\bin\erbsland-sample-tool.exe``.
The generator creates the wrapper for ``Reading.hpp``, the domain umbrella ``<erbsland/all_sample.hpp>``, and the header
map used by cleanup.
A passing test means a caller can include the wrapper, link the out-of-line method, and read its result.

Run ``./erbsland-sample-tool pre-commit`` before committing a change.
If a task rewrites files, the command reports a changed Git state.
Review the changes and run it again.
Commit the generated headers and map with the change that produced them.

.. erbsland-in-practice-end::

Document What the Caller Can Rely On
====================================

Keep the contract for a public method beside its declaration.
The extension's Sphinx site can use the same preprocessing and Doxygen integration as Core while reading the extension's
own headers.
Breathe takes the resulting XML and renders the declaration and its comment on your reference page.

.. erbsland-in-practice-begin:: Publish the unit symbol

For example, change the comment on ``Reading::unitSymbol()`` to say that it returns the SI symbol for milliseconds.
That detail tells a caller whether the string can be used as a display label.
The sample setup records the selected Core checkout's Sphinx plugins in the virtual environment.
Use that path to load the integration, and give Breathe the XML directory for your own project.

.. code-block:: python
    :caption: doc/conf.py

    import sys
    from pathlib import Path

    root = Path(__file__).resolve().parent.parent
    sys.path.insert(0, (root / ".venv/erbsland-sphinx.path").read_text(encoding="utf-8").strip())
    from erbsland_doxygen import default_config_options

    project = "Erbsland Sample"
    extensions = ["breathe", "erbsland_breathe", "erbsland_badges", "erbsland_doxygen"]
    breathe_default_project = "erbsland-core-sample"
    breathe_projects = {breathe_default_project: root / "_build/breathe/doxygen" / breathe_default_project / "xml"}
    breathe_doxygen_config_options = default_config_options(root / "_doxygen_input")
    erbsland_doc_tool = "erbsland-sample-tool"

A single Sphinx directive on the site's home page can publish the whole class.

.. code-block:: rst
    :caption: doc/index.rst

    Sample Measurements
    ===================

    .. doxygenclass:: erbsland::sample::Reading
        :members:

Run ``./erbsland-sample-tool doc-build`` and open ``_build/index.html``.
You should see the method signature and its updated comment.
On a larger site, :doc:`doc-update-reference </development/tool_usage/doc_update_reference>` can maintain separate
reference pages.
For instructions on adding the published library to an application, link readers to :doc:`/usage/extensions`.

.. erbsland-in-practice-end::
