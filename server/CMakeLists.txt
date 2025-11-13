cmake_minimum_required(VERSION 3.16)
project(http_login_cpp)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# httplib (HTTP 서버)
FetchContent_Declare(
  httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG v0.15.3
)
FetchContent_MakeAvailable(httplib)

# nlohmann/json (JSON 파서)
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# ====== 🔽 추가 (1) include 경로 추가 ======
include_directories(${CMAKE_SOURCE_DIR}/src)

# 실행파일
add_executable(http_login src/server.cpp)

# ====== 🔽 추가 (2) 링크할 라이브러리 ======
# - mariadb: C API 연결용
# - crypt: bcrypt 비교용 (libxcrypt)
# - pthreads는 Threads::Threads로
find_package(Threads REQUIRED)
target_link_libraries(http_login
  PRIVATE
    nlohmann_json::nlohmann_json
    ${CMAKE_THREAD_LIBS_INIT}
    mariadb
    crypt
)

# ====== 🔽 기존 httplib 헤더 경로 유지 ======
target_include_directories(http_login PRIVATE ${httplib_SOURCE_DIR})

# 윈도우만 ws2_32 추가
if (WIN32)
  target_link_libraries(http_login PRIVATE ws2_32)
endif()
