# src/CMakeLists.txt

# Give the executable a more descriptive name
set(TARGET_NAME vulkan_engine)

# Add source files to the executable
add_executable(${TARGET_NAME}
        main.cpp
        vk_engine.cpp
        vk_engine.h
        vk_loader.h
        vk_loader.cpp
)

# Require C++20 standard
set_property(TARGET ${TARGET_NAME} PROPERTY CXX_STANDARD 20)

# Set working directory for Visual Studio debugger
set_property(TARGET ${TARGET_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:${TARGET_NAME}>)

# Add current source directory to include paths
target_include_directories(${TARGET_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

# Link required libraries
target_link_libraries(${TARGET_NAME}
        vkguide_shared
        vkbootstrap
        imgui
        fastgltf::fastgltf
)

# Copy runtime DLLs to output directory after build (Windows)
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:${TARGET_NAME}> $<TARGET_FILE_DIR:${TARGET_NAME}>
        COMMAND_EXPAND_LISTS
)

# Ensure shaders are compiled before building the executable
add_dependencies(${TARGET_NAME} Shaders)