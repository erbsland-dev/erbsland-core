..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

**********************************
Server Lookup with an Owned Source
**********************************

This command-line application resolves a server name or IP address and prints every returned address.
It corrects :doc:`lookup_lost_source` by storing the source in an application member.
The setup and output stay similar so the ownership difference is easy to see.
:doc:`/topics/event/callback_lifetimes` explains how to apply this pattern to other event sources.

Owning the Operation Through Dispatch
=====================================

The application object encloses ``run()``:

.. erbsland-demo::
    :source: event/apps/LookupOwnedSource/main.cpp
    :show: f:main
    :sha256: 8dd886fa6eabee30c80fc51dbe24fe20622429f1eaab9ce7502d15599511adb4

.. code-block:: cpp

    auto main(const int argc, char *argv[]) -> int {
        auto app = demo::LookupOwnedSourceApp{argc, argv};
        return app.run();
    }

.. erbsland-demo-end::

Option handling queues startup on the main loop.
``_lookup`` keeps the operation alive after setup, and the result handler copies the host it will report:

.. erbsland-demo::
    :source: event/apps/LookupOwnedSource/LookupOwnedSourceApp.hpp
    :show: f:begin, c:Keeps the source alive(+0,+4)
    :exec: event/lookup_owned_source
    :show-cmd-line:
    :sha256: 9ec2abd313c8cca6bc8f463319170ffff09c58a78be6a206376940bd9fd4a2d5

.. code-block:: cpp

    void begin() {
        el::io::printLine("Looking up server: "_el, _host.toString());
        // This deadline belongs to the application, independently of the lookup.
        loop()->invokeAfter(_timeout, [this]() -> void { onTimeout(); });
        _lookup = loop()->get<el::Network>().createHostLookup();
        _lookup->events()
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
                _lookup.reset();
                quit(_exitCode);
            });
        _lookup->start(_host);
        el::io::printLine("Setup complete; waiting for the result."_el);
        if (_timeout.isZero()) {
            onTimeout();
        }
        // Otherwise, the member retains the lookup after this function returns.
    }

    el::HostLookupPtr _lookup;                       ///< Keeps the source alive between setup and completion.
    el::Host _host;                                  ///< Server selected on the command line.
    el::TimeDelta _timeout;                          ///< Independent application deadline.
    el::ExitCode _exitCode{el::ExitCode::success()}; ///< Result reported when the operation finishes.
    bool _finished{false};                           ///< Prevents a late deadline from changing a completed result.

.. erbsland-demo-run::

    .. code-block:: console

        $ event/lookup_owned_source

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mLooking up server: 127.0.0.1
        Setup complete; waiting for the result.
        Server address for 127.0.0.1: 127.0.0.1␛[0m

.. erbsland-demo-end::

A numeric address still delivers its result asynchronously, so this run exercises the ownership interval without DNS.
An unresolved name can be supplied instead; its returned addresses depend on the resolver.

Ending on Success, Failure, or Deadline
=======================================

The error handler records failure, and normal final delivery releases the source and requests application exit.
A separate application deadline releases the source if completion does not arrive.
Unconditional cleanup also releases it when an exception ends dispatch:

.. erbsland-demo::
    :source: event/apps/LookupOwnedSource/LookupOwnedSourceApp.hpp
    :show: f:onTimeout, f:cleanup
    :exec: event/lookup_owned_source --timeout-ms 0
    :exec-exit-code: 1
    :show-cmd-line:
    :sha256: 7743d0b98487ae8348ba7c2eb7d55f13c175b5ed2e8ae60b99e0ae368efb7684

.. code-block:: cpp

    void onTimeout() {
        if (_finished) {
            return;
        }
        _finished = true;
        _lookup.reset();
        el::io::printLine("Deadline reached without a lookup result."_el);
        quit(el::ExitCode::failure());
    }

    void cleanup() noexcept override { _lookup.reset(); }

.. erbsland-demo-run::

    .. code-block:: console

        $ event/lookup_owned_source --timeout-ms 0

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mLooking up server: 127.0.0.1
        Setup complete; waiting for the result.
        Deadline reached without a lookup result.␛[0m

.. erbsland-demo-end::

Zero aborts at the end of setup, before returning to result dispatch, demonstrating cleanup without an external
dependency.
The default deadline is one second; ``--help`` describes both options.
The application survives ``run()``, so the handlers can borrow its ``this`` safely.
