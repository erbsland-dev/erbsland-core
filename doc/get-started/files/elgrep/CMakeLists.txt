add_executable(elgrep
        src/ElGrepApp.cpp
        src/ElGrepApp.hpp
        src/main.cpp
)

target_compile_features(elgrep PRIVATE cxx_std_20)
target_link_libraries(elgrep PRIVATE erbsland::core)
