
***************************
CMake Build Files Structure
***************************

The correct format is automatically enforced by the ``pre_commit`` utility.

1.  Inside ``src`` directories, there is one ``CMakeLists.txt`` per directory that:

    -   adds the contents of that directory to the build.
    -   adds subdirectories to the build.
    -   it does not add files from subdirectories.

2.  Each ``CMakeLists.txt`` has this format:

    .. code-block: text

        # [copyright]
        - 1 empty line -
        cmake_minimum_required(VERSION {version})
        - 1 empty line -
        add_subdirectory(...)  # Sorted alphabetically.
        add_subdirectory(...)
        add_subdirectory(...)
        - 1 empty line -
        target_sources({target} PRIVATE
                file1.hpp
                file2.tpp  # Sorted alphabetically.
                file3.cpp
        )
        - 1 empty line -
        if(...)
        # ...
        endif()
        - 1 empty line -

    -   One empty line between logical blocks and one at the end.
    -   4 spaces indentation for code blocks.
    -   8 spaces indentation inside commands.
    -   Files and directories sorted alphabetically.
