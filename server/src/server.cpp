// src/server.cpp
// C++ minimal HTTP server with DB-backed login (detailed comments)
// Dependencies:
//  - cpp-httplib (single-header)
//  - nlohmann/json (single-header)
//  - libmariadb-dev (Ubuntu 패키지로 설치)
// Build: see your CMakeLists.txt (link mariadb, nlohmann_json, Threads)

#include <httplib.h>               // lightweight HTTP server library (single-header)
#include <nlohmann/json.hpp>       // JSON parsing (single-header)
#include <iostream>
#include <string>
#include <cstring>

extern "C" {
  #include <mariadb/mysql.h>      // MariaDB C connector 헤더 (libmariadb)
}

using json = nlohmann::json;

/*
  설명(개념):
  - 이 프로그램은 EC2 같은 리눅스에서 돌아가는 간단한 HTTP 서버.
  - 클라이언트(예: Qt 앱)는 /login 엔드포인트로 JSON {id, pw}를 POST.
  - 서버는 DB에서 id로 row를 조회하고, 저장된 pw와 비교해서 승인 여부를 반환.
  - Prepared Statement 사용 → SQL 인젝션 방지.
  - 현재 pw는 평문 비교(테스트용). 운영 시에는 반드시 해시(bcrypt 등)로 변경.
*/

/* ---------- DB 연결을 단순화하기 위한 RAII wrapper ---------- */
struct MariaDBConn {
    MYSQL* conn;
    MariaDBConn(): conn(mysql_init(nullptr)) {}       // mysql_init: MYSQL 객체 생성
    ~MariaDBConn() { if (conn) mysql_close(conn); }   // 소멸 시 자동 close

    // connect: 실제 DB 서버로 연결 시도
    // host: "127.0.0.1" 권장 (TCP로 항상 연결되므로 일관성 있음)
    bool connect(const char* host, const char* user, const char* pass, const char* db, unsigned int port = 3306) {
        if (!conn) return false;
        // mysql_real_connect: host,user,password,dbname,port,... 반환값 NULL이면 실패
        if (!mysql_real_connect(conn, host, user, pass, db, port, nullptr, 0)) {
            std::cerr << "[DB] mysql_real_connect error: " << mysql_error(conn) << "\n";
            return false;
        }
        // 연결 성공 시 true
        return true;
    }
};

/* ---------- /login 핸들러 내부 동작 요약 (순서) ----------
  1) 요청 바디를 JSON으로 파싱
  2) 필수 필드(id, pw) 존재 확인
  3) DB 연결 시도
  4) Prepared Statement로 "SELECT uid, pw, name FROM user WHERE id = ?"
  5) 파라미터 바인딩, 실행
  6) 결과 바인딩 및 fetch
  7) 저장된 pw와 입력 pw 비교 (현재: 평문)
  8) 성공/실패에 따라 JSON 응답 반환
*/

int main() {
    httplib::Server svr;

    // 헬스체크: 간단하게 서비스 살아있는지 확인할 때 사용
    svr.Get("/health", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(R"({"ok":true})", "application/json");
    });

    // 로그인 엔드포인트
    svr.Post("/login", [](const httplib::Request& req, httplib::Response& res) {
        // 1) 안전하게 JSON 파싱 (false -> 예외 대신 실패 반환)
        auto data = json::parse(req.body, nullptr, false);
        if (!data.is_object() || !data.contains("id") || !data.contains("pw")) {
            // 잘못된 요청(필드 누락 등)
            res.status = 400;
            res.set_content(R"({"ok":false,"msg":"bad_request - id/pw required"})", "application/json");
            return;
        }

        // 2) 파라미터 추출
        std::string id = data["id"].get<std::string>();   // 로그인 아이디(이메일)
        std::string pw = data["pw"].get<std::string>();   // 입력 비밀번호(평문; 현재 방식)

        // 3) DB 접속 정보 - 실전에서는 환경변수 또는 시크릿 매니저를 사용하라
        const char* DB_HOST = "127.0.0.1";
        const char* DB_USER = "seonseo";       // DB 계정
        const char* DB_PASS = "seonseopw";     // DB 비밀번호
        const char* DB_NAME = "login_test";    // DB 이름

        // 4) DB 연결
        MariaDBConn db;
        if (!db.connect(DB_HOST, DB_USER, DB_PASS, DB_NAME)) {
            // DB 연결 실패 -> 서버 내부 오류
            res.status = 500;
            res.set_content(R"({"ok":false,"msg":"db connect failed"})", "application/json");
            return;
        }

        // 5) Prepared Statement 준비
        const char* query = "SELECT uid, pw, name FROM user WHERE id = ?";
        MYSQL_STMT* stmt = mysql_stmt_init(db.conn);
        if (!stmt) {
            res.status = 500;
            res.set_content(R"({"ok":false,"msg":"stmt init fail"})", "application/json");
            return;
        }

        if (mysql_stmt_prepare(stmt, query, (unsigned long)strlen(query)) != 0) {
            // prepare 실패 시 db 에러 로그를 찍고 응답
            std::cerr << "[DB] prepare error: " << mysql_error(db.conn) << "\n";
            mysql_stmt_close(stmt);
            res.status = 500;
            res.set_content(R"({"ok":false,"msg":"stmt prepare fail"})", "application/json");
            return;
        }

        // 6) 파라미터 바인드 (id)
        MYSQL_BIND bind_param;
        memset(&bind_param, 0, sizeof(bind_param));
        bind_param.buffer_type = MYSQL_TYPE_STRING;
        bind_param.buffer = (char*)id.c_str();
        bind_param.buffer_length = (unsigned long)id.size();

        if (mysql_stmt_bind_param(stmt, &bind_param) != 0) {
            std::cerr << "[DB] bind_param error: " << mysql_error(db.conn) << "\n";
            mysql_stmt_close(stmt);
            res.status = 500;
            res.set_content(R"({"ok":false,"msg":"bind param fail"})", "application/json");
            return;
        }

        // 7) execute
        if (mysql_stmt_execute(stmt) != 0) {
            std::cerr << "[DB] execute error: " << mysql_error(db.conn) << "\n";
            mysql_stmt_close(stmt);
            res.status = 500;
            res.set_content(R"({"ok":false,"msg":"exec fail"})", "application/json");
            return;
        }

        // 8) 결과 바인딩 준비
        MYSQL_BIND result_bind[3];
        memset(result_bind, 0, sizeof(result_bind));

        long long uid = 0;
        // pw 컬럼은 평문이라 적당히 크기 잡음 (실제 해시면 더 길게)
        char pw_buf[512];
        unsigned long pw_len = 0;
        char name_buf[128];
        unsigned long name_len = 0;
        my_bool pw_is_null = 0;

        // uid
        result_bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
        result_bind[0].buffer = (char*)&uid;
        // pw
        result_bind[1].buffer_type = MYSQL_TYPE_STRING;
        result_bind[1].buffer = pw_buf;
        result_bind[1].buffer_length = sizeof(pw_buf);
        result_bind[1].length = &pw_len;
        result_bind[1].is_null = &pw_is_null;
        // name
        result_bind[2].buffer_type = MYSQL_TYPE_STRING;
        result_bind[2].buffer = name_buf;
        result_bind[2].buffer_length = sizeof(name_buf);
        result_bind[2].length = &name_len;

        if (mysql_stmt_bind_result(stmt, result_bind) != 0) {
            std::cerr << "[DB] bind_result error: " << mysql_error(db.conn) << "\n";
            mysql_stmt_close(stmt);
            res.status = 500;
            res.set_content(R"({"ok":false,"msg":"bind result fail"})", "application/json");
            return;
        }

        // store_result 는 선택적. 큰 결과집합이면 필요. 여기선 1 row 예상.
        mysql_stmt_store_result(stmt);

        bool found = false;
        if (mysql_stmt_fetch(stmt) == 0) {
            // fetch 성공 -> row 존재
            found = true;
        }

        mysql_stmt_free_result(stmt);
        mysql_stmt_close(stmt);

        if (!found) {
            // id 없음 -> 인증 실패
            res.status = 401;
            res.set_content(R"({"ok":false,"msg":"invalid credentials"})", "application/json");
            return;
        }

        // 9) 비교 (여기선 평문 비교)
        std::string stored_pw(pw_buf, pw_len);
        if (stored_pw == pw) {
            // 성공: 원하는 추가 동작(토큰 발급, last_login 업데이트 등) 넣어도 됨
            json out;
            out["ok"] = true;
            out["uid"] = uid;
            out["name"] = std::string(name_buf, name_len);
            res.set_content(out.dump(), "application/json");
        } else {
            // 패스워드 불일치
            res.status = 401;
            res.set_content(R"({"ok":false,"msg":"invalid credentials"})", "application/json");
        }
    });

    std::cout << "HTTP server running at: http://127.0.0.1:80\n";
    svr.listen("0.0.0.0", 80);
    return 0;
}
