cmake_minimum_required(VERSION 3.23)

set(CMAKE_DISABLE_FIND_PACKAGE_imgui ON)


project(purge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(Ktx CONFIG REQUIRED)
find_package(directxtex CONFIG REQUIRED)
add_subdirectory(thirdParty/imgui)


file(GLOB_RECURSE MY_SOURCES CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_executable(purge ${MY_SOURCES})

target_compile_definitions("${CMAKE_PROJECT_NAME}" PUBLIC RESOURCES_PATH="${CMAKE_CURRENT_SOURCE_DIR}/resources/")
target_compile_definitions("${CMAKE_PROJECT_NAME}" PUBLIC SHADERS_PATH="${CMAKE_CURRENT_SOURCE_DIR}/shaders/")

target_include_directories(purge
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/thirdParty/imgui"
   
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)


target_link_libraries(purge
    PUBLIC
        glfw
        imgui
        Vulkan::Vulkan
        Microsoft::DirectXTex
        KTX::ktx
)