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
    inc/GameImpl.h
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
    resource.h
)

set( SRC_FILES
    src/main.cpp
    src/GameImpl.cpp
    src/Window.cpp
    src/Application.cpp
    src/CommandQueue.cpp
    src/Game.cpp
    src/HighResolutionClock.cpp
)

set( RESOURCE_FILES
    myDirectX12Learning.rc
    Resources/Icon/app_icon.ico
)

source_group( "Resources" FILES myDirectX12Learning.rc )
source_group( "Resources\\Icon" FILES Resources/Icon/app_icon.ico )

set( SHADER_FILES
    shaders/VertexShader.hlsl
    shaders/PixelShader.hlsl
)

source_group( "Resources\\Shaders" FILES ${SHADER_FILES} )

set_source_files_properties( shaders/VertexShader.hlsl PROPERTIES 
    VS_SHADER_TYPE Vertex
    VS_SHADER_MODEL 5.1
)

set_source_files_properties( shaders/PixelShader.hlsl PROPERTIES 
    VS_SHADER_TYPE Pixel
    VS_SHADER_MODEL 5.1
)

add_executable( ${TARGET_NAME} WIN32
    ${HEADER_FILES}
    ${SRC_FILES}
    ${SHADER_FILES}
    ${RESOURCE_FILES}
)

target_include_directories( ${TARGET_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_link_libraries( ${TARGET_NAME}
    PUBLIC d3d12.lib
    PUBLIC dxgi.lib
    PUBLIC dxguid.lib
    PUBLIC Shlwapi.lib
)