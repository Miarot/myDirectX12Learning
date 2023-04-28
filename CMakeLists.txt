cmake_minimum_required( VERSION 3.25.1 ) # Latest version of CMake when this file was created.

# Use solution folders to organize projects
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Make sure DLL and EXE targets go to the same directory.
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib) # Output directory for static lib (.LIB)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin) # Output directory for shared lib (.DLL)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin) # Output directory for executables (.EXE)

project( myDirextX12Learning LANGUAGES CXX )

# Set the startup project.
set_directory_properties( PROPERTIES
    VS_STARTUP_PROJECT myDirextX12Learning
)

set( TARGET_NAME myDirextX12Learning )

set( HEADER_FILES
    inc/d3dx12.h
    inc/Helpers.h
    inc/Window.h
    inc/Application.h
    inc/CommandQueue.h
    inc/Game.h
    inc/Events.h
    inc/KeyCodes.h
    inc/HighResolutionClock.h
    inc/DX12LibPCH.h
    inc/resource.h
)

set( SRC_FILES
    src/main.cpp
    src/Window.cpp
    src/Application.cpp
    src/CommandQueue.cpp
    src/Game.cpp
    src/HighResolutionClock.cpp
)

add_executable( ${TARGET_NAME} WIN32
    ${HEADER_FILES}
    ${SRC_FILES}
)

target_include_directories( ${TARGET_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_link_libraries( ${TARGET_NAME}
    d3d12.lib
    dxgi.lib
    dxguid.lib
)
