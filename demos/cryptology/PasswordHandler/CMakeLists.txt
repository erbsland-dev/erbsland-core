# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(password-handler
        src/PasswordHandlerApp.cpp
        src/PasswordHandlerApp_addUser.cpp
        src/PasswordHandlerApp.hpp
        src/PasswordHandlerApp_login.cpp
        src/PasswordHandlerApp_userCommands.cpp
        src/PasswordPrompt.cpp
        src/PasswordPrompt.hpp
        src/PepperRotation.cpp
        src/PepperRotation.hpp
        src/PepperStore.cpp
        src/PepperStore_create.cpp
        src/PepperStore.hpp
        src/StorageFile.cpp
        src/StorageFile.hpp
        src/StoragePaths.hpp
        src/UserDatabase.cpp
        src/UserDatabase.hpp
        src/main.cpp
)
