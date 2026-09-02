// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "X509CertificateSigningRequest.hpp"

#include "../impl/PemCodec.hpp"
#include "../impl/PemDerFileTools.hpp"

#include "../../err/LogicError.hpp"
#include "../../path/Path.hpp"
#include "../../text/Literals.hpp"

namespace erbsland::cryptology {

using namespace text::literals;

auto X509CertificateSigningRequest::toPem() const -> text::String {
    return isEmpty() ? text::String{} : impl::PemCodec{_der, impl::PemLabel::CertificateRequest}.encode();
}

void X509CertificateSigningRequest::writeToFile(const path::Path &path, const PemDerFormat format) const {
    if (isEmpty()) {
        throw err::LogicError{"A certificate signing request is required for writing."_el};
    }
    const auto file = impl::PemDerFileTools{path, impl::PemDerFileTools::Artifact::CertificateRequest};
    if (file.outputFormat(format) == PemDerFormat::Pem) {
        file.writePem(toPem());
    } else {
        file.writeDer(_der);
    }
}

}
