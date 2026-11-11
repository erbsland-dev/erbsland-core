..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

********************************
Server Lookup with a Lost Source
********************************

This deliberately broken command-line application resolves a server name or IP address, but keeps its source only in the
setup function.
It makes a missing result observable without accessing destroyed objects or waiting indefinitely.
Read :doc:`/topics/event/callback_lifetimes` for the ownership lesson and
:doc:`lookup_owned_source` for the working version.

Starting the Request
====================

The application parses a host (default ``127.0.0.1`` and a deadline (default 1000 milliseconds), then queues
``begin()``.
Creation, editor access, and startup therefore happen on the lookup's owner loop.
The callback copies the host value, but no member retains the source itself:

.. erbsland-demo::
    :source: event/apps/LookupLostSource/LookupLostSourceApp.hpp
    :show: f:handleCommandLineOptions, f:begin, c:Server selected(+0,+3)
    :sha256: 9a5126e5da933a2d3a3cd3215f78a695d50e293d1d65e72aaffec0a38088c375

.. code-block:: cpp

    void handleCommandLineOptions(const el::OptionValuesPtr &values) override {
        _host = el::Host::fromStringOrThrow(values->getText("host"_el));
        const auto timeout = values->getInteger("timeout"_el);
        if (timeout < 0) {
            throw el::OptionError{"The deadline must be non-negative."_el, values->value("timeout"_el)};
        }
        _timeout = el::TimeDelta::milliseconds(timeout);
        loop()->invoke([this]() -> void { begin(); });
    }

    void begin() {
        el::io::printLine("Looking up server: "_el, _host.toString());
        // This deadline belongs to the application, independently of the lookup.
        loop()->invokeAfter(_timeout, [this]() -> void { onTimeout(); });
        const auto lookup = loop()->get<el::Network>().createHostLookup();
        lookup->events()
            .onResolved([host = _host](const el::List<el::IpAddress> &addresses) -> void {
                for (const auto &address : addresses) {
                    el::io::printLine("Server address for "_el, host.toString(), ": "_el, address);
                }
            })
            .onError([this](const el::NetworkErrorContext &error) -> void {
                el::io::printLine("Lookup failed: "_el, error.title());
                _exitCode = el::ExitCode::failure();
            })
            .onFinal([this]() -> void {
                _finished = true;
                quit(_exitCode);
            });
        lookup->start(_host);
        el::io::printLine("Setup complete; waiting for the result."_el);
        if (_timeout.isZero()) {
            onTimeout();
        }
        // BUG: the only owning pointer is destroyed here, before result dispatch.
    }

    el::Host _host;                                  ///< Server selected on the command line.
    el::TimeDelta _timeout;                          ///< Independent application deadline.
    el::ExitCode _exitCode{el::ExitCode::success()}; ///< Result reported when the operation finishes.
    bool _finished{false};                           ///< Prevents a late deadline from changing a completed result.

.. erbsland-demo-end::

Returning from ``begin()`` destroys the last source pointer.
The queued result is suppressed, and the final handler cannot end the application either.
A callback that captures its inputs correctly cannot repair this independent source-lifetime problem.

Observing the Missing Result
============================

The application's deadline runs independently of the lookup.
It prints a diagnostic and selects a failure exit code:

.. erbsland-demo::
    :source: event/apps/LookupLostSource/LookupLostSourceApp.hpp
    :show: f:onTimeout
    :exec: event/lookup_lost_source
    :exec-exit-code: 1
    :show-cmd-line:
    :sha256: 69c3f2598c6e17a2c5ef04e47d43e0924e9eac053fd94cfd7c72635295392e38

.. code-block:: cpp

    void onTimeout() {
        if (_finished) {
            return;
        }
        _finished = true;
        el::io::printLine("Deadline reached without a lookup result."_el);
        quit(el::ExitCode::failure());
    }

.. erbsland-demo-run::

    .. code-block:: console

        $ event/lookup_lost_source

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mLooking up server: 127.0.0.1
        Setup complete; waiting for the result.
        Deadline reached without a lookup result.␛[0m

.. erbsland-demo-end::

The numeric address requires no DNS service, yet the expected address line is missing.
Changing the deadline cannot make the destroyed source deliver it.
``--help`` describes the options; ``--timeout-ms 0`` aborts at the end of setup.
The application object outlives all dispatch, making its borrowed ``this`` valid in the deadline callback.
