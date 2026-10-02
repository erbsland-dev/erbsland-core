// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "Registry.hpp"

#include "cases/ecdsa/EcdsaCase.hpp"
#include "cases/ed25519/Ed25519Case.hpp"
#include "cases/rsa/RsaCase.hpp"
#include "cases/x25519/X25519Case.hpp"

namespace app::constant_time {

using namespace el::text::literals;

void Registry::addSigning() {
    for (
        const auto operation :
        {X25519Operation::Agree,
            X25519Operation::Multiply,
            X25519Operation::Invert,
            X25519Operation::Square,
            X25519Operation::Swap}) {
        add<X25519Case>(operation);
    }
    for (
        const auto operation :
        {Ed25519Operation::PublicKey,
            Ed25519Operation::Sign,
            Ed25519Operation::Reduce,
            Ed25519Operation::Add,
            Ed25519Operation::Multiply,
            Ed25519Operation::BaseMultiply,
            Ed25519Operation::FieldMultiply,
            Ed25519Operation::FieldInvert}) {
        add<Ed25519Case>(operation);
    }
    for (const auto curve : {ci::NistPrimeCurve::Name::P256, ci::NistPrimeCurve::Name::P384}) {
        for (
            const auto operation :
            {EcdsaOperation::PublicKey,
                EcdsaOperation::Sign,
                EcdsaOperation::Multiply,
                EcdsaOperation::Invert,
                EcdsaOperation::Add,
                EcdsaOperation::BaseMultiply}) {
            add<EcdsaCase>(curve, operation);
        }
    }
    for (
        const auto operation :
        {RsaOperation::Reduce,
            RsaOperation::Multiply,
            RsaOperation::Subtract,
            RsaOperation::Power,
            RsaOperation::SignSha256,
            RsaOperation::SignSha384}) {
        add<RsaCase>(operation);
    }
}

}
