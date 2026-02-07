# GUI module - ImGui integration

# Find ImGui package
find_package(imgui CONFIG REQUIRED)

# Source files
set(GUI_SOURCES
    imgui_impl.cpp
    physics_panel.cpp
)

# Create GUI library
add_library(axiom_gui STATIC ${GUI_SOURCES})
add_library(axiom::gui ALIAS axiom_gui)

# Include directories
target_include_directories(axiom_gui
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Link dependencies
target_link_libraries(axiom_gui
    PUBLIC
        axiom::core
        axiom::debug
        axiom::gpu
        axiom::frontend
        Vulkan::Vulkan
        glfw
        imgui::imgui
)

# Set C++20 standard
target_compile_features(axiom_gui PUBLIC cxx_std_20)

# Installation
install(TARGETS axiom_gui
    EXPORT axiomTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install headers
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/axiom/gui
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/axiom
    FILES_MATCHING PATTERN "*.hpp"
)
