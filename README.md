# C++ HTTP Login Server + Qt Client  

[![GitHub license](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE) 
[![Language](https://img.shields.io/badge/language-C%2B%2B-blue)](https://en.cppreference.com/w/)
[![Framework](https://img.shields.io/badge/Framework-Qt-green)](https://www.qt.io/)

AWS 인스턴스 환경에서 C++(Qt) 기반 HTTP 서버와 클라이언트 통신을 구현하고 테스트하는 데모 프로젝트
간단한 로그인 기능을 구현한 프로젝트입니다.  
서버는 AWS EC2(Ubuntu)에서 동작하며, 클라이언트는 Qt 6 기반으로 HTTP 요청·응답을 처리합니다.

---
```
http_login_cpp/
├── server/ # C++ HTTP 서버
│ ├── src/server.cpp
│ └── CMakeLists.txt
└── client/ # Qt GUI 클라이언트
├── Client_Gui.pro (또는 CMakeLists)
├── mainwindow.cpp
├── mainwindow.h
└── mainwindow.ui
```

### **1) 서버**
- C++17  
- cpp-httplib (HTTP 서버)
- nlohmann/json (JSON)  
- MariaDB C API 통해 DB 쿼리  
- 계정 정보 조회 후 로그인 검증  
- REST API 엔드포인트 제공
  - `GET  /health`
  - `POST /login`

---

### **2) 클라이언트 (Qt 6)**
- QNetworkAccessManager 기반 HTTP 통신  
- /health를 5초마다 체크해 “Online/Offline” 표시  
- 로그인 요청 → 서버 응답 결과 팝업 표시  
- 네트워크 오류 / HTTP 오류 / JSON 파싱 실패 / 로그인 실패 등 정확한 분기 처리  
- UI: ID/PW 입력, 상태 라벨, 로그인 버튼

---
#### 서버 빌드 & 실행 (AWS EC2)
```
sudo apt update
sudo apt install -y g++ cmake libmariadb-dev mariadb-server

cd http_login_cpp/server
cmake -S . -B build
cmake --build build -j

./build/http_login
```
#### 클라이언트 빌드 & 실행 (AWS EC2)
```
Qt Creator에서 Client_Gui 프로젝트 열기 - 빌드 - 실행 혹은
mkdir build
cd build
cmake ..
make -j
```
