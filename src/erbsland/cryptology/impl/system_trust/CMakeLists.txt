cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        LinuxSystemTrustBackend.cpp
        LinuxSystemTrustBackend.hpp
        LinuxTrustBundleCache.cpp
        LinuxTrustBundleCache.hpp
        SystemTrustBackend.cpp
        SystemTrustBackend.hpp
        SystemTrustBackend_fwd.hpp
)

if (APPLE)
    target_sources(erbsland-core PRIVATE
            MacosSystemTrustBackend.cpp
            MacosSystemTrustBackend.hpp
            MacosTrustResources.hpp
    )
    target_link_libraries(erbsland-core PRIVATE "-framework Security" "-framework CoreFoundation")
elseif (WIN32)
    target_sources(erbsland-core PRIVATE
            WindowsSystemTrustBackend.cpp
            WindowsSystemTrustBackend.hpp
            WindowsTrustResources.hpp
    )
    target_link_libraries(erbsland-core PRIVATE crypt32)
endif ()
