cmake_minimum_required(VERSION 2.8)
set(CMAKE_VERBOSE_MAKEFILE ON)

include_directories(/usr/local/include)
add_compile_options(-g -Wall -fpermissive -std=c++11)

add_library(logger SHARED log.cpp)
add_library(logger_s STATIC log.cpp)
set_target_properties(logger_s PROPERTIES OUTPUT_NAME "logger")
set_target_properties(logger PROPERTIES CLEAN_DIRECT_OUTPUT 1)
set_target_properties(logger_s PROPERTIES CLEAN_DIRECT_OUTPUT 1)

#target_compile_definitions(logger_s PRIVATE USE_BOOST)
#target_compile_definitions(logger PRIVATE USE_BOOST)

set(CMAKE_INSTALL_PREFIX /usr/local)
install(TARGETS logger logger_s
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
set(inc_headers log.h)
install(FILES ${inc_headers}
        DESTINATION include/logger)