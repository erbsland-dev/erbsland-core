// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TcpListener_fwd.hpp"
#include "TcpListenerDevice.hpp"
#include "TcpListenerDeviceCreateFn.hpp"
#include "TcpListenerEventEditor.hpp"

#include "../../../event/EventSubscription.hpp"
#include "../../tcp/TcpListener.hpp"

#include <atomic>
#include <cstddef>
#include <memory>
#include <mutex>
#include <optional>

namespace erbsland::network::impl {

/// Native TCP listener source implementation.
/// @tested{TcpListenerTest TcpSocketLiveTest}
class TcpListener final : public network::TcpListener {
    friend class TcpListenerEventEditor;

public:
    /// Create an inactive TCP listener source.
    TcpListener(
        event::EventsPtr ownerEvents,
        event::EventLoopDriverPtr driver,
        TcpListenerDeviceCreateFn createDevice = TcpListenerDevice::create);
    ~TcpListener() override;

public: // implement network::TcpListener
    [[nodiscard]] auto localEndpoint() const -> std::optional<IpEndpoint> override;
    [[nodiscard]] auto state() const noexcept -> NetworkSourceState override;
    void start(IpEndpoint localEndpoint, TcpListenerOptions options) override;
    void pauseAccepting() override;
    void resumeAccepting() override;
    void close() override;
    void abort() noexcept override;
    [[nodiscard]] auto events() -> network::TcpListenerEventEditor & override;

private:
    /// Create device callbacks bound to a listener generation.
    [[nodiscard]] auto createCallbacks(std::uint64_t generation) -> TcpListenerDeviceCallbacks;
    /// Process a socket accepted by a listener generation.
    void handleAccepted(std::uint64_t generation, TcpAcceptedSocketPtr socket);
    /// Post a listening notification for a listener generation.
    void postListening(std::uint64_t generation);
    /// Post an error notification for a listener generation.
    void postError(std::uint64_t generation, NetworkErrorContext context);
    /// Deliver a listening notification on the event loop.
    void deliverListening(std::uint64_t generation);
    /// Deliver an error notification on the event loop.
    void deliverError(std::uint64_t generation, NetworkErrorContext context);
    /// Release one pending connection request.
    void releasePendingRequest();
    /// Schedule acceptance re-evaluation after shared quota capacity changes.
    void handleQuotaCapacity();
    /// Update native acceptance according to the listener state.
    void updateAccepting();
    /// Post normal closure and final notifications.
    void postClosedAndFinal(std::uint64_t generation);
    /// Post an error and final notifications.
    void postErrorAndFinal(std::uint64_t generation, NetworkErrorContext context);
    /// Post the final notification once.
    void postFinal(std::uint64_t generation);
    /// Clean up the native listener device.
    void cleanupDevice(bool abortDevice) noexcept;
    /// Validate requested listener startup options.
    void validateStart(const IpEndpoint &localEndpoint, const TcpListenerOptions &options) const;
    /// Test whether a callback generation is still current.
    [[nodiscard]] auto isCurrent(std::uint64_t generation) const noexcept -> bool;

private:
    event::EventLoopDriverPtr _driver;                    ///< Owner loop's native driver.
    TcpListenerDeviceCreateFn _createDevice;              ///< Injectable native-device factory.
    std::unique_ptr<TcpListenerEventEditor> _eventEditor; ///< Stable source-owned editor.
    mutable std::recursive_mutex _lifecycleMutex;         ///< Serializes lifecycle changes with thread-safe abort.
    mutable std::mutex _dataMutex;                        ///< Protects endpoint and native device.
    TcpListenerDevicePtr _device;                         ///< Native listener device.
    std::optional<IpEndpoint> _localEndpoint;             ///< Actual bound endpoint.
    TcpListenerOptions _options;                          ///< Captured listener options.
    std::size_t _pendingRequestCount{};                   ///< Requests awaiting a decision.
    event::EventSubscription _quotaSubscription;          ///< Shared quota capacity subscription.
    std::atomic<NetworkSourceState> _state{NetworkSourceState::Inactive}; ///< Lifecycle state.
    std::atomic<std::uint64_t> _generation{0U};                           ///< Native operation generation.
    std::atomic<bool> _started{false};                                    ///< Whether this source was consumed.
    std::atomic<bool> _finalPosted{false};                                ///< Whether final delivery was posted.
    bool _acceptPaused{false};                                            ///< Whether application acceptance is paused.
    NetworkEventFn _onListening;                                          ///< Listening-ready handler.
    TcpConnectionRequestFn _onConnection;                                 ///< Pending-request handler.
    NetworkEventFn _onClosed;                                             ///< Normal-close handler.
    NetworkErrorFn _onError;                                              ///< Operational-error handler.
    NetworkEventFn _onFinal;                                              ///< Final handler.
};

}
