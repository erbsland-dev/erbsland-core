..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    !single: Working with Paths
    single: Path
    single: fromPosix
    single: fromWindows
    single: path elements
    single: path suffixes
    single: path comparison
    single: path joining
    single: path slicing
    single: path conversion

.. _path-working-with-paths:

******************
Working with Paths
******************

This page explains the platform-independent features of
:cpp:class:`Path <erbsland::path::Path>`.
You will learn how to construct, inspect, compare, modify, join, slice, and convert paths without accessing the
filesystem.
These operations work entirely on the logical structure of a path, making them deterministic and independent of the
operating system.

Constructing Paths
==================

Create a :cpp:class:`Path <erbsland::path::Path>` directly when the path text originates from your own application and
should use the library's automatic format detection.
The constructor recognizes POSIX roots, Windows drive roots, and UNC roots, then stores the result in a normalized,
platform-independent representation.

When the external format is already known, use
:cpp:func:`fromPosix() <erbsland::path::Path::fromPosix>` or
:cpp:func:`fromWindows() <erbsland::path::Path::fromWindows>` instead.
This avoids ambiguities.
For example, ``c:/manifestos`` is interpreted as a relative POSIX path when parsed as POSIX text, but as an absolute
Windows path when parsed as Windows text.

Invalid or structurally malformed input results in an empty path.
If invalid input should instead report an error, use the corresponding throwing functions such as
:cpp:func:`fromPosixOrThrow() <erbsland::path::Path::fromPosixOrThrow>`,
:cpp:func:`fromWindowsOrThrow() <erbsland::path::Path::fromWindowsOrThrow>`,
or
:cpp:func:`fromNativeOrThrow() <erbsland::path::Path::fromNativeOrThrow>`.

.. erbsland-demo::
    :source: path/WorkingWithPaths/ConstructionAndFormats.cpp
    :exec: path/working_with_paths --demo ConstructionAndFormats
    :sha256: 17c49b40471d42bdb90fe215cfddad030838858372679b9a88203b4b20c8975a

.. code-block:: cpp

    /// Construct paths from generic, POSIX, and Windows text.
    ///
    /// `Path` stores platform-independent path data. Generic construction recognizes common root forms and normalizes
    /// separators to slash characters. Use `fromPosix()` or `fromWindows()` when text comes from a known external format
    /// and must not be interpreted through the generic auto-detection rules.
    void constructionAndFormats() {
        // Generic construction normalizes repeated separators.
        const auto logBook = el::Path{"expedicao//ceu/diario.txt"_el};
        el::io::printLine("generic ...........: "_el, logBook.toString());

        // POSIX parsing treats a drive-looking prefix as ordinary relative text.
        const auto posixManifest = el::Path::fromPosix("c:/manifestos/vento.txt"_el);
        el::io::printLine("posix .............: "_el, posixManifest.toString());
        el::io::printLine("relative ..........: "_el, el::BooleanFormat::yesNo(), posixManifest.isRelative());

        // Windows parsing accepts backslashes and normalizes drive letters.
        const auto windowsChart = el::Path::fromWindows("C:\\Expedicao\\rotas\\manha.txt"_el);
        el::io::printLine("windows ...........: "_el, windowsChart.toString());

        // UNC roots keep the share name as written, while the server name is normalized.
        const auto sharedChart = el::Path::fromWindows("\\\\PORTO-CEU\\Mapas\\norte.txt"_el);
        el::io::printLine("unc root ..........: "_el, sharedChart.root());
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mgeneric ...........: expedicao/ceu/diario.txt
        posix .............: c:/manifestos/vento.txt
        relative ..........: yes
        windows ...........: c:/Expedicao/rotas/manha.txt
        unc root ..........: //porto-ceu/Mapas/␛[0m

.. erbsland-demo-end::

Inspecting Paths
================

Before working with a path, you often need to know whether it names a location relative to another directory or starts
at a root.
The :cpp:func:`isRelative() <erbsland::path::Path::isRelative>`,
:cpp:func:`isAbsolute() <erbsland::path::Path::isAbsolute>`, and
:cpp:func:`isRoot() <erbsland::path::Path::isRoot>` functions answer these questions from the path itself.

Absolute paths expose their root as the first public path element.
For example, the path ``/arquivo/rotas.txt`` consists of the root ``/`` followed by the elements ``arquivo`` and
``rotas.txt``.
Treating the root as part of the public element sequence keeps indexing and slicing explicit.
A slice that contains the first element remains absolute, while a slice that starts after the root becomes relative.

For a report or archive path, you may also need the file name or its extension.
The :cpp:func:`name() <erbsland::path::Path::name>`,
:cpp:func:`stem() <erbsland::path::Path::stem>`,
:cpp:func:`suffix() <erbsland::path::Path::suffix>`, and
:cpp:func:`suffixes() <erbsland::path::Path::suffixes>` functions inspect the final non-root element.
Leading dots are considered part of the file name rather than suffix separators, allowing hidden files to behave as
expected.

.. erbsland-demo::
    :source: path/WorkingWithPaths/InspectingPaths.cpp
    :exec: path/working_with_paths --demo InspectingPaths
    :sha256: 5e9f4abfff81db5d7e9de9090db296c351f191cc2e5121e561497991b5b9417e

.. code-block:: cpp

    /// Inspect roots, elements, names, suffixes, and parent paths.
    ///
    /// A path can be queried without touching the filesystem. Roots are part of the public element sequence, while names,
    /// stems, and suffixes operate on the final non-root element. This makes metadata-style path work deterministic and
    /// independent from the current operating system.
    void inspectingPaths() {
        const auto report = el::Path{"/arquivo/rotas/relatorio.final.txt"_el};

        // Basic structure and naming information.
        el::io::printLine("path ..............: "_el, report.toString());
        el::io::printLine("root ..............: "_el, report.root());
        el::io::printLine("name ..............: "_el, report.name());
        el::io::printLine("stem ..............: "_el, report.stem());
        el::io::printLine("suffixes ..........: "_el, report.suffixes());

        // Public elements include the root for absolute paths.
        for (auto index = el::ItemIndex{}; index.isWithin(report.elementCount()); ++index) {
            el::io::printLine("element "_el, index.toSizeT(), " .........: "_el, report.element(index));
        }

        // Parent paths are ordered from nearest to furthest.
        for (const auto &parent : report.parents()) {
            el::io::printLine("parent ............: "_el, parent.toString());
        }
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mpath ..............: /arquivo/rotas/relatorio.final.txt
        root ..............: /
        name ..............: relatorio.final.txt
        stem ..............: relatorio
        suffixes ..........: .final.txt
        element 0 .........: /
        element 1 .........: arquivo
        element 2 .........: rotas
        element 3 .........: relatorio.final.txt
        parent ............: /arquivo/rotas
        parent ............: /arquivo
        parent ............: /␛[0m

.. erbsland-demo-end::

Comparing Paths
===============

When you collect report paths or check for duplicate entries in a file list, you can compare
:cpp:class:`Path <erbsland::path::Path>` values directly with ``==`` and the ordering operators.
Like inspection, comparison works on the root and path elements without accessing the filesystem.
The files do not need to exist, and you do not need to allocate strings by converting the paths to text first.
The :cpp:func:`compare() <erbsland::path::Path::compare>` method provides the same ordering as a
``std::strong_ordering`` result.

Sorting paths follows their hierarchy: comparison starts with the roots, then proceeds through the elements until it
finds a difference.
A parent sorts before its descendants, so ``rotas`` comes before ``rotas/manha.txt``.
That child also comes before ``rotas-antigas``, because the first element ``rotas`` sorts before ``rotas-antigas``.
Sorting the text returned by :cpp:func:`toString() <erbsland::path::Path::toString>` would give a different order,
because the separators would participate in the string comparison.
Empty paths sort first, followed by relative paths and then absolute paths.

Within each root or element, comparison uses case-sensitive Unicode code-point order.
If your application's file list should ignore case, you can pass
:cpp:func:`Char::compareCaseFolded() <erbsland::text::Char::compareCaseFolded>` to ``compare()``.
This changes the character matching rule while keeping the same hierarchy.
The :doc:`/topics/text_strings/comparing_strings` topic explains the available character comparison rules.

Path equality answers whether the path values match; identifying an existing file requires filesystem information.
For example, ``rotas/./manha.txt`` and ``rotas/manha.txt`` compare unequal because comparison preserves the ``.``
element.
Links or filesystem-specific case rules may also let different paths refer to the same file.
Case folding alone cannot reproduce those rules.
See :doc:`overview` for the filesystem operations that complement these path-value operations.

Editing Names
=============

The name-editing methods return a new :cpp:class:`Path <erbsland::path::Path>` while leaving the original object
unchanged.

Use
:cpp:func:`withName() <erbsland::path::Path::withName>` to replace the final
path element.
Use
:cpp:func:`withStem() <erbsland::path::Path::withStem>` when only the base name
changes while the suffixes remain unchanged.
Use
:cpp:func:`withSuffix() <erbsland::path::Path::withSuffix>` to replace or remove
the suffixes without rebuilding the complete path.

These functions are particularly useful when deriving related files such as reports, archives, generated documentation,
or companion resources from a common base path.

.. erbsland-demo::
    :source: path/WorkingWithPaths/EditingPathNames.cpp
    :exec: path/working_with_paths --demo EditingPathNames
    :sha256: b876beaef8d7e5334918d9c86c44493e16367abbffce26c2bf35ad5a08e7f233

.. code-block:: cpp

    /// Edit the final path element without rebuilding the whole path string.
    ///
    /// `withName()`, `withSuffix()`, and `withStem()` return modified paths and leave the original path unchanged. They
    /// are useful when an application derives companion files, report formats, or archive names from a common base path.
    void editingPathNames() {
        const auto baseReport = el::Path{"expedicao/diario/vento.final.txt"_el};

        // Create related names from one base report.
        const auto publicReport = baseReport.withName("relatorio.txt"_el);
        const auto markdownReport = baseReport.withSuffix("md"_el);
        const auto archiveReport = baseReport.withStem("arquivo-vento"_el);
        const auto plainName = baseReport.withSuffix({}); // remove all suffixes

        el::io::printLine("base ..............: "_el, baseReport.toString());
        el::io::printLine("name ..............: "_el, publicReport.toString());
        el::io::printLine("suffix ............: "_el, markdownReport.toString());
        el::io::printLine("stem ..............: "_el, archiveReport.toString());
        el::io::printLine("no suffix .........: "_el, plainName.toString());
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mbase ..............: expedicao/diario/vento.final.txt
        name ..............: expedicao/diario/relatorio.txt
        suffix ............: expedicao/diario/vento.md
        stem ..............: expedicao/diario/arquivo-vento.final.txt
        no suffix .........: expedicao/diario/vento␛[0m

.. erbsland-demo-end::

Joining and Slicing
===================

Build larger paths by combining smaller path fragments with
:cpp:func:`join() <erbsland::path::Path::join>`,
:cpp:func:`joined() <erbsland::path::Path::joined>`, or the ``/`` operator.
``join()`` modifies the path, while ``joined()`` and ``/`` return a new path.

String arguments use generic path parsing, where relative text treats both slashes and backslashes as separators.
For text obtained from the native filesystem, use :cpp:func:`joinNative() <erbsland::path::Path::joinNative>` or
:cpp:func:`joinedNative() <erbsland::path::Path::joinedNative>` instead.
These methods use the current platform's path syntax, preserving a backslash inside a filename on POSIX systems.

When the right-hand path is absolute, only its non-root elements are appended.
This allows reusable path fragments to be combined safely regardless of whether they were originally parsed as absolute
or relative paths.

Use
:cpp:func:`slice() <erbsland::path::Path::slice>` to extract a range of public
path elements.
The resulting path preserves its absolute nature only when the slice begins with the root element.
Otherwise, the result becomes a relative path.

Because slicing operates on the same public element sequence exposed by the inspection functions, it behaves predictably
across all supported path formats.

.. erbsland-demo::
    :source: path/WorkingWithPaths/JoiningAndSlicing.cpp
    :exec: path/working_with_paths --demo JoiningAndSlicing
    :sha256: e6024a781857eaaf1811b39ee81a0cf81fde961f655a9e526e1ddcdcb3bcc920

.. code-block:: cpp

    /// Join paths and extract element ranges.
    ///
    /// Joining appends the non-root elements of the right-hand path, even when that path is absolute. Slicing works on the
    /// public element sequence. If the slice starts with the root element, the result remains absolute; otherwise it
    /// becomes a relative path.
    void joiningAndSlicing() {
        const auto archive = el::Path{"/arquivo/expedicoes"_el};
        const auto morningLog = archive / "ceu-leste"_el / "manha.txt"_el;
        const auto borrowedAbsolute = archive / el::Path{"/rotas/noite.txt"_el};

        // Build paths from reusable fragments.
        el::io::printLine("joined ............: "_el, morningLog.toString());
        el::io::printLine("absolute rhs ......: "_el, borrowedAbsolute.toString());

        // Slice with and without the root element.
        const auto absoluteSlice = morningLog.slice(el::ItemRange{el::ItemIndex{0}, el::ItemCount{3}});
        const auto relativeSlice = morningLog.slice(el::ItemRange{el::ItemIndex{1}, el::ItemCount{2}});

        el::io::printLine("absolute slice ....: "_el, absoluteSlice.toString());
        el::io::printLine("relative slice ....: "_el, relativeSlice.toString());
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mjoined ............: /arquivo/expedicoes/ceu-leste/manha.txt
        absolute rhs ......: /arquivo/expedicoes/rotas/noite.txt
        absolute slice ....: /arquivo/expedicoes
        relative slice ....: arquivo/expedicoes␛[0m

.. erbsland-demo-end::

Converting Paths
================

Use
:cpp:func:`toString() <erbsland::path::Path::toString>` whenever you need a
platform-independent textual representation.
It always uses forward slashes and is therefore well suited for logging, configuration files, serialization, and
diagnostics.

Use
:cpp:func:`toPosix() <erbsland::path::Path::toPosix>` or
:cpp:func:`toWindows() <erbsland::path::Path::toWindows>` only when an
external interface explicitly requires one of these formats.
Not every absolute path can be represented in every external syntax.
For example, an absolute Windows path cannot be converted into a POSIX path, and an absolute POSIX path cannot be
converted into a Windows path.

Use
:cpp:func:`toStdPath() <erbsland::path::Path::toStdPath>` when interoperating
with APIs that require ``std::filesystem::path``.
The path manipulation facilities of this library do not depend on the standard filesystem library, so conversion is only
necessary at API boundaries.

.. erbsland-demo::
    :source: path/WorkingWithPaths/ConvertingPaths.cpp
    :exec: path/working_with_paths --demo ConvertingPaths
    :sha256: f2804bab9242a14bd8ef033fd092c5a40a4a78139dd2ef149dd542c50088e982

.. code-block:: cpp

    /// Convert paths back to display, POSIX, Windows, and standard-library forms.
    ///
    /// `toString()` is the preferred platform-independent display form. `toPosixPath()` and `toWindowsPath()` are for
    /// explicit external formats, while `toStdPath()` is reserved for interoperability with code that expects a standard
    /// library path object.
    void convertingPaths() {
        const auto relative = el::Path{"expedicao/rotas/manha.txt"_el};
        const auto windows = el::Path::fromWindows("C:\\Expedicao\\rotas\\manha.txt"_el);
        const auto posix = el::Path::fromPosix("/arquivo/rotas/manha.txt"_el);

        // Relative paths can be emitted in either text format.
        el::io::printLine("display ...........: "_el, relative.toString());
        el::io::printLine("posix   ...........: "_el, relative.toPosix());
        el::io::printLine("windows ...........: "_el, relative.toWindows(el::PathWindowsFormat::Native));

        // Absolute Windows and POSIX roots stay in their own external format.
        el::io::printLine("drive   ...........: "_el, windows.toWindows(el::PathWindowsFormat::Native));
        el::io::printLine("posix / ...........: "_el, posix.toPosix());

        // Use std::filesystem interop only when another API requires it.
        const auto stdPath = relative.toStdPath();
        el::io::printLine("std ...............: "_el, el::StringConverter{stdPath.generic_string()}.toString());
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mdisplay ...........: expedicao/rotas/manha.txt
        posix   ...........: expedicao/rotas/manha.txt
        windows ...........: expedicao\rotas\manha.txt
        drive   ...........: c:\Expedicao\rotas\manha.txt
        posix / ...........: /arquivo/rotas/manha.txt
        std ...............: expedicao/rotas/manha.txt␛[0m

.. erbsland-demo-end::
    
