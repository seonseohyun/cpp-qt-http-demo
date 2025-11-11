cmake_minimum_required(VERSION 3.16)
project(http_login_cpp)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# httplib (HTTP 서버 라이브러리)
FetchContent_Declare(
  httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG v0.15.3
)
FetchContent_MakeAvailable(httplib)

# nlohmann/json (JSON 파싱용)
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# 실행파일 만들기
add_executable(http_login src/server.cpp)

# include 경로 추가
target_include_directories(http_login PRIVATE ${httplib_SOURCE_DIR})

# 링크할 라이브러리 설정
target_link_libraries(http_login PRIVATE nlohmann_json::nlohmann_json)

if (WIN32)
  target_link_libraries(http_login PRIVATE ws2_32)
endif()