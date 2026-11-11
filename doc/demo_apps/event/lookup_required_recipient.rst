..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

************************************
Server Lookup with a Required Report
************************************

This lookup application gives a separate report object responsibility for printing the addresses.
Its result handler retains that report even after setup releases the original pointer.
The example illustrates strong recipient ownership without a cycle; see :doc:`/topics/event/callback_lifetimes` for the
underlying capture rules.

Separating the Report from the Source
=====================================

The report owns its host value and has no pointer to the lookup or application:

.. erbsland-demo::
    :source: event/apps/LookupRequiredRecipient/AddressReport.hpp
    :sha256: 65009509fa39705f1ef6b1a0b648a12569740250ffc5d5bc9ad94ca7cd34d3c0

.. code-block:: cpp

    /// Print the result of a server lookup without owning the operation that supplies it.
    class AddressReport final {
    public:
        /// Prepare the report for a server.
        /// @param host The server whose addresses will be printed.
        explicit AddressReport(el::Host host) : _host{std::move(host)} {}

    public: // output
        /// Print every resolved address.
        /// @param addresses The addresses borrowed for this call.
        void onResolved(const el::List<el::IpAddress> &addresses) {
            for (const auto &address : addresses) {
                el::io::printLine("Server address for "_el, _host.toString(), ": "_el, address);
            }
        }

    private:
        el::Host _host; ///< Owned report context; no pointer back to the source.
    };

.. erbsland-demo-end::

The application owns the lookup, and the lookup's stored result handler owns the report.
The weak diagnostic pointer observes that relationship without extending it:

.. erbsland-demo::
    :source: event/apps/LookupRequiredRecipient/LookupRequiredRecipientApp.hpp
    :show: f:begin, c:Application-owned operation(+0,+1)
    :exec: event/lookup_required_recipient
    :show-cmd-line:
    :sha256: 75a3accd5a767c67ef6d241c241a3aeb8206d267e7a4093c92f48778d2fd1f41

.. code-block:: cpp

    void begin() {
        loop()->invokeAfter(_timeout, [this]() -> void { onTimeout(); });
        _lookup = loop()->get<el::Network>().createHostLookup();
        auto report = std::make_shared<AddressReport>(_host);
        _report = report; // A weak pointer lets the diagnostic observe destruction without preventing it.
        _lookup->events()
            .onResolved([report](const el::List<el::IpAddress> &addresses) -> void { report->onResolved(addresses); })
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
        report.reset(); // The callback has its own shared pointer to the report.
        el::io::printLine("Report retained by the callback: "_el, !_report.expired());
        if (_timeout.isZero()) {
            onTimeout();
        }
    }

    el::HostLookupPtr _lookup;                       ///< Application-owned operation; its callback retains the report.
    std::weak_ptr<AddressReport> _report;            ///< Non-owning diagnostic only.

.. erbsland-demo-run::

    .. code-block:: console

        $ event/lookup_required_recipient

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mReport retained by the callback: true
        Server address for 127.0.0.1: 127.0.0.1
        Report released after cleanup: true␛[0m

.. erbsland-demo-end::

The address is printed after the local report pointer has been reset.
Unlike a controller that owns its source, this report has no return ownership path to the stored callback.

Releasing the Recipient
=======================

Normal completion releases the source.
The independent application deadline and unconditional cleanup provide release paths when completion is missing or a
callback throws.
After the application has finished, ``main()`` prints whether the diagnostic weak pointer has expired:

.. erbsland-demo::
    :source: event/apps/LookupRequiredRecipient/main.cpp
    :show: f:main
    :exec: event/lookup_required_recipient --timeout-ms 0
    :exec-exit-code: 1
    :show-cmd-line:
    :sha256: 98ab2db3fb4a89de5f72811d575f618e36440badb3923c750f2a4a18a8345460

.. code-block:: cpp

    auto main(const int argc, char *argv[]) -> int {
        auto app = demo::LookupRequiredRecipientApp{argc, argv};
        const auto result = app.run();
        app.printCleanupResult();
        return result;
    }

.. erbsland-demo-run::

    .. code-block:: console

        $ event/lookup_required_recipient --timeout-ms 0

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mReport retained by the callback: true
        Deadline reached without a lookup result.
        Report released after cleanup: true␛[0m

.. erbsland-demo-end::

The deadline run has no address line, but still releases the report.
The CLI accepts a host and ``--timeout-ms`` (default one second), with ``--help`` for usage.
Numeric loopback runs require no external resolver.
