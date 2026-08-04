// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/network/source/all.hpp>
#include <erbsland/network/udp/all.hpp>

#include <utility>

namespace demo {

/// Pause receive delivery only while the consumer cannot safely process callbacks.
/// Here a skill catalog is replaced atomically. Datagram delivery resumes on the same bound socket, an oversized
/// update is reported as a drop, and a valid queued update is then delivered.
class SkillCatalogReload final {
public:
    void start() {
        el::application().events()->invoke([this]() -> void { createSockets(); });
    }

private:
    void createSockets() {
        const auto events = el::application().events();
        _service = events->get<el::Network>().createUdpSocket();
        _client = events->get<el::Network>().createUdpSocket();
        _resumeTimer = events->createTimer([this]() -> void { finishReload(); });

        _service->events()
            .onBound([this]() -> void { beginReload(); })
            .onDatagram([this](el::UdpDatagram datagram) -> void { receiveUpdate(std::move(datagram)); })
            .onDatagramDropped([this](const el::UdpDatagramDropContext &drop) -> void { reportDrop(drop); })
            .onClosed([this]() -> void { socketClosed(); })
            .onError([this](const el::NetworkErrorContext &error) -> void { fail(error); });
        _client->events()
            .onBound([this]() -> void { sendUpdatesDuringReload(); })
            .onClosed([this]() -> void { socketClosed(); })
            .onError([this](const el::NetworkErrorContext &error) -> void { fail(error); });

        auto options = el::UdpSocketOptions{};
        options.setMaximumDatagramSize(el::ByteLength{1U});
        _service->start(el::IpAddress::loopbackV4(), options);
    }

    void beginReload() {
        _service->pauseReceiving();
        el::io::printLine("Skill catalog reload started; delivery paused."_el);
        _client->start(el::IpAddress::loopbackV4());
    }

    void sendUpdatesDuringReload() {
        const auto destination = _service->localEndpoint().value();
        const auto oversized = el::ByteBlock{el::ByteLength{4U}, el::Byte{1U}};
        const auto valid = el::ByteBlock{el::ByteLength{1U}, el::Byte{2U}};
        if (!_client->send(destination, oversized).isAccepted() || !_client->send(destination, valid).isAccepted()) {
            el::stdErr()->printLine("The client could not queue both skill updates."_el);
            fail();
            return;
        }
        el::io::printLine("Client queued two updates while delivery was paused."_el);
        _resumeTimer->startOnce(el::Milliseconds{50});
    }

    void finishReload() {
        el::io::printLine("Skill catalog reload completed; delivery resumed."_el);
        _service->resumeReceiving();
    }

    void reportDrop(const el::UdpDatagramDropContext &drop) {
        if (drop.reason() == el::UdpDatagramDropReason::TooLarge) {
            el::io::printLine("Oversized skill update discarded."_el);
        }
    }

    void receiveUpdate(el::UdpDatagram datagram) {
        const auto id = datagram.data().get(el::ByteIndex{0U}).toUInt8();
        if (id == 2U) {
            el::io::printLine("Delivered after catalog reload: alchemie"_el);
        }
        _client->close();
        _service->close();
    }

    void socketClosed() {
        ++_closedSocketCount;
        if (_closedSocketCount == 2) {
            el::application().quit();
        }
    }

    void fail(const el::NetworkErrorContext &error) {
        el::stdErr()->printLine(error.title(), ": "_el, error.description());
        fail();
    }

    void fail() {
        _client->abort();
        _service->abort();
        el::application().quit(el::ExitCode::failure());
    }

private:
    el::UdpSocketPtr _service;
    el::UdpSocketPtr _client;
    el::EventTimerPtr _resumeTimer;
    int _closedSocketCount = 0;
};

SkillCatalogReload skillCatalogReload;

void pauseDelivery() {
    skillCatalogReload.start();
}

}
