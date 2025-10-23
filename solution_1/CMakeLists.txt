project(data_loader CXX)
set(CMAKE_CXX_STANDARD 17)

add_executable(data_loader src/main.cpp)

target_link_libraries(data_loader PRIVATE pqxx pq gflags)
