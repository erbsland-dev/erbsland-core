// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace app::constant_time {

/// Outcome of one bounded timing experiment.
enum class Outcome {
    NoLeakage,    ///< Adequate usable samples without a threshold crossing.
    Leakage,      ///< An eligible statistical channel exceeded the threshold.
    Insufficient, ///< Budget exhausted before adequate usable evidence.
    Unavailable,  ///< The requested implementation cannot be measured.
    Error,        ///< Configuration, preparation, or execution failed.
    Interrupted,  ///< Cooperative interruption stopped the sequence.
};

}
