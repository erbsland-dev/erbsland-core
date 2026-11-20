..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Demo Applications; ManifestVerifier

*************************************************
ManifestVerifier and the Last Verification Report
*************************************************

A delivery includes a checksum manifest and three files.
The price list has changed since the sender created the manifest.
An operator running a full audit wants every mismatch.
An automated import wants to stop at the first mismatch and know how much of the shipment remains unchecked.

ManifestVerifier supports both decisions.
Build the ``manifest-verifier`` target and run it from ``demos/event/apps/ManifestVerifier/example-files`` with
``shipment.sha256``.
Each nonempty manifest line contains a SHA-256 digest, two spaces, and a file name relative to the manifest.
The worker hashes the file's bytes in chunks of 64 KiB.
A mismatch produces exit status 1 in either mode.

Two Reports for the Same Shipment
=================================

.. erbsland-demo::
    :source: event/apps/ManifestVerifier/src/VerificationSession.cpp
    :show: f:VerificationSession::printSummary
    :files: ../../../demos/event/apps/ManifestVerifier/example-files/shipment.sha256 ../../../demos/event/apps/ManifestVerifier/example-files/catalog.txt ../../../demos/event/apps/ManifestVerifier/example-files/prices.txt ../../../demos/event/apps/ManifestVerifier/example-files/stock.txt
    :exec: event/manifest-verifier shipment.sha256
    :exec-exit-code: 1
    :exec-2: event/manifest-verifier --fail-fast shipment.sha256
    :exec-2-exit-code: 1
    :sha256: b1ac6d8d63067fd39d621a1d3e9ccfc39034b09960f7131d6004c2051fe2ffd5

.. code-block:: cpp

    void VerificationSession::printSummary() const {
        el::io::printLine(
            "Checked: "_el, _checked, ", mismatches: "_el, _mismatches, ", unchecked: "_el, _entries.count() - _checked);
    }

.. erbsland-demo-run::

    .. code-block:: console

        $ event/manifest-verifier shipment.sha256

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mOK        catalog.txt
        MISMATCH  prices.txt
        OK        stock.txt
        Checked: 3, mismatches: 1, unchecked: 0␛[0m

.. erbsland-demo-run::

    .. code-block:: console

        $ event/manifest-verifier --fail-fast shipment.sha256

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mOK        catalog.txt
        MISMATCH  prices.txt
        Checked: 2, mismatches: 1, unchecked: 1␛[0m

.. erbsland-demo-end::

In the first run the operator can use the result for ``stock.txt``.
In the second run there is no claim about that file.
The unchecked count makes a partial verification visible even to someone who sees only the final line.

One Decision Between Files
==========================

The procedural application keeps a verification session on its stack for the whole run.
Its standalone owner loop runs on the calling thread.
An unmanaged event thread reads files.
The manifest is immutable across both threads; only the owner updates the report counters.

.. erbsland-demo::
    :source: event/apps/ManifestVerifier/src/VerificationSession.cpp
    :show: f:VerificationSession::check, f:VerificationSession::record
    :sha256: beaee4f31bc11d330386471dd7fb9be97abd08c96f1a413d39535734f03f0a2b

.. code-block:: cpp

    void VerificationSession::check(const el::ItemIndex index, const el::StopToken &stop) {
        if (stop.isStopRequested() || !index.isWithin(_entries.count())) {
            _owner->invoke([this]() -> void { finish({}); });
            return;
        }
        const auto &entry = _entries.getRefOrThrow(index);
        const auto matches = entry.matchesFile(_directory);
        _owner->invoke([this, index, matches]() -> void { record(index, matches); });
    }

    void VerificationSession::record(const el::ItemIndex index, const bool matches) {
        ++_checked;
        const auto &name = _entries.getRefOrThrow(index).name();
        if (matches) {
            el::io::printLine("OK        "_el, name);
        } else {
            ++_mismatches;
            el::io::printLine("MISMATCH  "_el, name);
            if (_failFast) {
                _cancellation.requestStop();
            }
        }
        schedule(index.incremented());
    }

.. erbsland-demo-end::

The worker receives permission to check one entry.
When the price list fails, it has already returned the result and needs another command before it can open the stock
file.
The owner records the mismatch, requests cancellation in fail-fast mode, and schedules that command.
On the worker, the token check now sends a terminal reply.
Normal end-of-manifest uses the same reply.
At most one command is outstanding, so a later result cannot race the stop decision.

Finish While the Report Can Still Run
=====================================

.. erbsland-demo::
    :source: event/apps/ManifestVerifier/src/VerificationSession.cpp
    :show: f:VerificationSession::schedule, f:VerificationSession::finish
    :sha256: 767cc1c31ac75246e095390f45a0aceab52f9827dd1e5f9fe2346baa6ad75410

.. code-block:: cpp

    void VerificationSession::schedule(const el::ItemIndex index) {
        _worker->loop()->invoke([this, index, stop = _cancellation.stopToken()]() -> void {
            try {
                check(index, stop);
            } catch (const el::RuntimeError &) {
                _owner->invoke([this, error = std::current_exception()]() -> void { finish(error); });
            }
        });
    }

    void VerificationSession::finish(std::exception_ptr error) {
        _error = std::move(error);
        _owner->invoke([this]() -> void { printSummary(); });
        _owner->quit();
    }

.. erbsland-demo-end::

A file that cannot be read has no checksum result.
Its worker command catches the Core runtime error and delivers it through the same terminal path.
The file and all later entries remain unchecked.
The owner stores the error, submits its report, and closes admission.
Dispatch drains the accepted report.
Asking the owner to quit immediately upon the price mismatch would close the return path too soon.

.. erbsland-demo::
    :source: event/apps/ManifestVerifier/src/VerificationSession.cpp
    :show: f:VerificationSession::run
    :sha256: ea9540c68071212637160d101c84299a809fc9c314da31c5f2557cef6fccb2ee

.. code-block:: cpp

    auto VerificationSession::run() -> bool {
        _worker->start();
        try {
            schedule(el::ItemIndex{});
            _owner->run();
        } catch (...) {
            _cancellation.requestStop();
            _worker->quit();
            _worker->join();
            throw;
        }
        _worker->quit();
        _worker->join();
        if (const auto error = _owner->takeError()) {
            std::rethrow_exception(error);
        }
        if (_error) {
            std::rethrow_exception(_error);
        }
        return _mismatches.isZero();
    }

.. erbsland-demo-end::

Once owner dispatch ends, the worker needs no further decision in order to quit.
Joining here keeps the stack-owned session alive through its last use on the worker.
An owner-side exception also quits and joins the worker, which never blocks waiting for its next command.
After a normal drain the caller can distinguish a checksum mismatch from an I/O failure, which is rethrown with its
diagnostic.

See :doc:`/topics/event/shutting_down_event_work` for how this ordering changes when several operations share a worker
or an application part owns the resource.
