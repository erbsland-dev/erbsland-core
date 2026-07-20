// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HashWorkerFactory.hpp"

#include "Sha3HashWorker.hpp"

#include "../../err/LogicError.hpp"

namespace erbsland::cryptology::impl {

auto createHashWorker(const HashAlgorithm algorithm) -> std::shared_ptr<HashWorker> {
    switch (algorithm.toRawValue()) {
    case HashAlgorithm::Sha3_256:
        return std::make_shared<Sha3_256HashWorker>();
    case HashAlgorithm::Sha3_384:
        return std::make_shared<Sha3_384HashWorker>();
    case HashAlgorithm::Sha3_512:
        return std::make_shared<Sha3_512HashWorker>();
    }
    throw err::LogicError{"Invalid hash algorithm."};
}

}
