..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    !single: Random API Overview
    single: Random
    single: FastRandom
    single: SecureRandom
    single: ThreadSafeFastRandom
    single: Choosing a Generator
    single: Reproducible Randomness
    single: Writing Generator-Independent Code
    single: Thread Safety
    single: application
    single: Random numbers
    single: Generator
    single: Seed
    single: Simulations
    single: Secure Random

.. _random-overview:

*******************
Random API Overview
*******************

This page gives you a practical overview of the random API in Erbsland Core.
You will learn which generator to use for common tasks, how reproducible pseudo-random sequences work, and when a secure
source is required.

The random API provides three generator choices with clear intent.
Use fast randomness for simulations and ordinary variation, use the shared thread-safe generator for application-wide
non-security decisions, and use secure randomness whenever a generated value protects access, identity, or private data.
All generators share the convenience API from :cpp:class:`Random <erbsland::random::Random>`, so helper functions can
work with a common interface while the caller chooses the actual source.

Choosing a Generator
====================

Most code should start by deciding what the random value is used for.
This is more important than the exact method you call.

Use :cpp:class:`FastRandom <erbsland::random::FastRandom>` when the values only need to *look* random.
This is the right choice for simulations, games, procedural content, test data, randomized UI behavior, and similar
tasks where a predictable sequence does not harm the user.
Create a local instance when the random sequence belongs to one specific task.

Use :cpp:class:`ThreadSafeFastRandom <erbsland::random::ThreadSafeFastRandom>` when several threads need to share one
non-security generator.
The application object provides such a generator through
:cpp:class:`el::application().random() <erbsland::core::Application>`.
This shared generator is convenient for small application-wide decisions that are not important enough to pass a
separate generator through your own API.

Use :cpp:class:`SecureRandom <erbsland::random::SecureRandom>` when a generated value protects something.
A protected value includes passwords, reset links, API keys, session identifiers, salts, nonces, private identifiers,
and any value where guessing the result would create a security or privacy problem.
We recommend that you use the generator provided through the :cpp:class:`Application <erbsland::core::Application>`
interface through :cpp:class:`el::application().secureRandom() <erbsland::core::Application>`.

A useful rule is:

- If reproducibility is useful, use :cpp:class:`FastRandom <erbsland::random::FastRandom>`.
- If shared non-security randomness is convenient, use
  :cpp:class:`ThreadSafeFastRandom <erbsland::random::ThreadSafeFastRandom>`.
- If predictability would be dangerous, use :cpp:class:`SecureRandom <erbsland::random::SecureRandom>` through
  :cpp:class:`el::application().secureRandom() <erbsland::core::Application>`.

Overview Demo
=============

This demo uses all three generator choices in one small workflow.
The map rows and wind sample use the shared application generator for ordinary non-security choices.
The token uses :cpp:class:`SecureRandom <erbsland::random::SecureRandom>` and prints only its length, not the secret
itself.
Only the final replay check uses an explicit seed, because that line intentionally demonstrates reproducible
:cpp:class:`FastRandom <erbsland::random::FastRandom>` behavior.

.. erbsland-demo::
    :source: random/RandomTopics/ApiOverview.cpp
    :exec: random/random_topics --demo ApiOverview
    :source-sha256: 9b7298912dfdff6e8ff5297239575ff3e171776418c23208084f50756c513dcf

.. code-block:: cpp

    /// The random module offers generators with different intent.
    ///
    /// Use the shared application generator for ordinary non-security choices,
    /// switch to `SecureRandom` for values that protect access or identity, and
    /// reserve explicitly seeded `FastRandom` instances for reproducible tests and
    /// simulations.
    auto buildMapRows(el::Random &random) -> el::StringEditorList {
        const auto terrain = el::List<el::StringEditor>{
            el::StringEditor{"les"_el},
            el::StringEditor{"skala"_el},
            el::StringEditor{"voda"_el},
            el::StringEditor{"louka"_el},
        };
        auto rows = el::StringEditorList{};

        for (auto y = 0; y < 3; ++y) {
            auto row = el::StringEditor{};
            for (auto x = 0; x < 4; ++x) {
                if (x > 0) {
                    row.append(" "_el);
                }
                row.append(random.selectElement(terrain));
            }
            rows.append(row);
        }
        return rows;
    }

    void apiOverview() {
        // Use application randomness for ordinary non-security choices.
        auto &random = el::application().random();
        const auto mapRows = buildMapRows(random);
        el::io::printLine("Application random map:"_el);
        el::io::printLine(mapRows.join("\n"_el));

        // The same shared generator can be used throughout the application.
        const auto windSpeed = random.getDouble(0.0, 20.0);
        const auto yesNo = el::BooleanFormat::yesNo();
        el::io::printLine("Shared wind sample in range: "_el, yesNo, windSpeed >= 0.0 && windSpeed <= 20.0);

        // Secure randomness is used when a generated value protects something.
        const auto tokenAlphabet = el::CharSet::fromPattern("A-Za-z0-9"_el);
        const auto token = el::application().secureRandom().buildString(el::CpLength{16U}, tokenAlphabet);
        el::io::printLine("Secure token length: "_el, token.characterLength().toSizeT());

        // WARNING: Use explicit seeds only when reproducibility is the purpose.
        // Never use seeded fast randomness for secrets or security decisions.
        auto firstReplay = el::FastRandom{20260607U};
        auto secondReplay = el::FastRandom{20260607U};
        const auto sameReplaySequence = firstReplay.getUInt32(1U, 100U) == secondReplay.getUInt32(1U, 100U);
        el::io::printLine("Seeded replay check: "_el, yesNo, sameReplaySequence);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Application random map:
    louka skala les skala
    voda louka voda les
    les skala skala louka
    Shared wind sample in range: yes
    Secure token length: 16
    Seeded replay check: yes

.. erbsland-demo-end::

Reproducible Randomness
=======================

Sometimes you want the same pseudo-random sequence again.
Create a local :cpp:class:`FastRandom <erbsland::random::FastRandom>` with an explicit seed for this case.
With the same seed and the same sequence of calls, this generator produces the same values again.

This is useful for deterministic tests, saved game worlds, procedural assets, and debugging.
For example, a failing randomized test can print its seed, and you can run the same sequence again while investigating
the problem.

A seed is not a secret.
It makes the sequence reproducible, which is exactly what you want for tests and simulations.
It is also exactly what you do *not* want for passwords, tokens, keys, session identifiers, or other security-sensitive
values.
Use :cpp:class:`SecureRandom <erbsland::random::SecureRandom>` for those values instead.

Writing Generator-Independent Code
==================================

All generators derive from :cpp:class:`Random <erbsland::random::Random>`.
The base class provides the convenience API for integers, strings, byte blocks, element selection, and shuffling.
The concrete generator decides where the random bits come from.

This lets you write helper functions that accept ``Random &`` when they do not care which source is used.
The caller can then pass a fast local generator, the shared application generator, a secure source, or a deliberately
seeded generator for tests.

The demo above uses this pattern in ``buildMapRows()``.
The helper accepts :cpp:class:`Random <erbsland::random::Random>`, so the same function can be used with different
generator choices without changing the helper itself.

Thread Safety
=============

:cpp:class:`FastRandom <erbsland::random::FastRandom>` is intended for local use and is not thread-safe.
Keep it inside the task, object, or thread that owns the generated sequence.

:cpp:class:`ThreadSafeFastRandom <erbsland::random::ThreadSafeFastRandom>` synchronizes every primitive draw.
Convenience methods that perform several draws are safe to call from multiple threads, but they are not transactional.
Another thread may draw values between the individual primitive calls.

:cpp:class:`SecureRandom <erbsland::random::SecureRandom>` delegates to the operating system entropy source.
Use it through :cpp:class:`el::application().secureRandom() <erbsland::core::Application>`.
