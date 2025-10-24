project(region_query CXX)
set(CMAKE_CXX_STANDARD 17)

add_executable(region_query
  src/main.cpp
  src/QueryBuilder.cpp
  src/QueryExecutor.cpp
)

target_include_directories(region_query PRIVATE include)
target_link_libraries(region_query PRIVATE pqxx pq gflags)
