#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_btnLogin_clicked();
    void onLoginReply(QNetworkReply *reply);
    void checkServerHealth();
    void lineEditIni();

private:
    Ui::MainWindow *ui;
    QNetworkAccessManager *m_network;
};
#endif // MAINWINDOW_H
