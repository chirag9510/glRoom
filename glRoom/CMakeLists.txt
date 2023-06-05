# CMakeList.txt : CMake project for glRoom, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

find_package(Bullet REQUIRED)
find_package(SDL2 REQUIRED)
find_package(GLEW REQUIRED)

include_directories(${SDL2_INCLUDE_DIR} ${BULLET_INCLUDE_DIR} ${ENTT_INCLUDE_DIR} ${TINYOBJ_INCLUDE_DIR} ${STB_INCLUDE_DIR} ${GLM_INCLUDE_DIR} ${SPDLOG_INCLUDE_DIR} ${NUKLEAR_INCLUDE_DIR} ${SDL2_MIXER_INCLUDE_DIR})

# Add source to this project's executable.
add_executable (glRoom "main.cpp" "StateManager.h" "StateManager.cpp" "states/State.h" "states/MainMenuState.cpp" "states/PlayState.h" "states/PlayState.cpp"    "SMQueue.h" "systems/Shader.h" "systems/Shader.cpp"   "AppSettings.h" "systems/CameraSys.cpp" "systems/CameraSys.h"    "systems/Components.h" "systems/RenderingSys.h"  "systems/RenderingSys.cpp" "systems/PhysicsSys.h" "systems/PhysicsSys.cpp" "systems/DebugDraw.h" "systems/InputSys.h" "systems/InputSys.cpp" "systems/SystemComponents.h" "systems/IObserver.h" "systems/Subjects.h" "systems/CRTDisplaySys.h" "systems/CRTDisplaySys.cpp" "nuklear_sdl_gl3.h" "style.h" "systems/AudioSys.h" "systems/AudioSys.cpp"   "systems/GeometryLoader.h"  "systems/GeometryLoader.cpp" "systems/RenderState.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET glRoom PROPERTY CXX_STANDARD 20)
endif()

target_link_libraries(glRoom ${SDL2_LIBRARY} ${BULLET_LIBRARIES} ${SDL2_MIXER_LIBRARY} GLEW::GLEW opengl32.lib)

# TODO: Add tests and install targets if needed.
