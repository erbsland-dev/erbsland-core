..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Application; Design
    single: Application; Main Function
    single: Application; Event Loop
    single: Application; Command Modules
    single: Application; Parts

******************************
Choosing an Application Design
******************************

Every Erbsland Core executable creates one :cpp:class:`Application <erbsland::core::Application>` and finishes by
calling :cpp:func:`run() <erbsland::core::Application::run>`.
The useful choice is not whether to use that lifecycle, but where the application's own behavior should enter it.

Start with the simplest design that describes the real control flow.
Moving to another design later does not replace the surrounding option parsing, error boundary, or shared application
services.

Comparing the Designs
=====================

.. list-table::
    :header-rows: 1
    :widths: 22 25 53

    * - Design
      - Work model
      - Best fit
    * - :doc:`Function based <function_based_applications>`
      - One small callback
      - Short, single-file utilities that need initialization and a process exit code.
    * - :doc:`Event driven <event_driven_applications>`
      - Main event loop
      - Servers, monitors, clients, and programs whose work completes through asynchronous callbacks.
    * - :doc:`Procedural <procedural_applications>`
      - One synchronous method
      - Converters, generators, and batch tools with a clear sequence of operations.
    * - :doc:`Command style <command_style_applications>`
      - Selected module callback
      - Tools with actions such as ``list``, ``add``, ``remove``, or ``export``.
    * - :doc:`Application parts <applications_from_parts>`
      - Dependency-managed event loops
      - Larger applications composed from independently started services.

Understanding Main Dispatch
===========================

After successful command-line parsing, ``Application::run()`` calls the virtual ``main()`` method.
The default implementation first calls the main function of the selected option module.
If no module owns the command, it calls the function registered through ``setMainFn()``.
If neither exists, it starts registered application parts and enters the main event loop.

This order makes the default implementation useful for four of the five designs.
An overridden ``main()`` expresses a procedural application, but it also replaces that dispatch.
Such an override must explicitly call ``Application::main()``, ``partManager()->start()``, or ``runEventLoop()`` if it
wants any of those default behaviors.

Keeping the Framework Boundary
==============================

The design only chooses how application work begins.
All variants still benefit from consistent conversion of ``argc`` and ``argv``, generated help and version output,
framework exception reporting, application-wide services, and cleanup at the process boundary.

Help, version, and command-line errors finish before an application main function is called.
This keeps business logic focused on validated values and prevents asynchronous work from starting merely to display
help.
