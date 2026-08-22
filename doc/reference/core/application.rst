.. index::
    single: Application

***********
Application
***********

:cpp:class:`Application <erbsland::core::Application>` retains non-owning access to the original narrow or wide
``argv`` vector supplied to its constructor.
After option parsing, every suffix reported as sensitive text is overwritten in place with one star per existing byte or
code unit.
The terminating null and native buffer size are preserved.
Its converted :cpp:type:`CommandLineArguments <erbsland::core::CommandLineArguments>` list uses exactly five stars for
each sensitive value.
Masking occurs for successful parsing and parser or validator errors.

The native pointers must remain valid for the application lifetime.
This cleanup only reduces secrets retained in process memory; it cannot retract command-line values already exposed
through process listings, the shell, operating-system facilities, logs, or earlier application code.

Application-Wide Cryptology Policy
==================================

:cpp:func:`Application::cryptologyConfiguration() <erbsland::core::Application::cryptologyConfiguration>` lazily
constructs the process-wide :cpp:class:`CryptologyConfiguration <erbsland::cryptology::CryptologyConfiguration>`.
Its storage belongs to the shared application data, so applications connected with ``Application::linkWith()`` observe
the same configuration across module and DLL boundaries.
Configuration access and changes are thread-safe, and list or recommendation operations use one coherent snapshot.

``setHardwareAccelerationEnabled(false)`` is an operational escape hatch for deployments where a compiled accelerated
backend fails on the installed platform.
Configure it during early startup, before constructing cryptographic workers.
The flag permits automatic backend selection when enabled; it does not claim that hardware support is present.
Changes apply only to subsequently constructed workers.

``setMaximumStatus()`` sets a downgrade-only ceiling for an individual hash algorithm or symmetric encryption type.
The effective selector status is the less permissive of library policy and this ceiling, so configuration cannot promote
a ``Legacy`` or ``Disallowed`` algorithm.
``clearMaximumStatus()`` removes one ceiling and ``reset()`` restores all defaults.
Policy changes affect selector results, not explicit hashing, encryption, or decryption required for existing data.

Application TLS Configuration
=============================

``setTlsConfiguration(label, configuration)`` stores one immutable copy of a ``TlsConfiguration`` under an
application-wide label.
Connections resolve and retain that copy when ``connect()`` starts, so replacement or clearing affects only later
connections.
``reset()`` and ``clearTlsConfigurations()`` remove every registered entry.

Resolution first tries the exact label, then removes the final slash-delimited segment until it reaches the empty
global-default label.
For example, ``http/client/internal`` tries ``http/client/internal``, ``http/client``, ``http``, and ``""``.
One complete entry is selected; fields are never merged between a child and its parent.
An unresolved label or a selected entry without the policy required by a connection role fails synchronously before the
one-shot connection is consumed.

Labels contain at most 16 non-empty slash-separated segments and 255 bytes.
Segments use lowercase ASCII letters, digits, and interior hyphens.
The registry contains at most 256 exact entries.
``hasTlsConfiguration()`` tests only the exact label, while ``resolveTlsConfiguration()`` returns the requested label,
matched label, and immutable entry as one coherent result.

``TlsConfiguration`` stores an optional explicit-anchor
:cpp:class:`X509ServerCertificatePolicy <erbsland::cryptology::X509ServerCertificatePolicy>` and an optional immutable
shared :cpp:class:`TlsServerIdentity <erbsland::cryptology::TlsServerIdentity>`.
Copies and resolved snapshots share the identity without copying its move-only private key.
Protected client identities remain deferred.

``setProtectedDataMode()`` selects how :cpp:class:`ProtectedByteBlock <erbsland::cryptology::ProtectedByteBlock>`
initializes its application-wide protection provider.
Call ``validateProtectedDataSupport()`` during startup when provider availability must become an explicit startup
failure.
Validation or the first non-empty protected block locks the mode; ``reset()`` preserves a locked mode.
See :doc:`/reference/cryptology/key_agreement` for provider behavior and lifecycle restrictions.

Erbsland Core does not automatically read environment variables or command-line options for this interface.
The application decides how enterprise configuration is authenticated, parsed, and applied.

Application Compiled Resources
==============================

:cpp:func:`Application::resources() <erbsland::core::Application::resources>` lazily creates the read-only compiled
resource manager.
The manager indexes statically linked descriptors on first access and caches decoded data and text independently.
An application with no compiled descriptors receives an empty manager.
See :doc:`/topics/resource/compiled_resources` for CMake integration and lookup examples.

Interface
=========

.. doxygenclass:: erbsland::core::Application
    :members:

.. doxygenfunction:: erbsland::core::application() -> Application &
.. doxygenclass:: erbsland::core::ApplicationError
    :members:
.. doxygenclass:: erbsland::core::ApplicationInfo
    :members:
.. doxygentypedef:: erbsland::core::CommandLineArguments
.. doxygentypedef:: erbsland::core::InitializeFn
.. doxygentypedef:: erbsland::core::MainFn
.. doxygenclass:: erbsland::cryptology::CryptologyConfiguration
    :members:
.. doxygenclass:: erbsland::cryptology::TlsConfiguration
    :members:
.. doxygenclass:: erbsland::cryptology::TlsConfigurationResolution
    :members:
.. doxygenclass:: erbsland::unit::ExitCode
    :members:
