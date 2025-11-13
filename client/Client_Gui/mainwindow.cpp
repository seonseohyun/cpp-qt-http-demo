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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_network(new QNetworkAccessManager(this))
{
    ui->setupUi(this);

    connect(m_network, &QNetworkAccessManager::finished,
            this, &MainWindow::onLoginReply);

    ui->labelServerStatus->setText("서버 상태: 연결 확인 중...");
    checkServerHealth();

    auto timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::checkServerHealth);
    timer->start(5000); // 5초마다 /health 호출
}

//서버 상태 체크
void MainWindow::checkServerHealth()
{
    QUrl url("http://3.36.131.145:8080/health");
    QNetworkRequest req(url);
    m_network->get(req);
}

// 버튼 클릭 슬롯
void MainWindow::on_btnLogin_clicked()
{
    const QString id = ui->lineEditId->text().trimmed();
    const QString pw = ui->lineEditPw->text();

    QUrl url("http://3.36.131.145:8080/login");
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader,
                  "application/json");

    QJsonObject obj;
    obj["id"] = id;
    obj["pw"] = pw;

    QByteArray body = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    m_network->post(req, body);
    lineEditIni(); //입력창 초기화 여기서
}

// 로그인 서버 응답
void MainWindow::onLoginReply(QNetworkReply *reply)
{
    const QString path = reply->url().path(); // "/login" or "/health"
    const int statusCode =
        reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    // 1) 네트워크/HTTP 에러 우선 처리
    if (reply->error() != QNetworkReply::NoError) {

        // ✅ 로그인 요청인데 401이면 => "아이디/비번 오류"로 처리
        if (path == "/login" && statusCode == 401) {
            reply->deleteLater();
            QMessageBox::information(
                this,
                "로그인 실패",
                "아이디 또는 비밀번호가 잘못되었습니다."
                );
            return;
        }

        // 그 외는 진짜 네트워크/서버 문제
        if (path == "/health") {
            ui->labelServerStatus->setText("서버 상태: 네트워크 오류");
        }

        QMessageBox::warning(
            this,
            "네트워크 오류",
            "서버와 통신 중 오류가 발생했습니다.\n\n" + reply->errorString()
            );
        reply->deleteLater();
        return;
    }

    // 2) 여기까지 왔으면 HTTP 200 계열
    const QByteArray respData = reply->readAll();
    reply->deleteLater();

    QJsonParseError parseErr;
    QJsonDocument doc = QJsonDocument::fromJson(respData, &parseErr);
    if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {

        if (path == "/health") {
            ui->labelServerStatus->setText("서버 상태: 응답 파싱 실패");
            QMessageBox::warning(
                this,
                "서버 상태",
                "헬스 체크 응답 파싱 실패:\n" + parseErr.errorString()
                );
        } else if (path == "/login") {
            QMessageBox::warning(
                this,
                "서버 응답 오류",
                "로그인 응답 형식이 올바르지 않습니다."
                );
        }
        return;
    }

    QJsonObject obj = doc.object();
    const bool ok = obj.value("ok").toBool();

    // 3) /health 처리
    if (path == "/health") {
        if (ok)
            ui->labelServerStatus->setText("🟢 Online");
        else
            ui->labelServerStatus->setText("🔴 Offline");
        return;
    }

    // 4) /login 처리 (HTTP 200인 경우)
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

MainWindow::~MainWindow()
{
    delete ui;
}
