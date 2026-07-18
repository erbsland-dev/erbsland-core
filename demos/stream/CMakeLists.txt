# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(Support)
add_subdirectory(BufferingAndBackPressure)
add_subdirectory(ByteInputAndOutput)
add_subdirectory(CoroutineStreams)
add_subdirectory(CreatingStreams)
add_subdirectory(CustomStreams)
add_subdirectory(Lifecycle)
add_subdirectory(Positioning)
add_subdirectory(ReadinessAndTimeouts)
add_subdirectory(AnyStringBuilderStream)
add_subdirectory(StandardStreams)
add_subdirectory(StreamErrors)
add_subdirectory(TemporaryStreams)
add_subdirectory(TextInputAndOutput)

foreach(_stream_demo_target IN ITEMS
        buffering_and_back_pressure
        byte_input_and_output
        coroutine_streams
        creating_streams
        custom_streams
        lifecycle
        positioning
        readiness_and_timeouts
        any_string_builder_stream
        standard_streams
        stream_errors
        temporary_streams
        text_input_and_output
)
    target_link_libraries(${_stream_demo_target} PRIVATE erbsland-core-stream-demo-support)
endforeach()
