// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Tls13KeySchedule.hpp"

#include "../../../err/ParameterError.hpp"
#include "../../../mem/ByteBlock.hpp"
#include "../../../text/Literals.hpp"

namespace erbsland::cryptology::impl {

/// Test-only plaintext inspection of protected TLS 1.3 schedule values.
/// @tested{Tls13KeyScheduleTest}
class Tls13KeyScheduleTestAccess final {
public:
    /// Identifies one retained schedule secret.
    enum class Secret {
        Early,             ///< Early secret.
        ClientHandshake,   ///< Client handshake traffic secret.
        ServerHandshake,   ///< Server handshake traffic secret.
        Master,            ///< Master secret.
        ClientApplication, ///< Client application traffic secret generation zero.
        ServerApplication, ///< Server application traffic secret generation zero.
        ExporterMaster,    ///< Exporter master secret.
    };

public:
    /// Bind one key schedule for test inspection.
    explicit Tls13KeyScheduleTestAccess(const Tls13KeySchedule &schedule) noexcept : _schedule{schedule} {}

public:
    /// Copy one retained protected secret into sensitive test storage.
    /// @param secret The schedule secret to inspect.
    /// @return Sensitive plaintext test bytes, or an empty block if already erased.
    [[nodiscard]] auto bytes(const Secret secret) const -> mem::ByteBlock {
        const auto *source = &_schedule._earlySecret;
        switch (secret) {
        case Secret::Early:
            source = &_schedule._earlySecret;
            break;
        case Secret::ClientHandshake:
            source = &_schedule._clientHandshakeSecret;
            break;
        case Secret::ServerHandshake:
            source = &_schedule._serverHandshakeSecret;
            break;
        case Secret::Master:
            source = &_schedule._masterSecret;
            break;
        case Secret::ClientApplication:
            source = &_schedule._clientApplicationSecret;
            break;
        case Secret::ServerApplication:
            source = &_schedule._serverApplicationSecret;
            break;
        case Secret::ExporterMaster:
            source = &_schedule._exporterMasterSecret;
            break;
        }
        return source->unprotect();
    }

private:
    const Tls13KeySchedule &_schedule; ///< Inspected schedule.
};

}
