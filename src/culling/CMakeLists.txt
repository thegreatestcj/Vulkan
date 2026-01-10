add_library(culling STATIC
        frustum.cpp
        bvh.cpp
        cpu_culler_none.cpp
        cpu_culler_plane.cpp
        cpu_culler_bvh.cpp
        culler_factory.cpp
)

target_include_directories(culling PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(culling PUBLIC
        glm
        fmt::fmt
        nlohmann
        vkguide_shared
)

set_property(TARGET culling PROPERTY CXX_STANDARD 20)