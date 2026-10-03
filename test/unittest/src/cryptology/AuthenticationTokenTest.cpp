// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/AuthenticationTokenPair.hpp>
#include <erbsland/ByteBlockEditor.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/ParameterFailure.hpp>
#include <erbsland/PendingAuthenticationChallenge.hpp>
#include <erbsland/SignedByteBlock.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/TimeAmounts.hpp>
#include <erbsland/TimeDelta.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::cryptology;
using namespace el::text::literals;

namespace erbsland::cryptology::impl {

class AuthenticationChallengeTestAccess final {
public:
    /// Bind access to the target instance, which must outlive this helper.
    /// @param pending The accessed instance.
    explicit AuthenticationChallengeTestAccess(PendingAuthenticationChallenge &pending) noexcept : _pending{pending} {}

public: // access
    [[nodiscard]] auto verifyExpired(const mem::ConstByteSpan response) -> bool {
        return _pending.verifyAt(response, _pending._deadline + time::TimeDelta{time::Nanoseconds{1}});
    }

private:
    PendingAuthenticationChallenge &_pending; ///< The accessed instance.
};

}

TESTED_TARGETS(
    AuthenticationTokenPair ClientAuthenticationToken ServerAuthenticationToken PendingAuthenticationChallenge)
class AuthenticationTokenTest final : public el::UnitTest {
public:
    void testTextRoundTripAndMatching() {
        const auto scope = ApplicationTestScope<>{};
        auto pair = AuthenticationTokenPair::generate("site_001"_el);
        const auto clientText = pair.client().toString();
        const auto serverText = pair.server().toString();
        REQUIRE(clientText.isSensitive());
        REQUIRE(clientText.startsWith("elctk-site_001-c-"_el));
        REQUIRE(serverText.startsWith("elctk-site_001-s-"_el));
        const auto parsedClient = ClientAuthenticationToken::fromStringOrThrow(clientText);
        const auto parsedServer = ServerAuthenticationToken::fromStringOrThrow(serverText);
        REQUIRE(parsedServer.matches(parsedClient));
        REQUIRE_EQUAL(parsedClient.toString(), clientText);
        REQUIRE_EQUAL(parsedServer.toString(), serverText);
        REQUIRE(ClientAuthenticationToken::fromString(serverText).isEmpty());
        REQUIRE(ServerAuthenticationToken::fromString(clientText).isEmpty());
        auto padded = el::text::StringEditor{clientText};
        padded.append(U'=');
        REQUIRE(ClientAuthenticationToken::fromString(el::text::String{padded}).isEmpty());
        auto wrongMarker = el::text::StringEditor{clientText};
        wrongMarker.replace(el::unit::ByteRange{el::unit::ByteIndex{17U}, el::unit::ByteCount{1U}}, "B"_el);
        REQUIRE(ClientAuthenticationToken::fromString(el::text::String{wrongMarker}).isEmpty());
        const auto another = AuthenticationTokenPair::generate("site_001"_el);
        REQUIRE_FALSE(parsedServer.matches(another.client()));
        REQUIRE_THROWS(AuthenticationTokenPair::generate("bad-id"_el));
        REQUIRE_THROWS(AuthenticationTokenPair::generate(""_el));
    }

    void testChallengeReplayAndMismatch() {
        const auto scope = ApplicationTestScope<>{};
        auto pair = AuthenticationTokenPair::generate("client_2"_el);
        auto pending = pair.server().createChallenge();
        const auto response = pair.client().createResponse(pending.challengeBytes().span()).toByteBlock();
        REQUIRE(pending.verify(response.span()));
        REQUIRE_FALSE(pending.verify(response.span()));

        auto otherPair = AuthenticationTokenPair::generate("other"_el);
        auto pendingWrongClient = pair.server().createChallenge();
        REQUIRE_THROWS(otherPair.client().createResponse(pendingWrongClient.challengeBytes().span()));
        const auto otherPending = otherPair.server().createChallenge();
        const auto wrongResponse =
            otherPair.client().createResponse(otherPending.challengeBytes().span()).toByteBlock();
        REQUIRE_FALSE(pendingWrongClient.verify(wrongResponse.span()));
        REQUIRE_FALSE(pendingWrongClient.verify(response.span()));

        auto pendingTampered = pair.server().createChallenge();
        auto tampered = el::mem::ByteBlockEditor{
            pair.client().createResponse(pendingTampered.challengeBytes().span()).toByteBlock()};
        tampered.xorAt(tampered.lastIndex(), el::mem::Byte{1U});
        REQUIRE_FALSE(pendingTampered.verify(tampered.span()));
    }

    void testExpiryAndLifetime() {
        const auto scope = ApplicationTestScope<>{};
        auto pair = AuthenticationTokenPair::generate("client_3"_el);
        REQUIRE_THROWS(pair.server().createChallenge(el::time::Seconds{0}));
        REQUIRE_THROWS(pair.server().createChallenge(el::time::Seconds{61}));
        auto pending = pair.server().createChallenge(el::time::Seconds{1});
        const auto response = pair.client().createResponse(pending.challengeBytes().span()).toByteBlock();
        REQUIRE_FALSE(el::cryptology::impl::AuthenticationChallengeTestAccess{pending}.verifyExpired(response.span()));
        REQUIRE_FALSE(pending.verify(response.span()));
    }
};
