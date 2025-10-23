project(region_loader CXX)
set(CMAKE_CXX_STANDARD 17)

add_executable(region_loader src/main.cpp)

target_link_libraries(region_loader PRIVATE pqxx pq gflags)
