..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Configuration; Source Resolving
    single: Includes; Source Resolver
    single: FileSourceResolver
    single: SourceResolver

*******************************
Resolving Configuration Sources
*******************************

An ``@include`` command names more configuration input, but the ELCL language does not assume that the name must be a
local file.
A :cpp:class:`SourceResolver <erbsland::conf::SourceResolver>` translates the command's text into one or more
:cpp:class:`Source <erbsland::conf::Source>` objects for the parser.
The default :cpp:class:`FileSourceResolver <erbsland::conf::FileSourceResolver>` provides relative and absolute file
paths, protocol-prefixed paths, and deterministic wildcard expansion.

Most applications only need to restrict the default resolver to the path forms they accept.
A custom resolver is useful when include descriptors refer to application presets, package resources, a database, or
another source model that is not expressed as a file path.

From Include Text to Parsed Content
===================================

When the parser reaches an include such as:

.. code-block:: elcl

    @include: "parts/*.elcl"

it passes two pieces of context to the resolver: the raw text ``parts/*.elcl`` and the source identifier of the document
that contains the command.
The file resolver uses that identifier to interpret a relative path beside the including file.
A custom resolver can use it to choose a namespace, resolve a relative application resource, or reject a descriptor that
is not meaningful from that source.

The resolver returns a list because a single descriptor may expand to several sources.
The parser reads those sources in list order at the position of the ``@include`` command.
An empty list therefore means that the descriptor legitimately selected nothing, while a null list is an error.
If the descriptor itself is invalid, the resolver should throw ``ConfError`` with the ``Syntax`` category; the parser
adds the include command's location to that error.

Resolution selects candidate sources.
The parser's access check separately decides whether those sources may be read in the current include chain.
Keeping those responsibilities separate allows a resolver to focus on names and lookup while one access policy covers
both the built-in and custom resolver.

Use the Default File Resolver
=============================

The parser creates a ``FileSourceResolver`` by default, so ordinary file includes need no setup.
An exact relative path is resolved from the directory of the including file.
A filename wildcard can select a batch of regular files, and a ``**`` directory element can extend that search into
subdirectories.

.. erbsland-demo::
    :source: conf/SourceResolving/DefaultFileResolver.cpp
    :exec: conf/source_resolving --demo DefaultFileResolver
    :source-sha256: f816909ae784273b360e418f2cc652c3f87ee056ed51aa798f51b873a0ce7783

.. code-block:: cpp

    /// Resolve relative file includes and deterministic wildcard batches with the parser defaults.
    ///
    /// `FileSourceResolver` interprets an include relative to the file that contains it. A filename wildcard can expand to
    /// several regular files; the resolver sorts them before the parser processes their contents.
    void defaultFileResolver() {
        auto temporaryOptions = el::PathTempDirectoryOptions{};
        temporaryOptions.setPrefix("levantamento-"_el).setRandomLength(el::CpLength{8U});
        const auto temporary =
            el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow(temporaryOptions);
        const auto root = temporary->path();
        auto writeOptions = el::PathWriteTextOptions{};
        writeOptions.setCreateParents(true);

        (root / "parts/01-forest.elcl"_el)
            .content()
            .writeTextOrThrow(
                "[survey]\n"
                "forest: \"Parque Nacional da Tijuca\"\n"_el,
                writeOptions);
        (root / "parts/02-team.elcl"_el)
            .content()
            .writeTextOrThrow(
                "[team]\n"
                "observers: 8\n"_el,
                writeOptions);
        const auto mainPath = root / "survey.elcl"_el;
        mainPath.content().writeTextOrThrow("@include: \"parts/*.elcl\"\n"_el);

        // The default resolver expands the wildcard relative to survey.elcl.
        const auto document = el::conf::Parser{}.parseFileOrThrow(mainPath);
        el::io::printLine("Forest: "_el, document->getTextOrThrow("survey.forest"_el));
        el::io::printLine("Observers: "_el, document->getIntegerOrThrow("team.observers"_el));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Forest: Parque Nacional da Tijuca
    Observers: 8

.. erbsland-demo-end::

Wildcard results are sorted by resolved path before parsing.
This makes a numbered layout such as ``01-base.elcl``, ``02-local.elcl`` predictable, although every included path must
still contribute a valid part of the final ELCL document.
An exact path must name a regular file; a wildcard that has no matches returns an empty list.

The supported file forms are:

.. list-table:: Optional ``FileSourceResolver`` Features
    :header-rows: 1
    :widths: 28 72

    * - Feature
      - Accepted form
    * - ``FilenameWildcard``
      - One ``*`` in the filename, such as ``parts/*.elcl`` or ``node-*.elcl``.
    * - ``RecursiveWildcard``
      - One ``**`` directory element, such as ``profiles/**/*.elcl``.
    * - ``AbsolutePaths``
      - A platform-native absolute include path.
    * - ``WindowsUNCPath``
      - A Windows UNC path written with slash or backslash separators.
    * - ``FileProtocol``
      - The optional ``file:`` prefix, as in ``file:defaults.elcl``.

All five features are enabled on a newly created resolver.
Path separators are normalized, but wildcard syntax is intentionally small: a filename accepts at most one ``*``, and
``**`` must occupy a complete directory element.
The access check remains the authority for whether a resolved path lies inside the allowed area.

Restrict Accepted File Syntax
=============================

Applications often have a simpler include contract than the complete default feature set.
For example, a service may accept only exact paths relative to its main configuration file.
Create a shared ``FileSourceResolver``, disable every form the contract does not need, and install it with
:cpp:func:`Parser::setSourceResolver() <erbsland::conf::Parser::setSourceResolver>` before parsing.

.. erbsland-demo::
    :source: conf/SourceResolving/ConfigureFileResolver.cpp
    :exec: conf/source_resolving --demo ConfigureFileResolver
    :source-sha256: 370abf5304aaaf385033d4d768f75ce6e2c64a55c7c36d3a886185f0bd538cce

.. code-block:: cpp

    /// Restrict file-include syntax before installing a `FileSourceResolver` on a parser.
    ///
    /// Every optional path feature is enabled by default. Disable forms that an application does not need, then pass the
    /// configured shared resolver to `Parser::setSourceResolver()`. Exact relative paths continue to work with all optional
    /// features disabled.
    void configureFileResolver() {
        auto temporaryOptions = el::PathTempDirectoryOptions{};
        temporaryOptions.setPrefix("levantamento-"_el).setRandomLength(el::CpLength{8U});
        const auto temporary =
            el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow(temporaryOptions);
        const auto root = temporary->path();
        (root / "defaults.elcl"_el)
            .content()
            .writeTextOrThrow(
                "[survey]\n"
                "region: \"Mata Atlântica\"\n"_el);
        const auto mainPath = root / "survey.elcl"_el;
        mainPath.content().writeTextOrThrow("@include: \"defaults.elcl\"\n"_el);

        // Accept only exact relative paths without protocol prefixes or wildcards.
        const auto resolver = el::conf::FileSourceResolver::create();
        resolver->disable(el::conf::FileSourceResolver::RecursiveWildcard);
        resolver->disable(el::conf::FileSourceResolver::FilenameWildcard);
        resolver->disable(el::conf::FileSourceResolver::AbsolutePaths);
        resolver->disable(el::conf::FileSourceResolver::WindowsUNCPath);
        resolver->disable(el::conf::FileSourceResolver::FileProtocol);

        auto parser = el::conf::Parser{};
        parser.setSourceResolver(resolver);
        const auto document = parser.parseFileOrThrow(mainPath);
        el::io::printLine("Region: "_el, document->getTextOrThrow("survey.region"_el));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Region: Mata Atlântica

.. erbsland-demo-end::

:cpp:func:`FileSourceResolver::enable() <erbsland::conf::FileSourceResolver::enable>` and
:cpp:func:`FileSourceResolver::disable() <erbsland::conf::FileSourceResolver::disable>` change one feature at a time;
:cpp:func:`FileSourceResolver::isEnabled() <erbsland::conf::FileSourceResolver::isEnabled>` is useful when a larger
configuration assembles policy from several components.
An unsupported feature in include text produces a syntax error rather than silently changing its meaning.

These feature switches limit syntax, not filesystem reach.
Configure the parser's access check when the application needs to constrain parent directories, roots, suffixes, or
other authorization rules.
Setting the source resolver to ``nullptr`` disables ``@include`` entirely.

Write an Application-Specific Resolver
======================================

A custom resolver implements one method:
:cpp:func:`SourceResolver::resolve() <erbsland::conf::SourceResolver::resolve>`.
The following resolver recognizes a single application-owned descriptor and maps it to a file beneath a controlled
directory.
The included document does not learn the physical filename, and the main configuration cannot request an arbitrary path
through this resolver.

.. erbsland-demo::
    :source: conf/SourceResolving/CustomResolver.cpp
    :exec: conf/source_resolving --demo CustomResolver
    :source-sha256: 299cd0985036e29499de6c92b19a2c596b2cc2deff722cc09bdb07137df40f44

.. code-block:: cpp

    /// Resolve an application-owned include scheme to files beneath a controlled directory.
    ///
    /// A custom resolver receives the raw include text and the identifier of the document containing it. It must either
    /// return a list of closed sources in parsing order or throw `ConfError`. Returning file sources preserves useful,
    /// stable identifiers for include-loop detection and diagnostics.
    class PresetSourceResolver final : public el::conf::SourceResolver {
    public:
        explicit PresetSourceResolver(el::Path root) : _root{std::move(root)} {}

    public: // implement `SourceResolver`
        auto resolve(const el::conf::SourceResolverContext &context) -> el::conf::SourceListPtr override {
            if (context.includeText != "preset:forest"_el) {
                throw el::conf::ConfError{
                    el::conf::ConfErrorCategory::Syntax, "Unknown application configuration preset."_el};
            }
            auto sources = std::make_shared<el::conf::SourceList>();
            sources->push_back(el::conf::Source::fromFile(_root / "forest.elcl"_el));
            return sources;
        }

    private:
        el::Path _root;
    };

    /// Install a custom source resolver for application-specific include descriptors.
    void customResolver() {
        auto temporaryOptions = el::PathTempDirectoryOptions{};
        temporaryOptions.setPrefix("levantamento-"_el).setRandomLength(el::CpLength{8U});
        const auto temporary =
            el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow(temporaryOptions);
        const auto root = temporary->path();
        (root / "forest.elcl"_el)
            .content()
            .writeTextOrThrow(
                "[survey]\n"
                "forest: \"Floresta Nacional de Ipanema\"\n"_el);
        const auto mainPath = root / "survey.elcl"_el;
        mainPath.content().writeTextOrThrow("@include: \"preset:forest\"\n"_el);

        auto parser = el::conf::Parser{};
        parser.setSourceResolver(std::make_shared<PresetSourceResolver>(root));
        const auto document = parser.parseFileOrThrow(mainPath);
        el::io::printLine("Forest: "_el, document->getTextOrThrow("survey.forest"_el));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Forest: Floresta Nacional de Ipanema

.. erbsland-demo-end::

Return sources in the order in which their document fragments should appear.
Each source must be closed; the parser opens, reads, and closes it at the appropriate time.
Give every source a stable, meaningful :cpp:class:`SourceIdentifier <erbsland::conf::SourceIdentifier>`.
The parser uses identifiers in locations and diagnostics and compares them while detecting recursive include loops.
``Source::fromFile()`` supplies a canonical file identifier automatically, which is why the example can return a file
source without implementing another ``Source`` class.

The resolver should reject malformed or unknown descriptors itself instead of returning null.
It should also keep lookup work bounded: limit wildcard-like expansion, avoid network requests without an application
timeout, and never return a source list whose ordering depends on an unstable backend traversal.
The parser applies its own nesting and source-count limits, but resolver-specific limits make errors clearer and stop
expensive work earlier.

Extend or Replace File Resolving
================================

Use a configured ``FileSourceResolver`` when the include text still represents files.
Its feature switches preserve path normalization, wildcard limits, deterministic ordering, and platform handling.

When an application needs both file paths and another descriptor family, a small routing resolver can own a
``FileSourceResolver`` and delegate file-shaped requests to it.
Composition keeps the two formats distinct and lets the application apply extra checks before delegation.
Replace file resolving completely when every descriptor belongs to an application protocol and accepting a path would be
misleading or unnecessarily broad.

Whichever design you choose, treat resolution and authorization as two layers.
The next topic describes how the parser's access check controls which resolved sources may join a document.
