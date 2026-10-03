# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(mail_receiver PRIVATE
    MailReceiverApp.hpp
    MailSession.hpp
    MailTransaction.hpp
    ReceiverSettings.hpp
    main.cpp
)
