..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Configuration; Access Checks
    single: Configuration; File Access
    single: Security; Configuration Sources

*******************************************
Controlling Access to Configuration Sources
*******************************************

An include directive turns text from a configuration file into a request for another source.
Resolving that request and trusting its result are deliberately separate decisions.
This page explains how the parser checks every source before opening it, how to configure the built-in filesystem
boundary, and when an application should extend or replace that policy.

Every Source Crosses the Access Boundary
========================================

The :cpp:class:`AccessCheck <erbsland::conf::AccessCheck>` sits between source resolution and source I/O.
The parser calls its ``check()`` method for the initial document and for every source returned by an include resolver.
A rejected source is therefore never opened by the parser.

Each call receives an :cpp:struct:`AccessSources <erbsland::conf::AccessSources>` value with three identifiers:

``source``
    The source the parser is about to open.

``parent``
    The document whose ``@include`` directive requested this source, or a null pointer for the initial document.

``root``
    The initial document of the complete parse operation.

This context lets a policy distinguish the entry point from an include and evaluate a request against either its direct
parent or the original trust root.
The identifiers are lightweight descriptions; an access check should inspect their source names and paths without
opening the sources itself.

The Default File Boundary
=========================

Every new :cpp:class:`Parser <erbsland::conf::Parser>` uses a
:cpp:class:`FileAccessCheck <erbsland::conf::FileAccessCheck>`.
Its default policy accepts file includes beside their parent document and below that directory, while rejecting paths
that escape to a parent or unrelated directory.
Before comparing paths, it resolves them physically, so ``..`` elements and symbolic links cannot disguise where an
included file is located.

The default also rejects included files larger than 100 MB.
It does not require an ``.elcl`` suffix, and it leaves non-file sources to an application-specific policy unless you
enable the file-only restriction.

Configuring File Access
=======================

Create a shared file check with
:cpp:func:`FileAccessCheck::create() <erbsland::conf::FileAccessCheck::create>`, change only the features required by
your application, and install it with
:cpp:func:`Parser::setAccessCheck() <erbsland::conf::Parser::setAccessCheck>` before parsing.
The available :cpp:enum:`FileAccessCheck::Feature <erbsland::conf::FileAccessCheck::Feature>` values are:

.. list-table:: File access features
    :header-rows: 1
    :widths: 24 12 64

    * - Feature
      - Default
      - Effect
    * - ``SameDirectory``
      - Enabled
      - Accepts an included file in the directory of the document that includes it.
    * - ``Subdirectories``
      - Enabled
      - Accepts included files below the including document's directory.
    * - ``AnyDirectory``
      - Disabled
      - Removes the directory-containment restriction. Size and suffix checks still apply when enabled.
    * - ``OnlyFileSources``
      - Disabled
      - Rejects every source whose source name is not ``file``.
    * - ``LimitSize``
      - Enabled
      - Rejects an included file larger than 100 MB.
    * - ``RequireSuffix``
      - Disabled
      - Requires included file paths to end in ``.elcl``, compared without ASCII case sensitivity.

At least one directory feature must permit an included file.
Disabling ``SameDirectory`` and ``Subdirectories`` without enabling ``AnyDirectory`` blocks all file includes.
Enabling ``AnyDirectory`` is appropriate only when another boundary, such as an exact allowlist, already controls the
paths.

The next demo keeps the safe defaults, adds the suffix requirement, and layers a small application rule on top.
The generated theme uses Polish station names as configuration data; the policy itself remains independent of that data.

.. erbsland-demo::
    :source: conf/AccessChecks/ConfigureFileAccess.cpp
    :exec: conf/access_checks --demo ConfigureFileAccess
    :source-sha256: 1b241b92d6c346da43c619111ee91b629a8d03784fb09282fa8fa9588dac5172

.. code-block:: cpp

    /// Add an application rule to the configurable file access policy.
    ///
    /// `FileAccessCheck` provides the filesystem boundary, size limit, and optional suffix check. A derived check can call
    /// the base implementation first and then enforce a rule that is specific to the application. The configured shared
    /// instance is installed with `Parser::setAccessCheck()` before parsing starts.
    class StationFileAccessCheck final : public el::conf::FileAccessCheck {
    public: // implement `AccessCheck`
        auto check(const el::conf::AccessSources &sources) -> el::conf::AccessCheckResult override {
            const auto result = FileAccessCheck::check(sources);
            if (sources.parent != nullptr && el::Path{sources.source->path()}.name().startsWith("draft-"_el)) {
                throw el::conf::ConfError{
                    el::conf::ConfErrorCategory::Access,
                    "Draft station data cannot be included in an operational configuration."_el,
                    el::Path{sources.source->path()}};
            }
            return result;
        }
    };

    /// Configure and install a file access check before parsing a document.
    void configureFileAccess() {
        auto temporaryOptions = el::PathTempDirectoryOptions{};
        temporaryOptions.setPrefix("stacja-badawcza-"_el).setRandomLength(el::CpLength{8U});
        const auto temporary =
            el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow(temporaryOptions);
        const auto root = temporary->path();
        (root / "station.elcl"_el)
            .content()
            .writeTextOrThrow(
                "[station]\n"
                "name: \"Stacja Polarna Aurora\"\n"_el);
        const auto mainPath = root / "main.elcl"_el;
        mainPath.content().writeTextOrThrow("@include: \"station.elcl\"\n"_el);

        // Keep the safe directory and size defaults, and require the usual ELCL suffix as well.
        const auto accessCheck = std::make_shared<StationFileAccessCheck>();
        accessCheck->enable(el::conf::FileAccessCheck::RequireSuffix);

        auto parser = el::conf::Parser{};
        parser.setAccessCheck(accessCheck);
        const auto document = parser.parseFileOrThrow(mainPath);
        el::io::printLine("Station: "_el, document->getTextOrThrow("station.name"_el));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Station: Stacja Polarna Aurora

.. erbsland-demo-end::

Extending the File Policy
=========================

Deriving from :cpp:class:`FileAccessCheck <erbsland::conf::FileAccessCheck>` is the best fit when local files still form
the underlying trust boundary.
Call the base ``check()`` first, as ``StationFileAccessCheck`` does in the demo, and apply the application rule only
after the standard containment, size, and suffix checks have succeeded.
This preserves the less visible protections around canonical paths and filesystem errors.

Throwing :cpp:class:`ConfError <erbsland::conf::ConfError>` with the ``Access`` category gives the caller a useful
reason for the denial.
Returning ``AccessCheckResult::Denied`` is suitable when the generic ``Access denied to source`` message is sufficient.
In both cases, the parser adds the location of the include directive to the error.

Writing a Complete Access Policy
================================

A policy should implement :cpp:class:`AccessCheck <erbsland::conf::AccessCheck>` directly when the application's trust
model is not based on directory containment.
Typical examples are an exact file allowlist, signed resource identifiers, or sources served by an application-owned
repository.

The following implementation accepts only a fixed set of physical file paths.
It validates both the source protocol and the parser-provided context, canonicalizes its allowlist once, and checks the
initial document as well as its include.

.. erbsland-demo::
    :source: conf/AccessChecks/CustomAccessCheck.cpp
    :exec: conf/access_checks --demo CustomAccessCheck
    :source-sha256: 925bb12938f77812653ac1a1623274c352393fe7987001f726af0ebdc253a824

.. code-block:: cpp

    /// Restrict parsing to an exact list of approved configuration files.
    ///
    /// A fully custom `AccessCheck` is useful when an application already has a complete source policy. The parser calls
    /// `check()` for the root and every included source before opening it. This implementation canonicalizes file paths and
    /// grants access only when the requested path is present in its allowlist.
    class ApprovedStationSources final : public el::conf::AccessCheck {
    public:
        explicit ApprovedStationSources(const el::PathList &approvedPaths) {
            for (const auto &path : approvedPaths) {
                _approvedPaths += path.resolveOrThrow(el::PathResolveMode::Physical);
            }
        }

    public: // implement `AccessCheck`
        auto check(const el::conf::AccessSources &sources) -> el::conf::AccessCheckResult override {
            if (sources.source == nullptr || sources.root == nullptr || sources.source->name() != "file"_el) {
                return el::conf::AccessCheckResult::Denied;
            }
            auto requestedPath = el::Path{sources.source->path()};
            try {
                requestedPath = requestedPath.resolveOrThrow(el::PathResolveMode::Physical);
            } catch (const el::PathError &) {
                throw el::conf::ConfError{
                    el::conf::ConfErrorCategory::Access,
                    "Configuration Source Access Denied"_el,
                    "The configuration source path cannot be resolved."_el,
                    requestedPath,
                    std::current_exception()};
            }
            for (const auto &approvedPath : _approvedPaths) {
                if (requestedPath == approvedPath) {
                    return el::conf::AccessCheckResult::Granted;
                }
            }
            throw el::conf::ConfError{
                el::conf::ConfErrorCategory::Access,
                "The configuration source is not in the station allowlist."_el,
                requestedPath};
        }

    private:
        el::PathList _approvedPaths;
    };

    /// Install a custom access check that implements an application-owned trust boundary.
    void customAccessCheck() {
        auto temporaryOptions = el::PathTempDirectoryOptions{};
        temporaryOptions.setPrefix("stacja-badawcza-"_el).setRandomLength(el::CpLength{8U});
        const auto temporary =
            el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow(temporaryOptions);
        const auto root = temporary->path();
        const auto stationPath = root / "station.elcl"_el;
        stationPath.content().writeTextOrThrow(
            "[station]\n"
            "name: \"Obserwatorium Północne\"\n"_el);
        const auto mainPath = root / "main.elcl"_el;
        mainPath.content().writeTextOrThrow("@include: \"station.elcl\"\n"_el);

        const auto approvedPaths = el::PathList{mainPath, stationPath};

        auto parser = el::conf::Parser{};
        parser.setAccessCheck(std::make_shared<ApprovedStationSources>(approvedPaths));
        const auto document = parser.parseFileOrThrow(mainPath);
        el::io::printLine("Approved station: "_el, document->getTextOrThrow("station.name"_el));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Approved station: Obserwatorium Północne

.. erbsland-demo-end::

For a non-file source, use the same pattern with stable logical identifiers supplied by the matching source resolver.
Treat ``source``, ``parent``, and ``root`` as untrusted input: check for null identifiers where the contract permits
them, recognize only source names your policy understands, and make every default branch deny access.
Avoid reproducing filesystem containment with text-prefix comparisons, because path separators, symbolic links, case
rules, and parent elements make those comparisons unreliable.

Disabling Includes
==================

An application that accepts only self-contained documents can pass a null source resolver to
:cpp:func:`Parser::setSourceResolver() <erbsland::conf::Parser::setSourceResolver>`.
This disables ``@include`` processing altogether and leaves the access check responsible only for the initial source.
Applications that enable source resolution should always keep an explicit access boundary in place.
