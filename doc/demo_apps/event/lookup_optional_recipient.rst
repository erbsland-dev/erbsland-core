..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

************************************
Server Lookup with Optional Progress
************************************

This application retains a lookup's addresses independently of optional progress output.
Turning off the reporter does not cancel the operation or prevent completion.
The example develops the weak-recipient pattern in :doc:`/topics/event/callback_lifetimes`.

Keeping Required Results Separate
=================================

The application owns the source and a list for its required result.
A shared ``LookupProgress`` object supplies optional output.
The result callback borrows the application, which encloses dispatch, and captures a weak pointer to the reporter:

.. erbsland-demo::
    :source: event/apps/LookupOptionalRecipient/LookupOptionalRecipientApp.hpp
    :show: f:begin, c:Required operation(+0,+3)
    :exec: event/lookup_optional_recipient
    :exec-2: event/lookup_optional_recipient --quiet
    :sha256: 55cbcdcf6bb1d39da82c3fbae9a83984d4f600706abcfc983481ca0ba04146e6

.. code-block:: cpp

    void begin() {
        loop()->invokeAfter(_timeout, [this]() -> void { onTimeout(); });
        _lookup = loop()->get<el::Network>().createHostLookup();
        _progress = std::make_shared<LookupProgress>(_host);
        _lookup->events()
            .onResolved(
                [this, weak = std::weak_ptr<LookupProgress>{_progress}](
                    const el::List<el::IpAddress> &addresses) -> void {
                    _addresses = addresses; // Keep the required result independently of progress output.
                    if (const auto progress = weak.lock(); progress != nullptr) {
                        progress->onResolved(addresses);
                    }
                })
            .onError([this](const el::NetworkErrorContext &error) -> void {
                el::io::printLine("Lookup failed: "_el, error.title());
                _exitCode = el::ExitCode::failure();
            })
            .onFinal([this]() -> void {
                _finished = true;
                _lookup.reset();
                for (const auto &address : _addresses) {
                    el::io::printLine("Server address: "_el, address);
                }
                quit(_exitCode); // Completion never depends on the progress reporter.
            });
        _lookup->start(_host);
        if (_quiet) {
            // Turn off an already connected reporter before any result can be dispatched.
            _progress.reset();
        }
        if (_timeout.isZero()) {
            onTimeout();
        }
    }

    el::HostLookupPtr _lookup;                       ///< Required operation, owned by the application.
    el::List<el::IpAddress> _addresses;              ///< Result copied out of the callback's borrowed argument.
    std::shared_ptr<LookupProgress> _progress;       ///< Optional reporter that can be released independently.
    bool _quiet{false};                              ///< Whether to discard progress reporting after setup.

.. erbsland-demo-run::

    .. code-block:: console

        $ event/lookup_optional_recipient

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mResolved 127.0.0.1 to 1 address(es).
        Server address: 127.0.0.1␛[0m

.. erbsland-demo-run::

    .. code-block:: console

        $ event/lookup_optional_recipient --quiet

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mServer address: 127.0.0.1␛[0m

.. erbsland-demo-end::

``--quiet`` disables an already connected reporter before result dispatch.
The weak lock then fails, but the callback still copies the borrowed address list into application-owned storage.
The final callback prints those addresses and ends the operation in both runs.
This is a complete console program; the same separation is useful when an optional user-interface panel closes.

Handling Completion and Cleanup
===============================

All source editing and result processing happen on the main loop.
The final handler releases the source after success or an operational error.
The application deadline releases it when no result arrives, and unconditional cleanup releases both source and
reporter:

.. erbsland-demo::
    :source: event/apps/LookupOptionalRecipient/LookupOptionalRecipientApp.hpp
    :show: f:onTimeout, f:cleanup
    :exec: event/lookup_optional_recipient --quiet --timeout-ms 0
    :exec-exit-code: 1
    :show-cmd-line:
    :sha256: 83f7d9a5d434360e5cf393a5f338db5e3d677552f6d92aef6feb099a805c440a

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

    void cleanup() noexcept override {
        _lookup.reset();
        _progress.reset();
    }

.. erbsland-demo-run::

    .. code-block:: console

        $ event/lookup_optional_recipient --quiet --timeout-ms 0

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mDeadline reached without a lookup result.␛[0m

.. erbsland-demo-end::

The CLI accepts a host, ``--quiet``, and ``--timeout-ms`` (default one second).
Zero aborts at the end of setup before result delivery; ``--help`` describes the options.
The numeric examples require no DNS service.
