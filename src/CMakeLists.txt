cmake_minimum_required(VERSION 2.8)

set(FURREAL_SOURCE
    # Core simulation algorithms
    core/particle.cpp
    core/integrator.cpp
    core/spatial_hash.cpp
    core/bvh.cpp
    core/wind.cpp

    # Collision objects
    collision/sphere.cpp
    collision/plane.cpp
    collision/box.cpp
    collision/capsule.cpp

    # Hair simulation
    hair/strand.cpp
    hair/hairSystem.cpp
    hair/ftlSolver.cpp
    hair/hairInteraction.cpp

    # Rendering
    rendering/strandRenderer.cpp

    # I/O
    io/sceneLoader.cpp
    io/objLoader.cpp

    # Misc utilities
    misc/sphere_drawing.cpp
    misc/file_utils.cpp

    # Application
    main.cpp
    app.cpp
    camera.cpp
)

if(WIN32)
list(APPEND FURREAL_SOURCE misc/getopt.c)
endif(WIN32)

add_definitions(${NANOGUI_EXTRA_DEFS})

include_directories(
  ${CGL_INCLUDE_DIRS}
  ${FREETYPE_INCLUDE_DIRS}
  ${NANOGUI_EXTRA_INCS}
)
link_directories(
  ${CGL_LIBRARY_DIRS}
  ${FREETYPE_LIBRARY_DIRS}
)

add_executable(furreal ${FURREAL_SOURCE})
target_link_libraries(furreal
    CGL ${CGL_LIBRARIES}
    nanogui ${NANOGUI_EXTRA_LIBS}
    ${FREETYPE_LIBRARIES}
    ${CMAKE_THREADS_INIT}
)

if(APPLE)
  set_property(TARGET furreal APPEND_STRING PROPERTY COMPILE_FLAGS
               "-Wno-deprecated-declarations -Wno-c++11-extensions")
endif(APPLE)

set(EXECUTABLE_OUTPUT_PATH ..)
install(TARGETS furreal DESTINATION ${FurReal_SOURCE_DIR})
