.. index::
    single: Interactive Line Editor
    single: Terminal Input; ReadLine

***********************
Interactive Line Editor
***********************

Introduction
============

:cpp:class:`erbsland::cterm::ReadLine <erbsland::cterm::ReadLine>` provides styled interactive text editing on a full-control terminal.
It supports blocking and polling lifecycles, multiline input, history, inactivity timeouts, and deterministic cleanup.
See :doc:`/topics/cterm/input` for complete usage and behavior.

Usage
=====

Create the editor with :cpp:func:`erbsland::cterm::ReadLine::create() <erbsland::cterm::ReadLine::create>`, a
:cpp:type:`erbsland::cterm::TerminalPtr <erbsland::cterm::TerminalPtr>`, and
:cpp:class:`erbsland::cterm::ReadLineOptions <erbsland::cterm::ReadLineOptions>`.
Use :cpp:func:`erbsland::cterm::ReadLine::waitForInput() <erbsland::cterm::ReadLine::waitForInput>` for a blocking
prompt, or combine
:cpp:func:`erbsland::cterm::ReadLine::start() <erbsland::cterm::ReadLine::start>`, :cpp:func:`erbsland::cterm::ReadLine::update() <erbsland::cterm::ReadLine::update>`, and
:cpp:func:`erbsland::cterm::ReadLine::stop() <erbsland::cterm::ReadLine::stop>` with an application loop.

Only :cpp:member:`erbsland::cterm::ReadLineStatus::Committed` is successful and carries entered text.
The polling-only :cpp:member:`erbsland::cterm::ReadLineStatus::Idle`, cancellation, and timeout statuses carry an empty
string.
An active editor exclusively owns terminal output until it stops.
Horizontal input padding is represented by :cpp:class:`erbsland::block::MarginPair <erbsland::block::MarginPair>`;
negative leading or trailing values are clamped to zero.

Protected Secret Entry
======================

:cpp:class:`erbsland::cterm::ReadSecret <erbsland::cterm::ReadSecret>` is the parallel API for passwords and tokens.
It accepts the appearance and lifecycle parts of ``ReadLineOptions``, forces one logical and display line, rejects
history and initial text, and caps input at 1024 Unicode code points.
It renders a fixed bullet mask and transports a marked :cpp:type:`String <erbsland::text::String>` through
``ReadLineResult``.

Errors and Preconditions
========================

``ReadLine`` and ``ReadSecret`` raise ``TerminalError`` when interactive full-control output is unavailable.
Starting an already active editor or updating an inactive editor violates the lifecycle precondition and raises
``LogicFailure``.
Calling ``stop()`` while inactive remains harmless.

Interface
=========

.. doxygenclass:: erbsland::cterm::ReadLine
    :members:

.. doxygentypedef:: erbsland::cterm::ReadLinePtr
.. doxygenenum:: erbsland::cterm::ReadLineDisplayStyle
.. doxygenclass:: erbsland::cterm::ReadLineOptions
    :members:
.. doxygentypedef:: erbsland::cterm::ReadLineResult
.. doxygenclass:: erbsland::cterm::ReadLineStatus
    :members:
.. doxygenclass:: erbsland::cterm::ReadSecret
    :members:
