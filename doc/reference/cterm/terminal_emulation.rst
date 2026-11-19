..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

******************
Terminal Emulation
******************

``TerminalEmulation`` consumes raw UTF-8 terminal output without opening a terminal or launching a process.
Use it for subprocess captures, saved terminal output, or a live preview rendered from buffers.

Incremental Input
=================

Construct the emulator with ``TerminalEmulationOptions``, then call ``write()`` for each incoming ``String``.
Strings may contain malformed UTF-8, split UTF-8 characters, and split ANSI sequences.
Complete characters are interpreted immediately; incomplete UTF-8 and escape sequences remain pending.
Call ``finish()`` at EOF: incomplete UTF-8 becomes replacement characters and unfinished controls are discarded.
Repeated finishing is harmless; writing after finishing requires ``reset()``.
Reset clears both buffers, saved cursor states, decoding, and parsing while retaining the construction options.
Instances are not internally synchronized.

Screens and Coordinates
=======================

``regularBuffer()``, ``alternateBuffer()``, and ``activeBuffer()`` return const references owned by the emulator.
Both terminal frames retain the configured width and height and scroll at their bottom edge.
ANSI cursor coordinates address the active frame; cursor movement cannot address saved history.

``scrollbackBuffer()`` retains complete rows that scroll out of the regular frame, oldest first.
It starts with one empty placeholder row; the first departing row replaces that placeholder.
``hasScrollback()`` distinguishes the placeholder from actual history, including an actual blank row.
History grows vertically to ``scrollbackLimit()``, then discards its oldest row when another row arrives.
Alternate-screen scrolling never enters regular-screen history.
Reset clears all three buffers.
``CSI 3 J`` clears history while leaving the active frame intact.

DEC mode 47 switches buffers without clearing them.
Mode 1047 clears the alternate screen on exit.
Mode 1049 clears it on entry and also saves and restores the regular cursor and style.
Mode 1048 saves or restores the current screen's cursor and style without switching screens.
Leaving with mode 47 or 1049 retains the alternate contents for inspection.

Options and Supported Controls
==============================

Defaults are 120 columns, 25 initial rows, a scrollback limit of 10,000 rows, automatic wrapping, eight-column tabs, and
LF returning to column zero.
Screen dimensions, scrollback limit, and tab width accept values from 1 through 10,000. The effective maximum height is
at least the initial height, independently of setter order.
The emulator copies its options, so later option changes do not affect it.

Supported controls include CR, LF, BS, tabs, cursor movement, save/restore, erasing, scrolling, and alternate screens.
Presentation includes default/basic/bright colors and the attributes represented by ``BlockAttributes``.
Unsupported sequences are consumed without rendering their control payload.
Extended color parameters are consumed together and do not become unrelated character attributes.

Unicode uses Core's terminal-cell width rules and ``Block`` representation, including its limit of two combining code
points following a base character.
This is not a general grapheme-shaping or bidirectional-layout engine.

Rendering
=========

Serialize a frame or the history using its inherited ``toString()`` method and ``BufferRenderOptions``.
Combine populated history with the regular frame when a complete capture is needed.
Preserve empty bottom rows in history so blank lines between past and current output are retained.
When the alternate screen is active, a screen capture normally serializes only that frame.
See :doc:`buffer_rendering` for format and trimming policies.

Interface
=========

.. doxygenclass:: erbsland::cterm::TerminalEmulation
    :members:
.. doxygenclass:: erbsland::cterm::TerminalEmulationOptions
    :members:
