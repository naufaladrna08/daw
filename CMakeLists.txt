cmake_minimum_required(VERSION 3.0.0)
project(retro-daw VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)
enable_testing()

add_executable(retro-daw src/main.cpp)

target_link_libraries(retro-daw -lX11 -lXrandr -lXcursor)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
