cmake_minimum_required(VERSION 3.16)
project(bill-service)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)

pkg_check_modules(LIBPQXX REQUIRED libpqxx)
pkg_check_modules(HIREDIS REQUIRED hiredis)

include(FetchContent)

FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.14.1
)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
)

FetchContent_Declare(
    jwt_cpp
    GIT_REPOSITORY https://github.com/Thalhammer/jwt-cpp.git
    GIT_TAG v0.6.0
)

FetchContent_MakeAvailable(httplib nlohmann_json jwt_cpp)

add_executable(bill-service
    src/main.cpp
    src/auth_middleware.cpp
    src/database.cpp
    src/redis_client.cpp
    src/events_controller.cpp
    src/expenses_controller.cpp
    src/participants_controller.cpp
    src/settlements_controller.cpp
    src/split_calculator.cpp
    src/utils.cpp
)

target_include_directories(bill-service PRIVATE
    src/
    ${LIBPQXX_INCLUDE_DIRS}
    ${HIREDIS_INCLUDE_DIRS}
)

target_link_libraries(bill-service PRIVATE
    httplib::httplib
    nlohmann_json::nlohmann_json
    jwt-cpp::jwt-cpp
    ${LIBPQXX_LIBRARIES}
    ${HIREDIS_LIBRARIES}
    pthread
)

target_compile_options(bill-service PRIVATE ${LIBPQXX_CFLAGS_OTHER} ${HIREDIS_CFLAGS_OTHER})

install(TARGETS bill-service DESTINATION bin)