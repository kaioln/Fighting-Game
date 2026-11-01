add_executable(fgsim main.cpp combos.cpp)
target_link_libraries(fgsim PRIVATE fgcore)
fg_set_warnings(fgsim)
