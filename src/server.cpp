#include <httplib.h>          // C++ HTTP 서버/클라이언트 라이브러리
#include <nlohmann/json.hpp>  // JSON 파싱용 라이브러리
#include <iostream>

using json = nlohmann::json;  // json::parse() 등 쓸 때 간결하게 하려고 typedef

int main() {
    httplib::Server svr;  // HTTP 서버 객체 생성

    /* ---------------------------------------------------------
     * [1] GET /health
     * - 서버 상태 확인용 "헬스 체크" 엔드포인트
     * - 클라이언트가 GET 요청을 보내면 {"ok":true}를 리턴
     * --------------------------------------------------------- */
    svr.Get("/health", [](const httplib::Request& req, httplib::Response& res) {
        res.set_content(R"({"ok":true})", "application/json");
    });// 람다함수를 사용하여 health 체크 요청에 대한 응답을 정의해둠, 형식은 JSON
    

    /* ---------------------------------------------------------
     * [2] POST /login
     * - JSON 데이터를 받아 로그인 검증
     * - 예: { "id":"test", "pw":"1234" }
     * --------------------------------------------------------- */
    svr.Post("/login", [](const httplib::Request& req, httplib::Response& res) {

        // 요청 body를 JSON으로 파싱
        // parse()는 문자열을 JSON 객체로 변환함.
        auto data = json::parse(req.body, nullptr, false);

        // body가 JSON이 아니거나 필드가 없으면 에러 반환
        if (!data.is_object() || !data.contains("id") || !data.contains("pw")) {
            res.status = 400; // HTTP 400 Bad Request
            res.set_content(R"({"error":"bad_request"})", "application/json");
            return;
        }

        std::string id = data["id"];
        std::string pw = data["pw"];

        // 간단한 하드코딩된 인증 로직
        if (id == "seonseo" && pw == "1216") {
            // 로그인 성공 → 토큰 발급 응답
            res.set_content(R"({"status":"ok","token":"fake_token_123"})", "application/json");
        } else {
            // 로그인 실패
            res.status = 401; // Unauthorized
            res.set_content(R"({"status":"fail"})", "application/json");
        }
    });

    /* ---------------------------------------------------------
     * [3] 서버 실행
     * - 0.0.0.0 → 모든 네트워크 인터페이스에서 수신 가능
     * - 8080 → 기본 HTTP 포트 (보통 80, 8080, 3000 등 사용)
     * --------------------------------------------------------- */
    std::cout << "HTTP server running at: http://127.0.0.1:8080\n";
    svr.listen("0.0.0.0", 8080);
}
