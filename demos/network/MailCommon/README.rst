TLS Mail Demonstrations
=======================

``MailSender`` and ``MailReceiver`` are small consumer applications supporting the TLS connection topics.
``MailCommon/src`` contains their shared configuration, logging, and SMTP framing helpers.
Their application sources use public Core headers and can be reused with an ordinary ``erbsland::core`` dependency.

Build and run
-------------

From the repository root::

    cmake --build cmake-build-debug --target mail_sender mail_receiver erbsland-core-mail-unittest
    cmake-build-debug/demo-apps/network/mail_receiver --once --timeout 8 --verbose

In another terminal::

    cmake-build-debug/demo-apps/network/mail_sender --verbose

The default configuration is found beside the executable, independently of the current working directory.
An explicit ``--config`` path is resolved from the working directory; paths inside it are relative to that file.
The receiver normally stays running.
``--once`` ends after one completed exchange, and ``--timeout`` bounds service lifetime.
Both applications provide ``--help`` and explicit overrides for their main settings.

Configurations
--------------

The build copies these files and their relative certificate directories beside the executables:

* ``mail_sender.elcl`` trusts the demo CA and sends no client certificate.
* ``mail_sender_identity.elcl`` additionally presents a client certificate when requested.
* ``mail_receiver.elcl`` authenticates the server only.
* ``mail_receiver_required.elcl`` requires a valid client certificate.
* ``mail_receiver_optional.elcl`` accepts anonymous clients and validates certificates when presented.
* ``mail_receiver_sni.elcl`` requires a client certificate for ``localhost``; numeric loopback access selects its
  default profile, which does not request client authentication.

These applications use TLS immediately on connection, without STARTTLS or SMTP AUTH.
Message content and names are ASCII; addresses use simple mailboxes without quoted local parts.
Messages are limited to 64 KiB, data lines to 998 characters before dot escaping, and commands/replies to 510 characters
before CRLF.
The receiver logs messages and does not relay or deliver them to external mailboxes.

Demonstration certificates
--------------------------

All distributed private keys are public demonstration material.
Do not use them for deployed services.
The server identity covers ``localhost``, ``127.0.0.1``, and ``::1``.
The CA and leaf certificates have ten-year validity from generation, and the leaf certificates carry the appropriate
server/client extended key usages.

Regenerate the whole set together, then rebuild and rerun the integration tests::

    .venv/bin/python3 demos/network/MailCommon/generate_certificates.py
    cmake --build cmake-build-debug --target mail_sender mail_receiver

Only regeneration requires an ``openssl`` command.
The disposable CA private key is not distributed.
Regular builds and runs use the checked-in certificates and need no certificate-generation tools.

Validation and documentation
----------------------------

Run the demo unit tests and local TLS integration tests with::

    ctest --test-dir cmake-build-debug -R 'erbsland-core-mail-unittest|mail-tls-integration' --output-on-failure

The integration tests bind automatic loopback ports and use only local processes, including Python's TLS implementation
for an independent peer.
They require a Python interpreter with TLS 1.3 support.

Normalize edited topic pages with the native maintenance command::

    ./erbsland-core-tool doc-process-rst --recursive doc/topics/network

This leaves saved demo excerpts and output unchanged.
Explicit demo synchronization uses ``doc-process-rst --demos sync`` as described in the demo integration guidelines.
Mail exchanges require a running peer with the matching configuration; arrange that peer before capturing an individual
example, following the build-and-run instructions above.
Review the saved output after changing binaries or certificates.
