add_executable(fgsim main.cpp)
target_link_libraries(fgsim PRIVATE fgcore)
fg_set_warnings(fgsim)
