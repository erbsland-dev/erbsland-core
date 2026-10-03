# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core-mail-support PRIVATE
    MailApplication.hpp
    MailConfiguration.hpp
    MailMessage.hpp
    MailSupport.cpp
    PendingOutput.hpp
    SmtpLineBuffer.hpp
    SmtpReply.hpp
)
