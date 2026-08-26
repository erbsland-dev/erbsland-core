add_executable(elgrep
        src/ElGrepApp.cpp
        src/ElGrepApp.hpp
        src/main.cpp
)

erbsland_core_setup_application(TARGET elgrep)
