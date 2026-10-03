..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    !single: TLS Configuration
    single: ELCL; TLS Configuration
    single: TLS Profile
    single: Certificate; Trust Anchor
    single: TLS; Client Identity
    single: TLS; System Trust
    single: TLS; Key Password
    single: TLS; Client Authentication
    single: TLS; Configuration Reload

************************
Configuring TLS Profiles
************************

When your application opens a TLS connection, it needs to know which certificates to trust and whether to present an
identity of its own.
A server also needs a certificate and private key so that clients can authenticate it.
This page shows how to keep those choices in labeled profiles, load them from ELCL, and replace them together when your
application reloads its configuration.
You will also learn how to connect to private services and require client certificates for restricted services.

Client and Server Profiles
==========================

A profile groups the identity and trust settings for one connection role.
For a client, trust settings determine which servers it accepts; a client identity lets it authenticate to a server that
requests a certificate.
For a server, the identity is required, while trust settings matter when you ask clients to authenticate with
certificates.
These are independent choices: trusting a certificate authority does not give your application an identity.

Labels connect these profiles to the networking APIs.
HTTP clients and servers both request the label ``http`` by default, and lower-level TLS connections request ``tls``.
Each lookup selects the client or server part for that label.
You can therefore configure outgoing HTTPS requests and an incoming HTTPS service under ``http`` without sharing their
identities or trust decisions.

Loading an HTTPS Server Profile
===============================

For an HTTPS service, start with a certificate chain and its matching PKCS#8 private key.
The chain file starts with the server certificate, followed by its intermediate certificates.
Place this server entry in your application's ELCL configuration:

.. code-block:: elcl

    *[TLS . Server]*
    Label: "http"
    Certificate Chain File: "certificates/server.pem"
    Signing Key File: "keys/server.pem"

The section name makes this a server profile.
The paths are relative to the configuration file, so you can deploy the configuration, certificates, and keys together.
When an included ELCL file defines a path, that path starts beside the included file.
For configurations parsed from text or a stream, provide absolute paths because those sources have no base directory.

Load the section during application startup, before enabling HTTPS.
In this excerpt, ``configurationPath`` identifies your ELCL file:

.. code-block:: cpp

    using namespace el::text::literals;
    auto parser = el::conf::Parser{};
    auto document = parser.parseFileOrThrow(configurationPath);
    auto profiles = el::cryptology::TlsConfigurationParser{}.parse(document->value("tls"_el));
    el::core::application().cryptologyConfiguration().replaceTlsConfigurations(std::move(profiles));

:cpp:class:`TlsConfigurationParser <erbsland::cryptology::TlsConfigurationParser>` checks the configuration and loads the certificate and key files, including checking that the
key matches the leaf certificate.
The returned :cpp:class:`TlsConfigurationSet <erbsland::cryptology::TlsConfigurationSet>` owns the profiles and their
shared settings.
:cpp:func:`CryptologyConfiguration::replaceTlsConfigurations() <erbsland::cryptology::CryptologyConfiguration::replaceTlsConfigurations>` installs them together.
You can then enable HTTPS as shown in :doc:`/topics/network/using-http-servers`.

This document configures only the server role.
Outgoing HTTPS requests can still use the built-in client default, which trusts the system's certificate authorities
unless you change the shared trust settings.
You do not need to add an empty client entry for ordinary HTTPS requests.

Choosing Trust for a Private Service
====================================

System trust follows the certificate authorities maintained for the machine or user.
macOS and Windows evaluate certificates through their native trust APIs; Linux uses the distribution's CA bundle.
This suits clients connecting to services whose certificates are already trusted by that deployment.

For a private payments service, you may instead want to accept only certificates issued by your own authority.
Add a client profile with an explicit trust-anchor file:

.. code-block:: elcl

    *[TLS . Client]*
    Label: "http/payments"
    Trust Anchors File: "payments/root-ca.pem"
    Intermediate Certificates File: "payments/issuing-cas.pem"

Select ``http/payments`` in the HTTP client's TLS options when connecting to that service.
The anchor file determines which certificates you trust.
The optional intermediate file helps build a path to those anchors, but its certificates do not establish trust on their
own.
PEM files can hold certificate bundles; a DER file holds one certificate.

Providing anchors selects explicit trust for this profile, even if other clients use system trust.
An explicit ``System Trust`` field overrides the inherited choice, but ``System Trust: true`` cannot be combined with an
anchor file in the same profile.
Keeping the choice in the profile makes the payments connection independent of the application's shared default.

Shared Trust Settings
---------------------

The optional ``[TLS]`` section holds settings that apply across profiles.
For example, an application that should trust only explicitly configured authorities can set:

.. code-block:: elcl

    [TLS]
    System Trust: false

Every client profile then needs its own anchors or an explicit ``System Trust: true`` selection.
An unconfigured client fails because it has no trust source; this setting never turns certificate verification off.
A server needs a trust source when it requests client certificates.

``Trust Allow Network`` controls whether native trust evaluation may retrieve intermediate certificates and revocation
information, and defaults to ``true``.
If your deployment needs a particular Linux CA bundle, ``Trust Bundle File`` supplies its path.
That override belongs to Linux deployments; a system-trust policy rejects it on other platforms when evaluated.
Explicit-anchor validation uses local certificates and performs no online revocation checks.
System trust follows the platform's revocation behavior, while Linux system trust uses local anchors.

Labels and Profile Selection
============================

Hierarchical labels let you give a group of connections a common profile and specialize individual services.
For example, an HTTP client requesting ``http/internal/admin`` looks for that client profile first, then
``http/internal``, then ``http``.
An entry containing only a server profile is skipped during this search.
Labels use lowercase ASCII letters, digits, interior hyphens, and slash-separated segments.

Lookup selects one complete role profile.
If you add ``http/internal/admin``, include all of its trust and identity settings; missing fields are not copied from
``http/internal``.
This keeps a service's trust decision in one place.
A selected profile with unusable settings fails instead of trying a broader profile.

If no named profile matches, lookup reaches the built-in default at the empty label.
For clients, this uses system trust subject to the shared settings described above.
The server default has no identity, so you must register a named server profile before accepting TLS connections.
The empty label is reserved; customize ``http``, ``tls``, or a service-specific label instead.

Presenting a Client Identity
============================

Some private services require the client to present a certificate as well as authenticate the server.
For the payments service, extend the earlier client profile with an identity:

.. code-block:: elcl

    *[TLS . Client]*
    Label: "http/payments"
    Trust Anchors File: "payments/root-ca.pem"
    Intermediate Certificates File: "payments/issuing-cas.pem"
    Certificate Chain File: "payments/client-chain.pem"
    Signing Key File: "payments/client-key.pem"

This replaces the earlier entry rather than adding a second client entry with the same label.
The trust anchors still authenticate the payments server; the certificate chain and key identify your client to it.
Both roles use the same identity fields, with the leaf certificate first in the chain.
The loader checks that the matching key supports TLS 1.3 signing.

The client presents its configured identity only after authenticating the server and receiving a suitable certificate
request.
If the identity does not meet that request, the client sends an empty certificate response.
A server that requires client authentication will reject that response, so provision a certificate that the service
accepts before opening the connection.

Native Identities
-----------------

On macOS and Windows, you can keep a private key with its native provider and select its identity through
``Identity Fingerprint``.
Obtain the fingerprint from :cpp:func:`SystemIdentityStore::list() <erbsland::cryptology::SystemIdentityStore::list>`
and supply it as a 32-byte SHA-256 ELCL bytes value.
The fingerprint replaces the certificate, key, and password fields and selects that exact identity when the profile is
loaded.
File identities are also available on these platforms and provide the portable option for Linux.

Native providers may need permission to access a private key.
The shared ``[TLS]`` setting ``Identity Allow Interaction`` defaults to ``false`` so an unattended service does not wait
for a prompt.
Provision access before starting the service, or enable interaction for an application where a user can respond.
This permission stays with the loaded identity; reload it when changing the setting.

Requiring Client Certificates
=============================

A restricted administrative service can require every client to present a certificate issued by a dedicated client CA.
Add the authentication requirement and client trust anchors to that service's server profile:

.. code-block:: elcl

    *[TLS . Server]*
    Label: "http/admin"
    Certificate Chain File: "admin/server-chain.pem"
    Signing Key File: "admin/server-key.pem"
    Client Authentication: "required"
    Trust Anchors File: "admin/client-ca.pem"

The server presents its own chain and verifies each client's certificate against ``admin/client-ca.pem``.
Select ``http/admin`` in the server's TLS options, or map an SNI host name to it when several services share a listener.
SNI selects the complete server profile, so each host can have its own identity and client-authentication requirement.
See :doc:`/topics/network/using-tls-server-connections` for profile selection during acceptance.

With ``required``, a client without an acceptable certificate cannot complete the handshake.
For a service that also admits anonymous clients, ``optional`` requests a certificate but accepts an empty response.
It still rejects an invalid certificate when one is presented.
The default, ``disabled``, does not request a client certificate.

A successful certificate exchange proves that the client holds a key certified by an authority you trust.
Your application still needs to associate that identity with an account and decide which operations it may perform.
The TLS server topic explains how to inspect the authenticated path after the handshake; the
:doc:`HTTP server topic </topics/network/using-http-servers>` shows where that information is available to request
handlers.

Retrieving Key Passwords from Secure Storage
============================================

An encrypted PKCS#8 key needs a password when the profile is loaded.
The ``Signing Key Password`` field supplies that password.
On macOS and Windows, a secure-storage placeholder lets you keep it in Keychain or Credential Manager while the
configuration contains only the item name:

.. code-block:: elcl

    *[TLS . Client]*
    Label: "http/payments"
    Trust Anchors File: "payments/root-ca.pem"
    Intermediate Certificates File: "payments/issuing-cas.pem"
    Certificate Chain File: "payments/client-chain.pem"
    Signing Key File: "payments/client-key.pem"
    Signing Key Password: "${secure:payments-key}"

Before parsing this document, register a
:cpp:class:`SecureStorageSource <erbsland::text::placeholder::SecureStorageSource>` with the configuration parser.
For the earlier loading example, replace the parser construction with this setup:

.. code-block:: cpp

    auto parser = el::conf::Parser{};
    parser.addPlaceholderSource(std::make_shared<el::text::placeholder::SecureStorageSource>(
        el::cryptology::SecureStorage{"payments-service"_el}));

The storage namespace ``payments-service`` must match the namespace used when provisioning the ``payments-key`` item.
Registering the source does not read a secret; resolving the placeholder during parsing does.
Retrieval is synchronous, so load this configuration during startup or on a worker instead of a responsive event loop.
By default, storage access does not permit authorization dialogs.

Passwords keep their exact text, including whitespace and an existing empty value.
A missing item or failed retrieval is an error, rather than an empty password.
The returned text uses sensitive storage, which helps erase library-owned copies when they are released.
The TLS validation rules also mark the password fields as secret.
For your own configuration fields, add secret annotations to their validation rules: a placeholder alone does not set
:cpp:func:`Value::isSecret() <erbsland::conf::Value::isSecret>`.
See :doc:`/topics/security/about_sensitive_strings_and_byte_blocks` for the scope of memory cleanup.

Reloading Profiles Together
===========================

For a configuration reload, parse the new document into a separate
:cpp:class:`TlsConfigurationSet <erbsland::cryptology::TlsConfigurationSet>` and install it with the same replacement
call used at startup.
Loading files, resolving passwords, and accessing native identities are synchronous operations, so perform that work on
a worker when the application is already serving requests.
If loading or replacement fails, report the error and keep serving with the installed configuration.
:cpp:class:`ConfError <erbsland::conf::ConfError>` retains the responsible source location and any underlying loading error to help diagnose the problem.

Replacement publishes the new shared settings and named profiles together.
A connection that has already selected its configuration keeps that identity and effective trust policy; later
connections use the replacement.
This lets you rotate certificates without changing an active connection's authentication decision.
The replacement is the complete desired set, so include every profile you want to keep.
A missing TLS section yields the default set and removes previously registered named profiles when installed.
