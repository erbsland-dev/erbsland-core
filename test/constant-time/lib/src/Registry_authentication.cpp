// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "Registry.hpp"

#include "cases/authentication/AuthenticationCase.hpp"
#include "cases/hash/HashCase.hpp"
#include "cases/password/PasswordVerifierCase.hpp"
#include "cases/tls/TlsFinishedCase.hpp"

#include <erbsland/cryptology/HashAlgorithm.hpp>

namespace app::constant_time {

using namespace el::text::literals;

void Registry::addAuthentication() {
    for (const auto algorithm : el::HashAlgorithm::all()) {
        add<HashCase>(algorithm);
    }
    for (const auto algorithm : {el::HashAlgorithm::Sha2_256, el::HashAlgorithm::Sha2_384}) {
        for (
            const auto operation :
            {AuthenticationOperation::HmacCalculate,
                AuthenticationOperation::HmacVerify,
                AuthenticationOperation::HkdfExtract,
                AuthenticationOperation::HkdfExpand}) {
            add<AuthenticationCase>(algorithm, operation);
        }
        add<TlsFinishedCase>(algorithm);
    }
    add<PasswordVerifierCase>(false);
    add<PasswordVerifierCase>(true);
}

}
