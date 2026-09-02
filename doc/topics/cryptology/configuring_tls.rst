..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    !single: TLS Configuration
    single: ELCL; TLS Configuration
    single: TLS Profile
    single: Certificate; Trust Anchor

************************
Configuring TLS Profiles
************************

Many applications need several independent TLS profiles: a server identity for an incoming HTTPS connection, trust
anchors for an outgoing service, and perhaps a combined profile for a peer-to-peer protocol.
This page shows how to describe these profiles as an ELCL section list, validate each entry, and register the resulting
configuration with the application.

Reading a Profile List
======================

Each ``*[tls]*`` entry is one complete profile.
The label selects its place in the application registry; the remaining fields describe the client and server material
that belongs to this profile.

.. code-block:: elcl

    *[tls]*
    Label: "http/server"
    Server Certificate Chain File: "certificates/server.pem"
    Server Signing Key File: "keys/server-key.pem"

    *[tls]*
    Label: "http/client"
    Trust Anchors File: "certificates/roots.pem"
    Intermediate Certificates File: "certificates/intermediates.pem"

ELCL name normalization also accepts spellings such as ``server_signing_key_file``.
The displayed names are useful when configuration files are primarily maintained by people.

:cpp:class:`TlsConfigurationParser <erbsland::cryptology::TlsConfigurationParser>` handles one section-list entry at a
time.
This keeps duplicate-label policy and registration order in your application code.

.. code-block:: cpp

    using namespace el::text::literals;

    auto document = el::conf::Parser{}.parseFileOrThrow(configurationPath);
    const auto tlsSections = document->valueOrThrow("tls"_el);

    for (const auto &section : *tlsSections) {
        auto entry = el::cryptology::TlsConfigurationParser{}.parse(section);
        el::core::application().cryptologyConfiguration().setTlsConfiguration(
            entry.label(), entry.takeConfiguration());
    }

Parsing has no application-wide side effects.
The returned :cpp:class:`TlsConfigurationEntry <erbsland::cryptology::TlsConfigurationEntry>` owns both the validated
label and a complete :cpp:class:`TlsConfiguration <erbsland::cryptology::TlsConfiguration>`.
You can inspect the configuration before transferring it into a registry.

Choosing Client and Server Material
===================================

A client-only profile needs explicit trust anchors.
An intermediate bundle is optional, but it is meaningful only together with the anchors that establish trust.
PEM bundles may contain several certificates, while a DER file contains one certificate.

.. code-block:: elcl

    *[tls]*
    Label: "payments/client"
    Trust Anchors File: "payments/root-ca.pem"
    Intermediate Certificates File: "payments/issuing-cas.pem"

A server identity always consists of both a leaf-first certificate chain and its signing key.
The key file contains an unencrypted PKCS#8 key.
During parsing, the identity verifies that the key matches the leaf certificate and supports a TLS 1.3 signature scheme.

.. code-block:: elcl

    *[tls]*
    Label: "admin/server"
    Server Certificate Chain File: "admin/server-chain.pem"
    Server Signing Key File: "admin/server-key.pem"

One profile can serve both roles by configuring both sets of fields.

.. code-block:: elcl

    *[tls]*
    Label: "cluster/peer"
    Trust Anchors File: "cluster/root-ca.der"
    Server Certificate Chain File: "cluster/peer-chain.pem"
    Server Signing Key File: "cluster/peer-key.p8"

The file readers detect PEM and DER from their contents.
The suffixes in these examples help people identify the format but do not control parsing.

Fallbacks and Empty Overrides
=============================

TLS labels use lowercase ASCII letters, digits, interior hyphens, and slash-delimited segments.
Connection lookup first tries the complete label and then removes trailing segments.
An empty label is the global fallback.

.. code-block:: elcl

    *[tls]*
    Label: ""
    Trust Anchors File: "company/root-ca.pem"

    *[tls]*
    Label: "http/client/untrusted"

The second entry is intentionally empty.
Because registry lookup selects one complete profile instead of merging fields, it prevents ``http/client/untrusted``
from inheriting the global trust anchors.

Resolving Files from Their Defining Source
==========================================

A relative material path starts in the directory of the ELCL file that defines that field.
This remains true when an included file contributes an entry.
For example, if ``profiles/internal.elcl`` contains this profile, both files are resolved below ``profiles``.

.. code-block:: elcl

    *[tls]*
    Label: "internal/server"
    Server Certificate Chain File: "certificates/server.pem"
    Server Signing Key File: "keys/server-key.pem"

Absolute material paths are accepted as written.
A relative path from an in-memory text or stream source is rejected because it has no stable directory.
The resulting :cpp:class:`ConfError <erbsland::conf::ConfError>` points to the responsible field and retains the file
reader or cryptology error as its cause.

Versioning and Material Sources
===============================

The schema names the source kind directly: ``Trust Anchors File`` and ``Server Signing Key File`` select filesystem
sources.
Distinct source names allow later platform stores and hardware security modules to have their own validation and
parameters without changing the meaning of existing files.

Applications that validate configuration before applying it can use the same compiled rules and format version as the
parser.

.. code-block:: cpp

    const auto &rules = el::cryptology::TlsConfigurationParser::validationRules();
    rules->validate(section, el::cryptology::TlsConfigurationParser::version());

The parser repeats this validation in
:cpp:func:`TlsConfigurationParser::parse() <erbsland::cryptology::TlsConfigurationParser::parse>`, so callers that parse
immediately do not need a separate validation pass.
