..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    !single: Core Application Framework
    single: Application; Overview
    single: ApplicationPart; Overview

**************************
Core Application Framework
**************************

An executable needs one place where platform arguments become library values, options are parsed, shared services are
created, work begins, failures are reported, and shutdown is coordinated.
:cpp:class:`Application <erbsland::core::Application>` provides that boundary while allowing the program itself to stay
as small or as structured as its problem requires.

The topics in this chapter begin with five ways to organize an application.
They then continue into the application-part framework and version metadata that larger programs commonly need.

Choosing an Application Design
==============================

The same ``Application`` lifecycle supports a compact pair of functions, synchronous processing, asynchronous event
dispatch, command modules, and dependency-managed services.
:doc:`choosing_an_application_design` compares these designs and helps you select the smallest one that matches the
program's control flow.

Writing Small Applications with Functions
=========================================

A short executable often needs the framework lifecycle without needing a new class.
:doc:`function_based_applications` shows how initialization and main functions can configure and run such a program.

Building Event-Driven Applications
==================================

Servers, monitors, and asynchronous clients spend most of their lifetime reacting to events.
:doc:`event_driven_applications` explains how the default main implementation enters the application event loop and how
callbacks complete the process through ``quit()``.

Building Procedural Applications
================================

Converters, report generators, and batch tools often follow one synchronous path from parsed input to an exit code.
:doc:`procedural_applications` shows how an ``Application`` subclass can express that path in an overridden ``main()``.

Organizing Command-Style Tools
==============================

Some executables contain several actions selected by the first command-line argument.
:doc:`command_style_applications` connects action objects with option modules so parsing, help, and dispatch remain close
to each command.

Composing Applications from Services
====================================

Long-running programs may need independently managed services with explicit startup and shutdown dependencies.
:doc:`applications_from_parts` introduces application-owned parts and the lifecycle integration supplied by the default
application main loop.

:doc:`application_parts` continues with service interfaces, dependency declarations, lifecycle hooks, command-line
forwarding, failure handling, and cross-thread access.
:doc:`detached_application_parts` explains how to run the same kind of service graph without an ``Application`` owner.

Running as a Service or Foreground Daemon
=========================================

Servers can use one executable for interactive development, foreground operation under a POSIX supervisor, and a Windows
service.
:doc:`service_lifecycle` explains graceful termination, readiness reporting, and application-part integration.

Adding Build Version Information
================================

Applications can expose a version derived from Git without copying it into source code.
:doc:`adding_git_version_to_your_app` explains the CMake setup and how the generated value reaches application metadata.

Creating Release Packages
=========================

:doc:`release_packages` shows how to create standalone Windows and macOS ZIP packages as part of CMake installation.
