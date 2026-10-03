.. index::
    single: Logging; Line Format
    single: Log Lines; Patterns
    single: Log Lines; Stream Names
    single: Log Lines; Truncation

********************
Formatting Log Lines
********************

A useful log line gives its reader enough context to understand an event without making the event itself disappear in
decoration.
Interactive tools may need little more than the message, while a long-running service usually benefits from a timestamp,
severity, and stream name on every entry.
This page shows how to describe both styles with :cpp:class:`LogLineFormat <erbsland::log::LogLineFormat>`, and how to
keep long names and messages readable without confusing presentation limits with the manager's memory limits.

Setting the Default Log Line Format
===================================

A line format is a small value object that belongs to a complete
:cpp:class:`LogConfiguration <erbsland::log::LogConfiguration>`.
Construct it locally, change the settings that matter to your application, and pass it to
:cpp:func:`LogConfiguration::setLineFormat <erbsland::log::LogConfiguration::setLineFormat>`.
This selects the default for routes without a custom format.
Writers use this format unless you give a destination its own layout, as described later on this page.

The following demo uses a standalone manager so the complete sequence is visible.
The console writer is not part of line formatting; it is added because a manager needs a destination before the result
can be seen.
An :cpp:class:`Application <erbsland::core::Application>` follows the same configuration sequence with its
application-owned manager.

.. erbsland-demo::
    :source: log/LoggingTopics/LineFormats.cpp
    :exec: log/logging_topics --demo LineFormats
    :sha256: dfdd477ef757e0bb8cac945a50575886ed4e22a07ce5cc1d3e3b55fe78563229

.. code-block:: cpp

    /// Install one line format as part of a complete logging configuration.
    ///
    /// Create `LogLineFormat` as a value, customize it, and move it into `LogConfiguration`. The manager takes another
    /// complete configuration value and uses this default for routes without their own line-format override.
    void lineFormats() {
        auto lineFormat = el::LogLineFormat{};
        lineFormat.setPattern("{level} [{name}] {message}"_el);

        auto configuration = el::LogConfiguration{};
        configuration.setLineFormat(std::move(lineFormat))
            .addWriter(el::LogWriter::createForConsole(el::application().terminal()));

        const auto manager = el::LogManager::create();
        manager->setConfiguration(std::move(configuration));
        const auto log = manager->createStream("guild/routes"_el);
        log->info("The route catalog is ready."_el);
        manager->shutdown();
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0m␛[97;49mINF [␛[95;49mguild/routes␛[97;49m] The route catalog is ready.␛[0m

.. erbsland-demo-end::


Line Patterns and Field Placeholders
====================================

The pattern determines which pieces of an entry appear and how they are arranged.
A new :cpp:class:`LogLineFormat <erbsland::log::LogLineFormat>` starts with ``{time} {level} - {message}``.
Call :cpp:func:`LogLineFormat::setPattern <erbsland::log::LogLineFormat::setPattern>` when another layout better suits
your readers.
The pattern is not a general-purpose formatting language; it deliberately has four semantic placeholders:

.. list-table:: Available line-pattern placeholders
    :header-rows: 1

    * - Placeholder
      - Inserted value
    * - ``{time}``
      - The entry timestamp, rendered in the selected time zone.
    * - ``{level}``
      - The entry severity, rendered in the selected level format.
    * - ``{name}``
      - The hierarchical stream path, rendered in the selected name format. It is empty for the root stream.
    * - ``{message}``
      - The prepared message text, including any rendering-time truncation.

Literal text may surround the placeholders, and a placeholder may be omitted or repeated.
Write ``{{`` or ``}}`` when the result needs a literal opening or closing brace.
The setter validates the pattern immediately; an empty pattern, an unknown placeholder, or an unmatched brace raises
:cpp:class:`ParameterFailure <erbsland::err::ParameterFailure>` while the configuration is being assembled.

The demo compares three patterns: message-only output for a compact command, level and stream context for a larger
application, and a labelled format containing literal braces.

.. erbsland-demo::
    :source: log/LoggingTopics/LinePatterns.cpp
    :exec: log/logging_topics --demo LinePatterns
    :sha256: f91e060b3b63a1aee39630e456ad12568d27138e8176b4e6e26b252809192b70

.. code-block:: cpp

    /// Arrange semantic log fields with a validated placeholder pattern.
    ///
    /// `setPattern()` accepts time, level, name, and message placeholders. A compact command can show only the message,
    /// while larger applications often add level and name. Doubled braces produce literal braces in the output.
    void linePatterns() {
        const auto showPattern = [](const el::String &label, el::String pattern) -> void {
            el::io::printLine(label, ":"_el);
            auto lineFormat = el::LogLineFormat{};
            lineFormat.setPattern(std::move(pattern));
            auto configuration = el::LogConfiguration{};
            configuration.setLineFormat(std::move(lineFormat))
                .addWriter(el::LogWriter::createForConsole(el::application().terminal()));

            const auto manager = el::LogManager::create();
            manager->setConfiguration(std::move(configuration));
            manager->createStream("guild/routes"_el)->info("Selected the ridge route."_el);
            manager->shutdown();
        };

        showPattern("Message only"_el, "{message}"_el);
        showPattern("Level and name"_el, "{level} [{name}] {message}"_el);
        showPattern("Literal braces"_el, "{{guild}} {level}: {message}"_el);
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mMessage only:
        ␛[97;49mSelected the ridge route.
        ␛[39;49mLevel and name:
        ␛[97;49mINF [␛[95;49mguild/routes␛[97;49m] Selected the ridge route.
        ␛[39;49mLiteral braces:
        ␛[97;49m{guild} INF: Selected the ridge route.␛[0m

.. erbsland-demo-end::

UTC and Local Timestamp Formats
===============================

Log entries retain their timestamp in UTC, which makes entries from different machines and time zones straightforward to
compare.
Presentation is a separate choice.
:cpp:func:`LogLineFormat::setTimestampZone <erbsland::log::LogLineFormat::setTimestampZone>` accepts either
``LogTimestampZone::Utc`` or ``LogTimestampZone::Local`` and affects only text inserted for ``{time}``.

UTC is the default and ends its ISO timestamp with ``Z``.
Local rendering converts the same kind of retained instant through the process-local time zone and includes the
applicable numeric offset.
Local output is friendly in tools operated from one location; UTC is generally easier to correlate across services.
On a machine configured for UTC, both choices naturally display the same clock time.

.. erbsland-demo::
    :source: log/LoggingTopics/LineTimestampZones.cpp
    :exec: log/logging_topics --demo LineTimestampZones
    :sha256: 4f3e9b2d01cf47e54a539543845848a2cb2ebff7eb43b7a125f8055128c205b5

.. code-block:: cpp

    /// Render retained UTC timestamps either in UTC or in the process-local time zone.
    ///
    /// `setTimestampZone()` changes presentation only; the entry keeps its original UTC instant. ISO output includes `Z`
    /// for UTC or the applicable numeric offset for local time.
    void lineTimestampZones() {
        const auto showZone = [](const el::String &label, const el::LogTimestampZone zone) -> void {
            el::io::printLine(label, ":"_el);
            auto lineFormat = el::LogLineFormat{};
            lineFormat.setPattern("{time} - {message}"_el).setTimestampZone(zone);
            auto configuration = el::LogConfiguration{};
            configuration.setLineFormat(std::move(lineFormat))
                .addWriter(el::LogWriter::createForConsole(el::application().terminal()));

            const auto manager = el::LogManager::create();
            manager->setConfiguration(std::move(configuration));
            manager->rootStream()->info("Expedition clock synchronized."_el);
            manager->shutdown();
        };

        showZone("UTC rendering"_el, el::LogTimestampZone::Utc);
        showZone("Local rendering"_el, el::LogTimestampZone::Local);
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mUTC rendering:
        ␛[96;49m2026-11-06 10:32:15Z␛[97;49m - Expedition clock synchronized.
        ␛[39;49mLocal rendering:
        ␛[96;49m2026-11-06 11:32:15+01:00␛[97;49m - Expedition clock synchronized.␛[0m

.. erbsland-demo-end::

Severity Name Formats
=====================

A severity can be terse in a dense terminal view or deliberately conspicuous in a report.
:cpp:func:`LogLineFormat::setLevelFormat <erbsland::log::LogLineFormat::setLevelFormat>` controls the spelling inserted
for ``{level}``: ``ThreeLetterUpper`` produces values such as ``WRN`` and is the default, ``ShortLower`` produces
``wrn``, ``FullLower`` produces ``warning``, and ``FullUpper`` produces ``WARNING``.

Only the rendered spelling changes.
The entry remains a warning in all four cases, so writer routing and the console writer's semantic styling continue to
use the original severity.
This makes it safe to choose a format for readability without changing which entries reach a destination.

.. erbsland-demo::
    :source: log/LoggingTopics/LineLevelFormats.cpp
    :exec: log/logging_topics --demo LineLevelFormats
    :sha256: 1f2f932acce28d40de8a8a9f1e10470512ad4cfa012807c2d3fc4544b7b8b1ba

.. code-block:: cpp

    /// Choose a compact or descriptive spelling for severity levels.
    ///
    /// `setLevelFormat()` affects only the text inserted by `{level}`. The entry keeps the same warning level, so routing
    /// and console styling remain unchanged across all four representations.
    void lineLevelFormats() {
        const auto showLevel = [](const el::String &label, const el::LogLevelFormat levelFormat) -> void {
            el::io::printLine(label, ":"_el);
            auto lineFormat = el::LogLineFormat{};
            lineFormat.setPattern("{level}: {message}"_el).setLevelFormat(levelFormat);
            auto configuration = el::LogConfiguration{};
            configuration.setLineFormat(std::move(lineFormat))
                .addWriter(el::LogWriter::createForConsole(el::application().terminal()));

            const auto manager = el::LogManager::create();
            manager->setConfiguration(std::move(configuration));
            manager->rootStream()->warn("One route marker needs repainting."_el);
            manager->shutdown();
        };

        showLevel("Three uppercase letters"_el, el::LogLevelFormat::ThreeLetterUpper);
        showLevel("Three lowercase letters"_el, el::LogLevelFormat::ShortLower);
        showLevel("Full lowercase name"_el, el::LogLevelFormat::FullLower);
        showLevel("Full uppercase name"_el, el::LogLevelFormat::FullUpper);
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mThree uppercase letters:
        ␛[93;49mWRN: One route marker needs repainting.
        ␛[39;49mThree lowercase letters:
        ␛[93;49mwrn: One route marker needs repainting.
        ␛[39;49mFull lowercase name:
        ␛[93;49mwarning: One route marker needs repainting.
        ␛[39;49mFull uppercase name:
        ␛[93;49mWARNING: One route marker needs repainting.␛[0m

.. erbsland-demo-end::

Stream Name Formats
===================

Hierarchical stream paths are most valuable when they reveal where an entry originated, but the most useful portion
depends on the surrounding output.
Call :cpp:func:`LogLineFormat::setNameFormat <erbsland::log::LogLineFormat::setNameFormat>` to select the representation
inserted for ``{name}``.

``Full`` is the default and preserves the complete path.
``Leaf`` keeps only its final segment, which works well when an application has already separated output by subsystem.
``HeadAndLeaf`` combines the first and final segments with ``/…/``; it retains both the broad domain and the specific
producer while discarding intermediate organization.
``LeftTruncated`` preserves as much of the right side as fits within the name limit, where the most specific segments
are usually found.

The following output uses the same stream for every entry, making the information retained by each choice easy to
compare.

.. erbsland-demo::
    :source: log/LoggingTopics/LineNameFormats.cpp
    :exec: log/logging_topics --demo LineNameFormats
    :sha256: 919bacfbe2b2ab0d677bfe087597f77cac06697a670f9c3c14603025c53e9ac7

.. code-block:: cpp

    /// Select how much of a hierarchical stream path appears in `{name}`.
    ///
    /// `setNameFormat()` can preserve the full path, keep its leaf, combine its first and final segments, or retain the
    /// right side of a long path within the default 40-character name limit.
    void lineNameFormats() {
        const auto showName = [](const el::String &label, const el::LogNameFormat nameFormat) -> void {
            el::io::printLine(label, ":"_el);
            auto lineFormat = el::LogLineFormat{};
            lineFormat.setPattern("{name}: {message}"_el).setNameFormat(nameFormat);
            auto configuration = el::LogConfiguration{};
            configuration.setLineFormat(std::move(lineFormat))
                .addWriter(el::LogWriter::createForConsole(el::application().terminal()));

            const auto manager = el::LogManager::create();
            manager->setConfiguration(std::move(configuration));
            manager->createStream("guild/exploration/finland/lapland/northern-lights"_el)->info("Survey opened."_el);
            manager->shutdown();
        };

        showName("Full path"_el, el::LogNameFormat::Full);
        showName("Leaf segment"_el, el::LogNameFormat::Leaf);
        showName("First and final segments"_el, el::LogNameFormat::HeadAndLeaf);
        showName("Left-truncated path"_el, el::LogNameFormat::LeftTruncated);
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mFull path:
        ␛[95;49mguild/exploration/finland/lapland/northern-lights␛[97;49m: Survey opened.
        ␛[39;49mLeaf segment:
        ␛[95;49mnorthern-lights␛[97;49m: Survey opened.
        ␛[39;49mFirst and final segments:
        ␛[95;49mguild/…/northern-lights␛[97;49m: Survey opened.
        ␛[39;49mLeft-truncated path:
        ␛[95;49m…oration/finland/lapland/northern-lights␛[97;49m: Survey opened.␛[0m

.. erbsland-demo-end::

Left-Truncated Stream Name Limits
=================================

The ``LeftTruncated`` representation becomes especially useful when stream hierarchies can grow with plugins, jobs, or
other runtime data.
:cpp:func:`LogLineFormat::setNameLimit <erbsland::log::LogLineFormat::setNameLimit>` sets its maximum length in Unicode
code points.
The default is 40 code points.
The formatter keeps the right side and places the configured truncation mark at the beginning, so a reader can see both
that context was removed and which specific producer emitted the entry.

The name limit is consulted only when ``LogNameFormat::LeftTruncated`` is selected.
It does not silently shorten ``Full``, ``Leaf``, or ``HeadAndLeaf`` names.
A limit of zero disables shortening and therefore shows the complete path even in ``LeftTruncated`` mode.

.. erbsland-demo::
    :source: log/LoggingTopics/LineNameLimits.cpp
    :exec: log/logging_topics --demo LineNameLimits
    :sha256: bc18f0aa429e7c707c1cf527754a7743ee0a8ca0c49efef501e1c076671f1ed5

.. code-block:: cpp

    /// Bound left-truncated stream names by a code-point count.
    ///
    /// `setNameLimit()` is consulted only for `LogNameFormat::LeftTruncated`. Keeping the right side preserves the most
    /// specific path segments; a zero limit disables name shortening even when that format is selected.
    void lineNameLimits() {
        const auto showLimit = [](const el::String &label, const el::CpCount limit) -> void {
            el::io::printLine(label, ":"_el);
            auto lineFormat = el::LogLineFormat{};
            lineFormat.setPattern("{name}: {message}"_el)
                .setNameFormat(el::LogNameFormat::LeftTruncated)
                .setNameLimit(limit);
            auto configuration = el::LogConfiguration{};
            configuration.setLineFormat(std::move(lineFormat))
                .addWriter(el::LogWriter::createForConsole(el::application().terminal()));

            const auto manager = el::LogManager::create();
            manager->setConfiguration(std::move(configuration));
            manager->createStream("guild/exploration/finland/lapland/northern-lights"_el)->info("Survey opened."_el);
            manager->shutdown();
        };

        showLimit("32 characters"_el, el::CpCount{32U});
        showLimit("18 characters"_el, el::CpCount{18U});
        showLimit("Unlimited"_el, el::CpCount::zero());
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0m32 characters:
        ␛[95;49m…finland/lapland/northern-lights␛[97;49m: Survey opened.
        ␛[39;49m18 characters:
        ␛[95;49m…d/northern-lights␛[97;49m: Survey opened.
        ␛[39;49mUnlimited:
        ␛[95;49mguild/exploration/finland/lapland/northern-lights␛[97;49m: Survey opened.␛[0m

.. erbsland-demo-end::

Multiline Messages and First-Line Truncation
============================================

Multiline messages are useful for structured explanations and diagnostics, yet they can be awkward in destinations where
one physical line is expected to represent one event.
:cpp:func:`LogLineFormat::setMessageTruncation <erbsland::log::LogLineFormat::setMessageTruncation>` chooses how the
message is prepared before it is inserted into the pattern.

``LogMessageTruncation::None`` is the default and preserves the complete message, including line breaks.
``LogMessageTruncation::FirstLine`` stops at the first newline and appends the truncation mark when later lines were
removed.
This mode does not consult the numeric message limit: its boundary is the first line, not a character count.

.. erbsland-demo::
    :source: log/LoggingTopics/LineFirstLineTruncation.cpp
    :exec: log/logging_topics --demo LineFirstLineTruncation
    :sha256: b64bc882606877d434603879c5aa3eabffda26e419a5ebcfcbbe0bd86e1b854c

.. code-block:: cpp

    /// Choose whether multiline messages remain complete or stop after their first line.
    ///
    /// `LogMessageTruncation::None` is the default and preserves line breaks. `FirstLine` keeps the text before the first
    /// newline and appends the configured truncation mark without consulting the message limit.
    void lineFirstLineTruncation() {
        const auto showMessage = [](const el::String &label, const el::LogMessageTruncation truncation) -> void {
            el::io::printLine(label, ":"_el);
            auto lineFormat = el::LogLineFormat{};
            lineFormat.setPattern("{level}: {message}"_el).setMessageTruncation(truncation);
            auto configuration = el::LogConfiguration{};
            configuration.setLineFormat(std::move(lineFormat))
                .addWriter(el::LogWriter::createForConsole(el::application().terminal()));

            const auto manager = el::LogManager::create();
            manager->setConfiguration(std::move(configuration));
            manager->rootStream()->info("Cloud cover is increasing.\nThe ridge team will wait below the summit."_el);
            manager->shutdown();
        };

        showMessage("Complete message"_el, el::LogMessageTruncation::None);
        showMessage("First line only"_el, el::LogMessageTruncation::FirstLine);
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mComplete message:
        ␛[97;49mINF: Cloud cover is increasing.
        The ridge team will wait below the summit.
        ␛[39;49mFirst line only:
        ␛[97;49mINF: Cloud cover is increasing.…␛[0m

.. erbsland-demo-end::

Message Character Limits
========================

When the message itself must fit into a predictable space, select ``LogMessageTruncation::CharacterCount`` and set the
budget with :cpp:func:`LogLineFormat::setMessageLimit <erbsland::log::LogLineFormat::setMessageLimit>`.
The limit counts Unicode code points in the message, rather than encoded bytes.
Pattern literals, the level, the timestamp, and the stream name sit outside this particular budget.

The truncation mark occupies part of the available count, keeping the final message within the requested length.
A zero limit disables character-count shortening instead of producing an empty message.
This makes it convenient to expose ``0`` as an application setting meaning “unlimited.”

.. erbsland-demo::
    :source: log/LoggingTopics/LineMessageLimits.cpp
    :exec: log/logging_topics --demo LineMessageLimits
    :sha256: 9bd09be7f0fde19e2276c25e1913c6c3fdf8c08ecaf3fc13b1bc48c66b3bbebe

.. code-block:: cpp

    /// Limit the rendered message while leaving the other pattern fields outside the budget.
    ///
    /// Select `CharacterCount` with `setMessageTruncation()`, then pass the code-point budget to `setMessageLimit()`. A
    /// zero message limit disables this shortening rule.
    void lineMessageLimits() {
        const auto showLimit = [](const el::String &label, const el::CpCount limit) -> void {
            el::io::printLine(label, ":"_el);
            auto lineFormat = el::LogLineFormat{};
            lineFormat.setPattern("{level} [{name}] {message}"_el)
                .setMessageTruncation(el::LogMessageTruncation::CharacterCount)
                .setMessageLimit(limit);
            auto configuration = el::LogConfiguration{};
            configuration.setLineFormat(std::move(lineFormat))
                .addWriter(el::LogWriter::createForConsole(el::application().terminal()));

            const auto manager = el::LogManager::create();
            manager->setConfiguration(std::move(configuration));
            manager->createStream("guild/weather"_el)
                ->warn("A snow front will reach the northern platform before dusk."_el);
            manager->shutdown();
        };

        showLimit("48 message characters"_el, el::CpCount{48U});
        showLimit("24 message characters"_el, el::CpCount{24U});
        showLimit("Unlimited message"_el, el::CpCount::zero());
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0m48 message characters:
        ␛[93;49mWRN [␛[95;49mguild/weather␛[93;49m] A snow front will reach the northern platform b…
        ␛[39;49m24 message characters:
        ␛[93;49mWRN [␛[95;49mguild/weather␛[93;49m] A snow front will reach…
        ␛[39;49mUnlimited message:
        ␛[93;49mWRN [␛[95;49mguild/weather␛[93;49m] A snow front will reach the northern platform before dusk.␛[0m

.. erbsland-demo-end::

Total Rendered Line Limits
==========================

Sometimes the destination, rather than the message, imposes the useful boundary.
``LogMessageTruncation::TotalLineLength`` treats
:cpp:func:`LogLineFormat::setMessageLimit <erbsland::log::LogLineFormat::setMessageLimit>` as a code-point budget for the
complete rendered line.
The formatter first accounts for pattern literals and the rendered time, level, and name, then gives the remaining space
to ``{message}``.
A detailed prefix therefore leaves less room for the same message, as the demo illustrates.

If ``{message}`` occurs more than once, its remaining budget is shared between those occurrences.
If fixed fields already consume the complete budget, the message becomes empty; fixed fields themselves are never
shortened by this setting.
As with character-count mode, a zero limit disables the rule.

.. erbsland-demo::
    :source: log/LoggingTopics/LineTotalLimits.cpp
    :exec: log/logging_topics --demo LineTotalLimits
    :sha256: 75eb38068337b60348cbeff7f28f597cc133ec0a4681191d36750564a9fbc25a

.. code-block:: cpp

    /// Limit the complete rendered line by giving the message the space left by fixed fields.
    ///
    /// `TotalLineLength` counts pattern literals, the level, the rendered name, and every message occurrence against the
    /// `setMessageLimit()` budget. A richer prefix therefore leaves fewer characters for the message.
    void lineTotalLimits() {
        const auto showPattern = [](const el::String &label, el::String pattern) -> void {
            el::io::printLine(label, ":"_el);
            auto lineFormat = el::LogLineFormat{};
            lineFormat.setPattern(std::move(pattern))
                .setMessageTruncation(el::LogMessageTruncation::TotalLineLength)
                .setMessageLimit(el::CpCount{56U});
            auto configuration = el::LogConfiguration{};
            configuration.setLineFormat(std::move(lineFormat))
                .addWriter(el::LogWriter::createForConsole(el::application().terminal()));

            const auto manager = el::LogManager::create();
            manager->setConfiguration(std::move(configuration));
            manager->createStream("guild/weather"_el)
                ->warn("A snow front will reach the northern platform before dusk."_el);
            manager->shutdown();
        };

        showPattern("Message-only pattern"_el, "{message}"_el);
        showPattern("Level-and-name pattern"_el, "{level} [{name}] {message}"_el);
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mMessage-only pattern:
        ␛[93;49mA snow front will reach the northern platform before du…
        ␛[39;49mLevel-and-name pattern:
        ␛[93;49mWRN [␛[95;49mguild/weather␛[93;49m] A snow front will reach the norther…␛[0m

.. erbsland-demo-end::

Truncation Marks
================

A shortened value should look shortened.
The default mark is the single ellipsis character ``…``, a compact choice that works well in ordinary console and file
output.
:cpp:func:`LogLineFormat::setTruncationMark <erbsland::log::LogLineFormat::setTruncationMark>` can replace it with text
that better suits a plain-text destination, or with an empty string when the surrounding format already makes the limit
obvious.

The same mark is used for first-line messages, message character limits, complete-line limits, and left-truncated names.
For the numeric limits it is part of the code-point budget, so a longer marker leaves less room for retained text.
An empty marker saves that space, but it also removes the reader's visible clue that text is missing.

.. erbsland-demo::
    :source: log/LoggingTopics/LineTruncationMarks.cpp
    :exec: log/logging_topics --demo LineTruncationMarks
    :sha256: 3e142dd1a67b9e76022748b36762f8564fc4f89bfbf78efacefdbacb3b28fe0a

.. code-block:: cpp

    /// Choose the visible mark that tells readers text was shortened.
    ///
    /// `setTruncationMark()` replaces the default ellipsis for first-line, character-count, total-line, and left-truncated
    /// name formatting. The mark occupies part of a character-count budget and may also be empty.
    void lineTruncationMarks() {
        const auto showMark = [](const el::String &label, el::String mark) -> void {
            el::io::printLine(label, ":"_el);
            auto lineFormat = el::LogLineFormat{};
            lineFormat.setPattern("{message}"_el)
                .setMessageTruncation(el::LogMessageTruncation::CharacterCount)
                .setMessageLimit(el::CpCount{36U})
                .setTruncationMark(std::move(mark));
            auto configuration = el::LogConfiguration{};
            configuration.setLineFormat(std::move(lineFormat))
                .addWriter(el::LogWriter::createForConsole(el::application().terminal()));

            const auto manager = el::LogManager::create();
            manager->setConfiguration(std::move(configuration));
            manager->rootStream()->info("The northern observation platform is closing for the night."_el);
            manager->shutdown();
        };

        showMark("Ellipsis"_el, "…"_el);
        showMark("Text marker"_el, " [more]"_el);
        showMark("No marker"_el, ""_el);
    }

.. erbsland-demo-run::

    .. erbsland-ansi::
        :escape-char: ␛

        ␛[0mEllipsis:
        ␛[97;49mThe northern observation platform i…
        ␛[39;49mText marker:
        ␛[97;49mThe northern observation plat [more]
        ␛[39;49mNo marker:
        ␛[97;49mThe northern observation platform is␛[0m

.. erbsland-demo-end::

Per-Writer Line Formats
=======================

A terminal and a log file often serve different readers.
The terminal can show a compact progress message, while a file keeps timestamps and stream names for later
investigation.
Each writer route can have its own :cpp:class:`LogLineFormat <erbsland::log::LogLineFormat>`, so the same entry can
appear differently at each destination.

The first example on this page sets a default format for the configuration.
To give its console route a message-only layout, replace the console ``addWriter()`` call with this small variation:

.. code-block:: cpp

    auto consoleFormat = el::LogLineFormat{};
    consoleFormat.setPattern("{message}"_el);
    configuration.addWriter(
        el::LogWriter::createForConsole(el::application().terminal()), {}, std::move(consoleFormat));

The second argument, ``{}``, keeps the filter that accepts all levels and paths.
The third argument supplies the console format.
Any other writer added without that third argument continues to use the configuration's default format.
:doc:`elcl_configuration` shows a complete, parser-exercised configuration with different console and file layouts.

A route's custom format supplies all of its presentation settings, including truncation and timestamp choices.
It does not inherit settings from the configuration default.
When you want to change only one setting, copy the shared format and adjust that copy before passing it to
:cpp:func:`LogConfiguration::addWriter <erbsland::log::LogConfiguration::addWriter>`.
Formatting leaves the original entry intact: shortening a message for the console does not shorten the file writer's
copy.

Producer Message Limits versus Rendered Line Limits
===================================================

All limits on :cpp:class:`LogLineFormat <erbsland::log::LogLineFormat>` are presentation choices applied when a writer
renders an entry.
They do not reduce the message retained in the manager's queue, and their units are Unicode code points.
Choose them to keep logs pleasant to scan and compatible with a destination's line conventions.

:cpp:func:`LogManagerOptions::setMaximumMessageBytes <erbsland::log::LogManagerOptions::setMaximumMessageBytes>` solves a
different problem.
It places a producer-side byte limit on the sanitized message before the entry enters the queue, protecting memory even
when a writer later uses an unlimited line format.
Configure that boundary as part of :doc:`configuring_manager_options`, independently of the line layout readers see.
