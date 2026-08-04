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
    single: path joining
    single: path slicing
    single: path conversion

.. _path-working-with-paths:

******************
Working with Paths
******************

This page explains the platform-independent features of
:cpp:class:`Path <erbsland::path::Path>`.
You will learn how to construct, inspect, modify, join, slice, and convert paths without accessing the filesystem.
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
    :source-sha256: 3e496fc5a26f646b0d996cce9186dc0382ff7e6a6494b83ec910c62c04da8bca

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

.. erbsland-ansi::
    :escape-char: ␛

    generic ...........: expedicao/ceu/diario.txt
    posix .............: c:/manifestos/vento.txt
    relative ..........: yes
    windows ...........: c:/Expedicao/rotas/manha.txt
    unc root ..........: //porto-ceu/Mapas/

.. erbsland-demo-end::

Inspecting Paths
================

Many path operations only require information about the path itself and do not need to access the filesystem.
The :cpp:class:`Path <erbsland::path::Path>` class therefore provides a rich set of inspection functions that operate
purely on its internal representation.

Use
:cpp:func:`isRelative() <erbsland::path::Path::isRelative>`,
:cpp:func:`isAbsolute() <erbsland::path::Path::isAbsolute>`, and
:cpp:func:`isRoot() <erbsland::path::Path::isRoot>` to classify a path.

Absolute paths expose their root as the first public path element.
For example, the path ``/arquivo/rotas.txt`` consists of the root ``/`` followed by the elements ``arquivo`` and
``rotas.txt``.
Treating the root as part of the public element sequence keeps indexing and slicing explicit.
A slice that contains the first element remains absolute, while a slice that starts after the root becomes relative.

Use
:cpp:func:`name() <erbsland::path::Path::name>`,
:cpp:func:`stem() <erbsland::path::Path::stem>`,
:cpp:func:`suffix() <erbsland::path::Path::suffix>`, and
:cpp:func:`suffixes() <erbsland::path::Path::suffixes>` to inspect the final
non-root element.
Leading dots are considered part of the file name rather than suffix separators, allowing hidden files to behave as
expected.

.. erbsland-demo::
    :source: path/WorkingWithPaths/InspectingPaths.cpp
    :exec: path/working_with_paths --demo InspectingPaths
    :source-sha256: e218177859f87e13102bf8d1a02ce2ab1608dae4910800eb27a7bb372f25b703

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

.. erbsland-ansi::
    :escape-char: ␛

    path ..............: /arquivo/rotas/relatorio.final.txt
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
    parent ............: /

.. erbsland-demo-end::

Editing Names
=============

Paths are immutable.
Operations that modify a path always return a new
:cpp:class:`Path <erbsland::path::Path>` while leaving the original object
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
    :source-sha256: e1bdeca81da7cc224769353e13764b28ea2d5b01019839bd8974a3072085d356

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

.. erbsland-ansi::
    :escape-char: ␛

    base ..............: expedicao/diario/vento.final.txt
    name ..............: expedicao/diario/relatorio.txt
    suffix ............: expedicao/diario/vento.md
    stem ..............: expedicao/diario/arquivo-vento.final.txt
    no suffix .........: expedicao/diario/vento

.. erbsland-demo-end::

Joining and Slicing
===================

Build larger paths by combining smaller path fragments with
:cpp:func:`join() <erbsland::path::Path::join>`,
:cpp:func:`joined() <erbsland::path::Path::joined>`, or the ``/`` operator.

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
    :source-sha256: f70a1d00bf03bf3cde211d103092f52b68a6c516522cb726229ca2bbd025fedf

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

.. erbsland-ansi::
    :escape-char: ␛

    joined ............: /arquivo/expedicoes/ceu-leste/manha.txt
    absolute rhs ......: /arquivo/expedicoes/rotas/noite.txt
    absolute slice ....: /arquivo/expedicoes
    relative slice ....: arquivo/expedicoes

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
    :source-sha256: c656f53e8aaef20efedd1a5c7aff6772f0de66ee8eec2f6908d06ff87b7dfab1

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
        el::io::printLine("std ...............: "_el, el::String{stdPath.generic_string()});
    }

.. erbsland-ansi::
    :escape-char: ␛

    display ...........: expedicao/rotas/manha.txt
    posix   ...........: expedicao/rotas/manha.txt
    windows ...........: expedicao\rotas\manha.txt
    drive   ...........: c:\Expedicao\rotas\manha.txt
    posix / ...........: /arquivo/rotas/manha.txt
    std ...............: expedicao/rotas/manha.txt

.. erbsland-demo-end::
    
