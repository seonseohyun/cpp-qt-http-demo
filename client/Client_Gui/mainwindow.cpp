#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QMessageBox>
#include <QTimer>

/*
 * 생성자:
 *  - UI 초기화
 *  - QNetworkAccessManager 생성 + finished 시그널 연결
 *  - 서버 상태 라벨 초기화
 *  - 타이머로 5초마다 /health 호출
 */

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_network(new QNetworkAccessManager(this))
{
    ui->setupUi(this);

    // 네트워크 요청이 끝날 때마다 onLoginReply 슬롯으로 콜백
    // (GET /health, POST /login 둘 다 여기로 들어옴)
    connect(m_network, &QNetworkAccessManager::finished,
            this, &MainWindow::onLoginReply);

    ui->labelServerStatus->setText("서버 상태: 연결 확인 중...");

    // 프로그램 시작 시 한 번 /health 호출
    checkServerHealth();

    auto timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::checkServerHealth);
    timer->start(5000); // 5초마다 /health 호출
}


/*
 * 서버 상태 체크
 *  - 서버야~! /health 주소로 GET 요청 보낼게 살아있니?
 *  - GET http://3.36.131.145:8080/health
 *  - 서버로 비동기 요청을 보내고, 서버는 finished 시그널 발생
 *  - 결과는 onLoginReply()에서 path == "/health"로 구분해서 처리
 */

void MainWindow::checkServerHealth()
{
    QUrl url("http://15.164.100.14:8080/health");

    QNetworkRequest req(url);
    // GET 요청 전송, HTTP 요청을 보내려면 Request가 필요함, like 택배박스

    m_network->get(req);
    //이 주소로 GET 방식 HTTP 요청 보내라! 실제로 브라우저에서 URL->Enter 누르는 것을 Qt가 처리
}

/*
 * 로그인 버튼 클릭 시 호출되는 슬롯
 *  - lineEditId, lineEditPw에서 값 읽어서
 *  - JSON 바디 만들어서 /login으로 POST
 */

void MainWindow::on_btnLogin_clicked()
{
    // 1) UI에서 입력 값 읽기
    const QString id = ui->lineEditId->text().trimmed();
    const QString pw = ui->lineEditPw->text();

    // 입력 검증 (빈 값이면 로그인 시도 X)
    if (id.isEmpty()) {
        QMessageBox::warning(this, "입력 오류", "아이디를 입력하세요.");
        ui->lineEditId->setFocus();
        return;
    }

    if (pw.isEmpty()) {
        QMessageBox::warning(this, "입력 오류", "비밀번호를 입력하세요.");
        ui->lineEditPw->setFocus();
        return;
    }

    QRegularExpression emailRegex(
        R"(([\w\.-]+)@([\w\.-]+)\.([a-zA-Z]{2,}))"
        );
    if (!emailRegex.match(id).hasMatch()) {
        QMessageBox::warning(this, "입력 오류", "이메일 형식의 아이디를 입력하세요");
        ui->lineEditId->setFocus();
        return;
    }

    // 2) 요청 URL 설정
    QUrl url("http://15.164.100.14:8080/login");
    QNetworkRequest req(url);

    // 3) HTTP 헤더 설정: JSON 보낸다고 명시
    req.setHeader(QNetworkRequest::ContentTypeHeader,
                  "application/json");

    // 4) JSON 바디 구성
    QJsonObject obj;
    obj["id"] = id;
    obj["pw"] = pw;

    // Compact: 공백 없는 짧은 JSON으로 변환
    QByteArray body = QJsonDocument(obj).toJson(QJsonDocument::Compact);

    // 5) POST 요청 전송 → 응답은 onLoginReply()에서 처리
    m_network->post(req, body);
    // 6) 입력창 초기화
    lineEditIni();
}

/*
 * onLoginReply(QNetworkReply *reply)
 *
 * 이 함수는 m_network가 보낸 모든 요청(GET/POST)의 응답을 한 번에 처리.
 *  - /health 결과인지, /login 결과인지는 reply->url().path()로 구분.
 *  - 네트워크 에러, HTTP 상태 코드, JSON 파싱, ok 값까지 단계별로 체크함.
 */

void MainWindow::onLoginReply(QNetworkReply *reply)
{
    //이 응답은 어떤 요청의 응답인가?
    const QString path = reply->url().path(); // "/login" or "/health"
    // statusCode는 서버가 줄 때만 존재함 즉 머냐 서버에 접속 못했다? code는 없다.
    const int statusCode =
        reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    // 1) statusCode가 있으면 서버 응답을 받은 것
    //    → HTTP 에러(4xx, 5xx)는 나중에 처리
    // statusCode가 0이면 진짜 네트워크 연결 실패
    if (statusCode == 0 && reply->error() != QNetworkReply::NoError) {
        QMessageBox::warning(this, "네트워크 오류",
                             "서버에 연결할 수 없습니다.\n" + reply->errorString());
        reply->deleteLater();
        return;
    }

    // 1-1) HTTP 상태 코드 검사
    // (서버까지는 갔지만 서버가 401, 500 같은 실패 코드를 반환한 경우) *200이 성공

    if (statusCode != 200)
    {
        // /login 실패
        if (path == "/login" && statusCode == 401) {
            reply->deleteLater();
            QMessageBox::information(
                this,
                "로그인 실패",
                "아이디 또는 비밀번호가 잘못되었습니다."
                );
            return;
        }

        // 헬스는 200 외의 코드를 내보내지 않으므로
        QMessageBox::warning(
            this,
            "서버 오류",
            "서버 오류 발생 (HTTP " + QString::number(statusCode) + ")"
            );
        reply->deleteLater();
        return;
    }

    // 2) 여기까지 왔으면 HTTP 200 계열 !!통신 성공!!
    const QByteArray respData = reply->readAll();
    reply->deleteLater();

    // 3) JSON 파싱
    QJsonParseError parseErr;
    QJsonDocument doc = QJsonDocument::fromJson(respData, &parseErr);
    if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {

        // /health 응답이 JSON이 아니거나 깨졌을 때
        if (path == "/health") {
            ui->labelServerStatus->setText("서버 상태: 응답 파싱 실패");
            QMessageBox::warning(
                this,
                "서버 상태",
                "헬스 체크 응답 파싱 실패:\n" + parseErr.errorString()
                );

        // /login 응답 형식이 이상할 때
        } else if (path == "/login") {
            QMessageBox::warning(
                this,
                "서버 응답 오류",
                "로그인 응답 형식이 올바르지 않습니다."
                );
        }
        return;
    }

    // 4) JSON 객체로 변환 성공 → 공통 필드 ok 읽기
    QJsonObject obj = doc.object();
    const bool ok = obj.value("ok").toBool();

    // ---- /health 처리 ----
    if (path == "/health") {
        if (ok)
            ui->labelServerStatus->setText("🟢 Online");
        else
            ui->labelServerStatus->setText("🔴 Offline");
        return;
    }

    // ---- /login 처리 ----
    if (!ok) {
        QMessageBox::information(
            this,
            "로그인 실패",
            "아이디 혹은 비밀번호를 확인하세요."
            );
        return;
    }

    const QString name = obj.value("name").toString();
    QMessageBox::information(
        this,
        "로그인 성공",
        name + "님 환영합니다!"
        );
}

//라인에딧 초기화
void MainWindow::lineEditIni() {
    ui->lineEditId->clear();
    ui->lineEditPw->clear();
}

/*
 * 소멸자:
 *  - ui 포인터 삭제 (Qt가 위젯 트리 정리해줌)
 *  - m_network는 parent가 this라서 자동 삭제됨
 */

MainWindow::~MainWindow()
{
    delete ui;
}
