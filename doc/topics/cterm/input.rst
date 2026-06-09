..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

*****
Input
*****

The input classes provide access to keyboard input from the terminal.
They are designed for interactive applications such as dashboards, tools, and terminal games that need immediate key
handling.

Usage
=====

Polling for Keys in a Redraw Loop
---------------------------------

For interactive applications, switch the input backend to key mode and poll for input with a timeout.
This allows your application to update the screen regularly while still reacting to keyboard events.

.. code-block:: cpp

    using namespace std::chrono_literals;

    terminal.input().setMode(Input::Mode::Key);
    auto quitRequested = false;

    while (!quitRequested) {
        if (const auto key = terminal.input().readKey(90ms); key.valid()) {
            if (key == U'q') {
                quitRequested = true;
            } else if (key == Key::Left) {
                // Move selection.
            }
        }
    }

Using a timeout keeps the redraw loop responsive without busy waiting.
In key mode, ``readKey(0ms)`` and any negative timeout perform a non-blocking poll.
Use ``waitForKey()`` when you intentionally want to block until the next key arrives.
The older ``read()`` wrapper is deprecated and now forwards to either ``readKey()`` or ``waitForKey()``.

Switching Between Key and Line Input
------------------------------------

``Input::Mode`` controls whether the terminal reads raw key presses or full lines of text.
``Mode::Key`` is the right choice for interactive applications with a redraw loop, while ``Mode::ReadLine`` fits
prompts, configuration tools, and simple command-driven interfaces.

.. code-block:: cpp

    terminal.input().setMode(Input::Mode::ReadLine);
    terminal.print("Name: ");
    const auto name = terminal.input().readLine();

    terminal.input().setMode(Input::Mode::Key);
    terminal.printLine("Press any key to continue...");
    const auto key = terminal.input().waitForKey();

Switching modes on the same terminal makes it easy to combine menu-driven screens with occasional free-form text input.

Describing Key Bindings
-----------------------

``InputDefinition`` represents a key binding together with the input mode it applies to.
It is useful when describing configurable shortcuts or when displaying the currently active bindings.

.. code-block:: cpp

    auto quitKey = InputDefinition{Key{Key::Character, U'q'}, InputDefinition::ForMode::Key};
    auto helpKey = InputDefinition{Key{Key::F1}, InputDefinition::ForMode::Key};

    std::cout << "Quit: " << quitKey.toDisplayText() << "\n";
    std::cout << "Help: " << helpKey.toString() << "\n";

The helper functions ``toDisplayText()`` and ``toString()`` make it easy to present key bindings in help screens or
configuration output.

For interactive tools with one command bound to several keys, use ``Keys``.
It stores unique key presses in priority order, can test whether a decoded key is part of the set, and can separate main
keys from alternatives for compact and detailed help.

Special keys may carry modifiers such as ``shift+up``, ``ctrl+pageup``, or ``alt+f4``.
Modified keys are distinct from their unmodified base key, so a binding for ``up`` does not also match ``shift+up``.
Printable text input remains text input: pressing Shift with a letter produces the resulting character, for example
``Q``.

Matching Decoded Key Types
--------------------------

When you only care about the general kind of a key event, inspect ``Key::Type`` instead of comparing against a long list
of individual keys.
For character comparisons, prefer Unicode code points such as ``U'q'`` and use ``Key::unicode()`` or ``Key::combined()``
instead of the deprecated ASCII accessor.
Character comparisons match the exact decoded code point, so compare against the exact character you want to handle.

.. code-block:: cpp

    using namespace std::chrono_literals;

    if (const auto key = terminal.input().readKey(50ms); key.valid()) {
        if (key.type() == Key::Character || key.type() == Key::Combined) {
            terminal.printLine("Typed: ", BlockString{key.combined()});
        } else if (key.type() == Key::Escape) {
            terminal.printLine("Leaving key mode.");
        }
    }

This is especially handy when the application wants to distinguish between text entry, navigation keys, and control keys
before it decides how to handle the event.

.. figure:: demos/images/retro-plasma.jpg
    :width: 100%

    The animated demos use ``Input`` in key mode so screen redraws and
    keyboard handling remain responsive.
