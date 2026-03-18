project(imgui LANGUAGES CXX)

file(GLOB_RECURSE MY_SOURCES CONFIGURE_DEPENDS
    "*.cpp"
)


find_package(glfw3 CONFIG REQUIRED)

add_library(imgui STATIC ${MY_SOURCES})

target_link_libraries(imgui
  PRIVATE 
		glfw
)
