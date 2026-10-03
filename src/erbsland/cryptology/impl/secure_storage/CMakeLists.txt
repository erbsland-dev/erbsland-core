cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        SecureStorageBackend.cpp
        SecureStorageBackend.hpp
        SecureStorageBackend_fwd.hpp
        UnsupportedSecureStorageBackend.hpp
)

if (APPLE)
    target_sources(erbsland-core PRIVATE
            MacosPasswordBuffer.hpp
            MacosSecureStorageBackend.cpp
            MacosSecureStorageBackend.hpp
    )
    target_link_libraries(erbsland-core PRIVATE "-framework Security" "-framework CoreFoundation")
elseif (WIN32)
    target_sources(erbsland-core PRIVATE
            WindowsCredentialBuffer.hpp
            WindowsSecureStorageBackend.cpp
            WindowsSecureStorageBackend.hpp
    )
    target_link_libraries(erbsland-core PRIVATE Advapi32)
endif ()
