// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PortableX509CertificateData_fwd.hpp"

#include "../../mem/SharedDataPointer.hpp"
#include "../../mem/SharedVirtualData.hpp"

namespace erbsland::cryptology::impl {

/// Abstract shared storage behind portable and future platform-backed X.509 certificates.
/// A platform implementation retains its native certificate reference and materializes an immutable portable view when
/// requested. The public facade never exposes a native handle.
/// @notest{Abstract interface; portable and mock backend suites test concrete behavior.}
class X509CertificateData : public mem::SharedVirtualData {
public:
    // defaults
    ~X509CertificateData() override = default;

public:
    /// Access the materialized portable certificate representation.
    /// @return The immutable portable representation.
    [[nodiscard]] virtual auto portableData() const -> const PortableX509CertificateData & = 0;
    /// Create an unreferenced copy preserving the dynamic backend type.
    [[nodiscard]] virtual auto clone() const -> X509CertificateData * override = 0;
};

using X509CertificateDataPtr = mem::SharedDataPointer<X509CertificateData, true>;

}
