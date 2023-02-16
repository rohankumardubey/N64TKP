cmake_minimum_required(VERSION 3.19)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_STANDARD 20)
add_subdirectory(core)
project(N64StandaloneFrontend)
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)
add_compile_options(-fsanitize=address
    -Wall -Wextra -pedantic -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op
    -Wmissing-include-dirs -Wnoexcept -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-promo -Wstrict-null-sentinel
    -Wstrict-overflow=5 -Wundef -Wno-unused -Wno-variadic-macros -Wno-parentheses -fdiagnostics-show-option
)
add_link_options(-fsanitize=address)
set(FRONTEND_FILES
    main.cxx
    frontend.cxx
    debugger.cxx
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_impl_glfw.cpp
    imgui/imgui_impl_opengl3.cpp
)
add_executable(N64StandaloneFrontend ${FRONTEND_FILES})
target_include_directories(N64StandaloneFrontend PRIVATE ${GLEW_INCLUDE_DIRS})
target_link_libraries(N64StandaloneFrontend N64TKP glfw ${GLEW_LIBRARIES} ${OPENGL_LIBRARIES})