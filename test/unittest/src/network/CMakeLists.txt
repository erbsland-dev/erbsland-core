# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core-unittest PRIVATE
        ConnectionQuotaTest.cpp
        ConnectionTest.cpp
        HostLookupLiveDnsTest.cpp
        HostLookupTest.cpp
        HostResolverTest.cpp
        Http1CodecTest.cpp
        Http1TransactionTest.cpp
        HttpClientTest.cpp
        HttpCookieJarTest.cpp
        HttpCookieSessionManagerTest.cpp
        HttpHeadersTest.cpp
        HttpMessageTest.cpp
        HttpRoutesTest.cpp
        HttpServerLiveTest.cpp
        HttpServerSessionTest.cpp
        HttpStaticContentTest.cpp
        HttpValueTest.cpp
        IpAddressTest.cpp
        IpNetworkTest.cpp
        NetworkFacadeTest.cpp
        NetworkProtocolFacadeTest.cpp
        NetworkValueTest.cpp
        PublicSuffixListTest.cpp
        TcpConnectionTest.cpp
        TcpListenerTest.cpp
        TcpSocketLiveTest.cpp
        TlsClientConnectionTest.cpp
        TlsClientProtocolTest.cpp
        TlsServerConnectionTest.cpp
        TlsServerProtocolTest.cpp
        TlsWireCodecTest.cpp
        UdpSocketLiveTest.cpp
        UdpSocketTest.cpp
        UrlTest.cpp
)
