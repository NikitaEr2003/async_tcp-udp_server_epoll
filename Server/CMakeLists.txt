
cmake_minimum_required(VERSION 3.16)


project(Server VERSION 1.0 LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(SOURCES
    main.cpp
    server.cpp
    threadpool.cpp

)

set(HEADERS
    server.h
    threadpool.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)

