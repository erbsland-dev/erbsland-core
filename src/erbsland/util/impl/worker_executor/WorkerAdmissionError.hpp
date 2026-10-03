#pragma once

#include "../../../err/RuntimeError.hpp"

namespace erbsland::util::impl {

/// Expected rejection when a bounded background execution domain cannot admit another operation.
/// @tested{WorkerExecutorTest}
class WorkerAdmissionError final : public err::RuntimeError {
public:
    using RuntimeError::RuntimeError;
};

}
