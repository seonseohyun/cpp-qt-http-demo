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
}

// 로그인 서버 응답
void MainWindow::onLoginReply(QNetworkReply *reply)
{
    const QString path = reply->url().path(); // "/login" or "/health"

    // 1) 네트워크 에러
    if (reply->error() != QNetworkReply::NoError) {
        ui->labelServerStatus->setText("서버 상태: 네트워크 오류");
        QMessageBox::warning(this,
                             "네트워크 오류",
                             "서버 응답 실패:\n" + reply->errorString());
        reply->deleteLater();
        return;
    }

    // 2) JSON 파싱
    const QByteArray respData = reply->readAll();
    reply->deleteLater();

    QJsonParseError parseErr;
    QJsonDocument doc = QJsonDocument::fromJson(respData, &parseErr);
    if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
        if (path == "/health") {
            ui->labelServerStatus->setText("서버 상태: 응답 파싱 실패");
            QMessageBox::warning(this,
                                 "서버 상태",
                                 "응답 파싱 실패:\n" + parseErr.errorString());
        } else if (path == "/login") {
            QMessageBox::warning(this,
                                 "네트워크 오류",
                                 "응답 파싱 실패:\n" + parseErr.errorString());
        }
        return;
    }

    // 3) JSON 내용 처리
    QJsonObject obj = doc.object();
    const bool ok = obj.value("ok").toBool();

    //health
    if (path == "/health") {
        if (ok)
            ui->labelServerStatus->setText("🟢  Online");
        else
            ui->labelServerStatus->setText("🔴 Offline");
        return;
    }

    // 4) /login 처리
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

MainWindow::~MainWindow()
{
    delete ui;
}
