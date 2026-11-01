add_library(fgcore STATIC
  src/json.cpp
  src/command.cpp
  src/loader.cpp
  src/sim.cpp
  src/serialize.cpp
  src/rollback.cpp
  src/replay.cpp
  src/analyzer.cpp
)
target_include_directories(fgcore PUBLIC include)
fg_set_warnings(fgcore)

add_executable(fgcore_tests
  tests/test_main.cpp
  tests/helpers.cpp
  tests/test_json.cpp
  tests/test_command.cpp
  tests/test_loader.cpp
  tests/test_framedata.cpp
  tests/test_combat.cpp
  tests/test_mare.cpp
  tests/test_match.cpp
  tests/test_determinism.cpp
)
target_link_libraries(fgcore_tests PRIVATE fgcore)
target_compile_definitions(fgcore_tests PRIVATE FG_DATA_DIR="${PROJECT_SOURCE_DIR}/data")
fg_set_warnings(fgcore_tests)
# Testes usam combinações de bits (int -> u16) livremente; o núcleo mantém -Wconversion.
if(MSVC)
  target_compile_options(fgcore_tests PRIVATE /wd4244 /wd4267)
else()
  target_compile_options(fgcore_tests PRIVATE -Wno-conversion -Wno-sign-conversion)
endif()
add_test(NAME fgcore_tests COMMAND fgcore_tests)
