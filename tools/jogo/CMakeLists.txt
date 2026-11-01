# Executável do jogo (apresentação 3D). Opcional: requer baixar a raylib (zlib) na configuração.
include(FetchContent)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
set(CUSTOMIZE_BUILD ON CACHE BOOL "" FORCE)
FetchContent_Declare(raylib
  URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(raylib)

add_executable(fgjogo main.cpp)
target_link_libraries(fgjogo PRIVATE fgcore raylib)
target_compile_definitions(fgjogo PRIVATE FG_ROOT_DIR="${PROJECT_SOURCE_DIR}")
if(MSVC)
  target_compile_options(fgjogo PRIVATE /utf-8 /W3)
  # sem janela de console no Windows
  set_target_properties(fgjogo PROPERTIES WIN32_EXECUTABLE TRUE LINK_FLAGS "/ENTRY:mainCRTStartup")
else()
  target_compile_options(fgjogo PRIVATE -Wall -Wextra)
endif()
