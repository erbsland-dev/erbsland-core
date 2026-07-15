..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

********
Terminal
********

The terminal classes provide the runtime connection between your rendered content and the real console.
``Terminal`` manages the application lifecycle, refresh strategy, cursor control, direct text output, and the platform
backend that ultimately emits text and control sequences.

This section focuses on application-level terminal control.
For the shared streaming output API used by both ``Terminal`` and ``CursorBuffer``, see
:doc:`cursor-output`. For custom backend implementations and capability
fallback behavior, see :doc:`backend`.

Usage
=====

Preparing and Rendering a Screen
--------------------------------

``Terminal`` is the high-level entry point for full-screen terminal applications.
It owns the platform backend, manages the visible screen state, and renders ``ReadableBuffer`` objects with clipping and
optional crop marks.

.. code-block:: cpp

    auto terminal = Terminal{BlockSize{90, 28}};
    terminal.initializeScreen();
    terminal.setRefreshMode(Terminal::RefreshMode::Overwrite);

    auto buffer = Buffer{terminal.size()};
    buffer.fill(Block{" ", Color{fg::Default, bg::Black}});
    buffer.drawBlockText(
        "Frame and buffer rendering",
        BlockRectangle{2, 2, buffer.size().width() - 4, 5},
        Alignment::Center,
        Color{fg::BrightWhite, bg::Blue});

    auto settings = UpdateSettings{};
    settings.setSwitchToAlternateBuffer(false);

    terminal.updateScreen(buffer, settings);
    terminal.flush();
    terminal.restoreScreen();

``Terminal::size()`` returns the drawable size used by ``updateScreen()``.
By default, this includes a one-column and one-row compatibility margin so full-screen buffers work out of the box.

Disable that margin with ``setSafeMarginEnabled(false)`` if you want to use the full detected terminal size and rely
purely on cursor-based updates.

For short-lived tools that should leave their output in the normal terminal history, disable alternate-screen switching
via ``UpdateSettings`` as shown above.

Render Loop for Interactive Applications
----------------------------------------

The following example shows a minimal render loop for an interactive terminal application.

.. code-block:: cpp

    struct MyApp {
        void renderFrame() {
            _terminal.testScreenSize();
            _buffer.resize(_terminal.size());
            _buffer.fill(Block{" ", bg::Black});

            // Draw the current frame into the buffer.
            _terminal.updateScreen(_buffer, _updateSettings);
        }

        void handleKey(Key key) {
            if (key == U'q') {
                _quitRequested = true;
            }
        }

        void run() {
            _terminal.initializeScreen();
            _terminal.input().setMode(Input::Mode::Key);

            _updateSettings.setMinimumSize(BlockSize{40, 20});
            _updateSettings.setMinimumSizeMessage(BlockString("Too Small!"));

            while (!_quitRequested) {
                const auto key = _terminal.input().readKey(std::chrono::milliseconds{90});
                if (key.valid()) {
                    handleKey(key);
                }
                renderFrame();
            }

            _terminal.restoreScreen();
        }

        Terminal _terminal;
        Buffer _buffer;
        UpdateSettings _updateSettings;
        bool _quitRequested;
    };

Call ``Terminal::initializeScreen()`` once when your application starts, and ``Terminal::restoreScreen()`` once just
before it exits.

After initialization, use ``Terminal::isInteractive()`` to detect whether the terminal's output endpoint is attached to
a real terminal.
The platform backends base this decision on standard output, because that is where ``Terminal`` emits its data.
Redirecting standard output therefore makes the terminal non-interactive even if standard error, standard input, or a
controlling terminal is still attached.

Initialization and restoration do not emit terminal control sequences for a non-interactive output endpoint.
Switch to a plain-text output mode before writing through the terminal itself when output is redirected.

If your application is interrupted (for example via ``Ctrl+C`` ), the library restores the terminal state automatically
before returning control to the shell.

Create exactly one ``Terminal`` instance for the lifetime of your application.
While multiple instances are possible, they share the same underlying backend, so a single instance is simpler and
safer.

For interactive programs, switch the input mode to ``Input::Mode::Key``.
This hides the cursor and lets your application react to individual key presses via ``input().readKey()``.
The timeout also serves as a natural frame rate control.

Keep a persistent ``Buffer`` instance, resize it to the current terminal size, and reuse it for each frame.
After drawing into the buffer, call ``updateScreen()`` to present the new frame.

By default, rendering is optimized to update only the parts of the screen that actually changed.
This makes frequent refresh rates practical, as only modified regions are written to the terminal.

Direct Writes, Line Buffering, and Refresh Modes
------------------------------------------------

For simpler tools and status output, you can use ``Terminal`` without a full-screen buffer.
``print()``, ``printLine()``, and ``write()`` send text directly to the terminal while still applying color handling.

These direct-write functions are part of the shared ``CursorWriter`` interface.
This page focuses on how they behave on a real terminal; the reusable streaming API itself is documented on
:doc:`cursor-output`.

.. code-block:: cpp

    terminal.setOutputMode(Terminal::OutputMode::FullControl);
    terminal.printLine(fg::BrightGreen, "Service started");
    terminal.print("Current mode: ", fg::BrightYellow, "interactive");
    terminal.writeLineBreak();
    terminal.flush();

Line buffering keeps partial lines in memory until a newline is written or ``flush()`` is called.
This keeps incremental output clean and avoids fragmented lines.

For full-screen applications:

* ``RefreshMode::Clear`` clears the entire screen before each frame.
* ``RefreshMode::Overwrite`` moves to the home position and works with
  the back buffer for efficient updates.

Controlling Screen Update Settings
----------------------------------

``UpdateSettings`` controls how ``Terminal::updateScreen()`` behaves when content does not fit the terminal or the
terminal is too small.

.. code-block:: cpp

    auto settings = UpdateSettings{};
    settings.setMinimumSize(BlockSize{60, 18});
    settings.setMinimumSizeBackground(Block{" ", Color{fg::Inherited, bg::Red}});
    settings.setMinimumSizeMessage(BlockString{"Please enlarge the terminal"});
    settings.setShowCropMarks(true);

    terminal.updateScreen(buffer, settings);

Use this to define minimum sizes, customize fallback backgrounds, and control crop indicators when the layout cannot be
rendered as intended.

Output Modes
------------

``Terminal`` supports two output modes:

* ``Terminal::OutputMode::FullControl``
  (default) enables colored output, cursor control, refresh handling,
  back-buffer updates, and terminal-size detection.
* ``Terminal::OutputMode::BlockText``
  keeps the terminal in plain-text mode. In this mode, ``write()`` and
  ``print()`` still work, but cursor control, refresh handling, screen
  clearing, size detection, and the back buffer are disabled.

The terminal always owns a backend internally, but most applications can treat it as an implementation detail and rely
on the high-level API.

Scoped Terminal Sessions
------------------------

``TerminalSession`` is the small RAII helper for code that should always pair ``initializeScreen()`` and
``restoreScreen()``.

.. code-block:: cpp

    auto terminal = Terminal{};
    auto session = TerminalSession{terminal};

    terminal.setRefreshMode(Terminal::RefreshMode::Overwrite);
    terminal.printLine(fg::BrightGreen, "Interactive section is active");
    terminal.flush();

This is especially useful in short tools, tests, or setup-heavy code paths where multiple return statements would
otherwise make screen restoration easy to forget.

If you need to inject a custom backend or understand capability fallback behavior, continue with :doc:`backend`.
