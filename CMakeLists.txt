cmake_minimum_required (VERSION 3.1)

if(POLICY CMP0072)
  cmake_policy(SET CMP0072 NEW)
endif()

project (PanelMethod C)

find_package(PkgConfig REQUIRED)
find_package(assimp CONFIG REQUIRED)
pkg_check_modules(GTK REQUIRED gtk+-3.0)
INCLUDE(FindOpenGL REQUIRED)


add_executable(PanelMethod src/main.c src/util/vectormath.c src/geometry/wing.c src/geometry/airfoil.c src/solver/panel.c src/graphics/section.c src/graphics/object.c src/graphics/mesh.c)
target_include_directories(PanelMethod PRIVATE ${GTK_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIR}/Headers)
target_link_libraries(PanelMethod PRIVATE m ${GTK_LIBRARIES} assimp OpenGL::GL lapacke jansson)
set_property(TARGET PanelMethod PROPERTY C_STANDARD 99)

