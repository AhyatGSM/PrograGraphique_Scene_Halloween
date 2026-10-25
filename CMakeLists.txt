cmake_minimum_required(VERSION 3.10)
project(OpenGLScene CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)

# Classes communes compilées une seule fois
add_library(engine STATIC
    Camera.cpp
    Mesh.cpp
    ShaderProgram.cpp
    Texture2D.cpp
)
# Dossier du projet en include : trouve stb_image/stb_image.h et glm/ s'il est local
target_include_directories(engine PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(engine PUBLIC OpenGL::GL GLEW::GLEW glfw)

# Copie shaders/models/textures dans build/ à chaque compilation
add_custom_target(copy_resources ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/shaders  ${CMAKE_BINARY_DIR}/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/models   ${CMAKE_BINARY_DIR}/models
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/textures ${CMAKE_BINARY_DIR}/textures
)

# Un exécutable par fichier contenant un main()
set(PROGRAMS
    # Lighting_Phong
    # Lighting_Materials
    # Lighting_Dir
    # Lighting_Point
    # Lighting_Spot
    # Lighting_Multiple
    MyScene            # ta scène, ignorée tant que MyScene.cpp n'existe pas
)

foreach(prog ${PROGRAMS})
    if(EXISTS ${CMAKE_SOURCE_DIR}/${prog}.cpp)
        add_executable(${prog} ${prog}.cpp)
        target_link_libraries(${prog} PRIVATE engine)
        add_dependencies(${prog} copy_resources)
    endif()
endforeach()
