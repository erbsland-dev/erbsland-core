// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HashWorkerFactory.hpp"

#include "HashWorkerAdapter.hpp"

#include "../algorithm/Md5.hpp"
#include "../algorithm/Sha1.hpp"
#include "../algorithm/Sha2.hpp"
#include "../algorithm/Sha3.hpp"

#include "../../../err/LogicError.hpp"

namespace erbsland::cryptology::impl {

auto createHashWorker(const HashAlgorithm algorithm) -> std::shared_ptr<HashWorker> {
    switch (algorithm.toRawValue()) {
    case HashAlgorithm::Sha3_256:
        return std::make_shared<HashWorkerAdapter<Sha3_256, HashAlgorithm::Sha3_256>>();
    case HashAlgorithm::Sha3_384:
        return std::make_shared<HashWorkerAdapter<Sha3_384, HashAlgorithm::Sha3_384>>();
    case HashAlgorithm::Sha3_512:
        return std::make_shared<HashWorkerAdapter<Sha3_512, HashAlgorithm::Sha3_512>>();
    case HashAlgorithm::Sha2_256:
        return std::make_shared<HashWorkerAdapter<Sha2_256, HashAlgorithm::Sha2_256>>();
    case HashAlgorithm::Sha2_384:
        return std::make_shared<HashWorkerAdapter<Sha2_384, HashAlgorithm::Sha2_384>>();
    case HashAlgorithm::Sha2_512:
        return std::make_shared<HashWorkerAdapter<Sha2_512, HashAlgorithm::Sha2_512>>();
    case HashAlgorithm::Sha1:
        return std::make_shared<HashWorkerAdapter<Sha1, HashAlgorithm::Sha1>>();
    case HashAlgorithm::Md5:
        return std::make_shared<HashWorkerAdapter<Md5, HashAlgorithm::Md5>>();
    }
    throw err::LogicError{"Invalid hash algorithm."};
}

}
