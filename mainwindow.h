#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QProcess>
#include <QListWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void fetchRepositories();
    void onNetworkReply(class QNetworkReply *reply);
    void selectLocalFolder();
    void startUpdate();
    void onProcessOutput();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    QLineEdit *usernameInput;
    QPushButton *fetchBtn;
    QListWidget *repoList;
    QLineEdit *localPathInput;
    QPushButton *browseBtn;
    QPushButton *updateBtn;
    QTextEdit *logOutput;
    QNetworkAccessManager *networkManager;
    QProcess *dockerProcess;
    QString currentAction;
    QString currentRepo;
};

#endif
