..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Options; Option Values
    single: OptionValues
    single: OptionValue

.. _options-option-values:

*************
Option Values
*************

:cpp:class:`OptionValues <erbsland::options::OptionValues>` is the parsed result of a successful command-line parse.
This page explains how values are assigned, how aliases work, and how application code should read flags, counts, lists,
defaults, and selected module names.

How the Parser Fills the Result
===============================

After the parser has selected the active root and module sets, it assigns command-line arguments to the matching
definitions.
For each option, every accepted name and lookup alias maps to the same
:cpp:class:`OptionValue <erbsland::options::OptionValue>` instance.

For a definition such as ``{"-v", "--verbose", "verbose"}``, all three names refer to the same parsed value.
Application code normally reads the dashless alias, because it is stable even if the command-line spelling changes.

Defaults from option definitions are inserted when the user omits a value.
Those default values are present in :cpp:class:`OptionValues <erbsland::options::OptionValues>` just like user-supplied
values, except that they do not have a source argument index.

.. erbsland-demo::
    :source: option/OptionValues/main.cpp
    :exec: option/option_values
    :source-sha256: c0f53642c6de072472b75c2cfe3af7bbc706940583866cc4ae6a0199d7fef1d8

.. code-block:: cpp

    /// `OptionValues` stores parsed values under every accepted name of an option.
    ///
    /// Command line spellings and dashless lookup aliases point to the same `OptionValue` instance.
    /// Typed getters return flags, flag counts, integers, text, and lists without forcing application code to inspect the
    /// underlying variant directly.
    auto optionValues() -> el::ExitCode {
        auto options = el::Options::create();
        auto info = el::ApplicationInfo{};
        info.setApplicationName("Laboratory Values"_el);
        info.setApplicationVersion(el::Version{0, 8, 0});
        options->setApplicationInfo(info);
        options->addOption({"-v"_el, "--verbose"_el, "verbose"_el}).setHelpDescription("Increases the detail level."_el);
        options->addOption({"-p"_el, "--point"_el, "point"_el})
            .setType(el::OptionType::Text)
            .setMaximum(el::ArgumentCount{4U})
            .setHelpDescription("Measurement point. Can appear up to four times."_el);
        options->addOption({"--level"_el, "level"_el})
            .setType(el::OptionType::Integer)
            .setDefaultValue(el::OptionInteger{2})
            .setHelpDescription("Default observation level."_el);
        options->addOption("instrument"_el).setRequired().setHelpDescription("Primary instrument."_el);

        auto manager = el::OptionManager{options};
        const auto values = manager.parseOrThrow(
            makeArgs({"valores"_el, "-vv"_el, "--point"_el, "entrada"_el, "-p"_el, "lente"_el, "microscopio"_el}));

        const auto verboseByLongName = values->value("--verbose"_el);
        const auto verboseByAlias = values->value("verbose"_el);
        const auto points = values->getTextList("point"_el);
        el::io::printLine("same object: "_el, el::BooleanFormat::yesNo(), verboseByLongName == verboseByAlias);
        el::io::printLine("verbose count: "_el, values->getFlagCount("verbose"_el));
        el::io::printLine("instrument: "_el, values->getText("instrument"_el));
        el::io::printLine("level: "_el, values->getInteger("level"_el));
        el::io::printLine("points: "_el, points.join(", "_el));
        el::io::printLine("first index: "_el, values->value("point"_el)->argumentIndex().toSizeT());
        return el::ExitCode::success();
    }

.. erbsland-ansi::
    :escape-char: ␛

    same object: yes
    verbose count: 2
    instrument: microscopio
    level: 2
    points: entrada, lente
    first index: 3

.. erbsland-demo-end::

Use Typed Getters for Normal Code
=================================

The typed getters are the easiest and safest way to read parsed values:

* :cpp:func:`getFlag() <erbsland::options::OptionValues::getFlag>` reads the stored boolean value.
* :cpp:func:`getFlagCount() <erbsland::options::OptionValues::getFlagCount>` reads how often a flag appeared.
* :cpp:func:`getInteger() <erbsland::options::OptionValues::getInteger>` reads an integer.
* :cpp:func:`getText() <erbsland::options::OptionValues::getText>` reads text, choice, or marked sensitive text.
* :cpp:func:`getIntegerList() <erbsland::options::OptionValues::getIntegerList>` reads repeated integers.
* :cpp:func:`getTextList() <erbsland::options::OptionValues::getTextList>` reads repeated text values.

Each getter accepts a fallback value.
Use static defaults in the option definition when the default is part of the command-line contract and should appear in
help.
Use getter defaults when the fallback is local to one code path or depends on runtime state.

Flags, Lists, and Module Names
==============================

Repeated flags are stored as a count.
This makes ``-vv`` and ``-v -v`` useful for verbosity levels without creating extra options.
An explicit false value is still an occurrence: ``getFlag()`` returns ``false``, while ``getFlagCount()`` returns one
and the corresponding :cpp:class:`OptionValue <erbsland::options::OptionValue>` retains its source argument index.
This distinction lets code tell an omitted flag or configured default from an explicit user choice.

Repeated text or integer values are stored as lists.
The parser preserves the order in which the values were assigned to that option.

If modules are used, :cpp:func:`moduleName() <erbsland::options::OptionValues::moduleName>` returns the selected module
name.
Applications that do not use module main functions can dispatch manually with this value.

.. erbsland-demo::
    :source: option/OptionValueAccess/main.cpp
    :exec: option/option_value_access
    :source-sha256: 628c48c4e9f0b336ffcd6611ac8124b5be83f11f7942482b1052444fe23bb0f9

.. code-block:: cpp

    /// `OptionValues` is the parsed result map produced by `OptionManager`.
    ///
    /// Every command-line spelling and lookup alias for an option points to the same `OptionValue` object.
    /// Typed getters read flags, flag counts, integers, text, and repeated value lists.
    /// Defaults can come from the option definition or from the getter call, and module-aware tools can inspect
    /// `moduleName()` when dispatching manually.
    auto optionValueAccess() -> el::ExitCode {
        auto manager = el::OptionManager{createValueOptions()};
        const auto values = manager.parseOrThrow(
            makeArgs({"night-values"_el, "count"_el, "-vv"_el, "--route"_el, "forest-edge"_el, "-r"_el, "pond"_el, "dune"_el}));

        const auto routes = values->getTextList("route"_el);
        auto routeList = el::StringEditorList{};
        for (const auto &route : routes) {
            routeList.append(route.copy());
        }

        el::io::printLine("module: "_el, values->moduleName());
        el::io::printLine("area: "_el, values->getText("area"_el));
        el::io::printLine("verbose count: "_el, values->getFlagCount("verbose"_el));
        el::io::printLine("routes: "_el, routeList.join(", "_el));
        el::io::printLine("level: "_el, values->getInteger("level"_el));
        el::io::printLine("profile: "_el, values->getText("profile"_el, "night"_el));
        el::io::printLine("route argument index: "_el, values->value("route"_el)->argumentIndex().toSizeT());
        return el::ExitCode::success();
    }

.. erbsland-ansi::
    :escape-char: ␛

    module: count
    area: dune
    verbose count: 2
    routes: forest-edge, pond
    level: 2
    profile: night
    route argument index: 4

.. erbsland-demo-end::

Inspect Raw OptionValue Objects
===============================

Use :cpp:func:`value() <erbsland::options::OptionValues::value>` when application code needs details beyond the typed
getter result.
The returned :cpp:class:`OptionValue <erbsland::options::OptionValue>` stores the source option, the concrete storage
variant, and the argument indexes that produced the value.

Raw values are useful for diagnostics, telemetry, or tooling that needs to explain where a setting came from.
Most application logic should still prefer typed getters, because they express the expected value kind directly.
