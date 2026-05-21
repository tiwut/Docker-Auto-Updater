#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileDialog>
#include <QMessageBox>
#include <QScrollBar>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("Docker Auto Deployer & Updater");
    resize(600, 550);

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    QHBoxLayout *topLayout = new QHBoxLayout();
    usernameInput = new QLineEdit("tiwutdev"); 
    fetchBtn = new QPushButton("1. Fetch Existing Repos");
    topLayout->addWidget(new QLabel("Docker Hub User:"));
    topLayout->addWidget(usernameInput);
    topLayout->addWidget(fetchBtn);
    mainLayout->addLayout(topLayout);

    repoList = new QListWidget();
    mainLayout->addWidget(new QLabel("Select an existing repo, OR type a new one below:"));
    mainLayout->addWidget(repoList);

    QHBoxLayout *targetLayout = new QHBoxLayout();
    targetRepoInput = new QLineEdit();
    targetRepoInput->setPlaceholderText("e.g., my-awesome-new-app");
    targetLayout->addWidget(new QLabel("Image Name:"));
    targetLayout->addWidget(targetRepoInput);
    mainLayout->addLayout(targetLayout);

    QHBoxLayout *pathLayout = new QHBoxLayout();
    localPathInput = new QLineEdit();
    localPathInput->setPlaceholderText("Select the local folder containing the Dockerfile...");
    browseBtn = new QPushButton("Browse...");
    pathLayout->addWidget(localPathInput);
    pathLayout->addWidget(browseBtn);
    mainLayout->addLayout(pathLayout);

    updateBtn = new QPushButton("2. Build & Push Image");
    updateBtn->setMinimumHeight(40);
    updateBtn->setStyleSheet("background-color: #007bff; color: white; font-weight: bold;");
    mainLayout->addWidget(updateBtn);

    logOutput = new QTextEdit();
    logOutput->setReadOnly(true);
    logOutput->setStyleSheet("background-color: #1e1e1e; color: #00ff00; font-family: monospace;");
    mainLayout->addWidget(new QLabel("Console Output:"));
    mainLayout->addWidget(logOutput);

    setCentralWidget(centralWidget);

    networkManager = new QNetworkAccessManager(this);
    dockerProcess = new QProcess(this);
    dockerProcess->setProcessChannelMode(QProcess::MergedChannels);

    connect(fetchBtn, &QPushButton::clicked, this, &MainWindow::fetchRepositories);
    connect(networkManager, &QNetworkAccessManager::finished, this, &MainWindow::onNetworkReply);
    connect(repoList, &QListWidget::itemClicked, this, &MainWindow::onRepoSelected);
    connect(browseBtn, &QPushButton::clicked, this, &MainWindow::selectLocalFolder);
    connect(updateBtn, &QPushButton::clicked, this, &MainWindow::startUpdate);
    
    connect(dockerProcess, &QProcess::readyReadStandardOutput, this, &MainWindow::onProcessOutput);
    connect(dockerProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &MainWindow::onProcessFinished);
}

MainWindow::~MainWindow() {}

void MainWindow::fetchRepositories() {
    QString username = usernameInput->text().trimmed();
    if (username.isEmpty()) return;

    repoList->clear();
    logOutput->append("Fetching repositories for " + username + "...");
    
    QUrl url("https://hub.docker.com/v2/repositories/" + username + "/?page_size=100");
    networkManager->get(QNetworkRequest(url));
}

void MainWindow::onNetworkReply(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(response);
        QJsonArray results = doc.object().value("results").toArray();

        for (const QJsonValue &val : results) {
            QString repoName = val.toObject().value("name").toString();
            repoList->addItem(repoName);
        }
        logOutput->append("Found " + QString::number(results.size()) + " repositories.");
    } else {
        logOutput->append("Error fetching API: " + reply->errorString());
    }
    reply->deleteLater();
}

void MainWindow::onRepoSelected(QListWidgetItem *item) {
    targetRepoInput->setText(item->text());
}

void MainWindow::selectLocalFolder() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select Directory containing Dockerfile", "", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!dir.isEmpty()) {
        localPathInput->setText(dir);
    }
}

void MainWindow::startUpdate() {
    QString repoName = targetRepoInput->text().trimmed();
    if (repoName.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please enter an Image Name or select one from the list.");
        return;
    }
    if (localPathInput->text().isEmpty()) {
        QMessageBox::warning(this, "Error", "Please select the local folder containing the Dockerfile.");
        return;
    }

    QString username = usernameInput->text().trimmed();
    currentRepo = username + "/" + repoName + ":latest";
    QString localDir = localPathInput->text();

    logOutput->clear();
    updateBtn->setEnabled(false);

    currentAction = "BUILD";
    logOutput->append(">>> STARTING BUILD: " + currentRepo);
    
    QString cmd = QString("docker build --pull -t %1 '%2'").arg(currentRepo, localDir);
    dockerProcess->start("/bin/bash", QStringList() << "-c" << cmd);
}

void MainWindow::onProcessOutput() {
    QByteArray output = dockerProcess->readAllStandardOutput();
    logOutput->append(QString::fromUtf8(output).trimmed());
    
    QScrollBar *sb = logOutput->verticalScrollBar();
    sb->setValue(sb->maximum());
}

void MainWindow::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    QByteArray remaining = dockerProcess->readAllStandardOutput();
    if (!remaining.isEmpty()) {
        logOutput->append(QString::fromUtf8(remaining).trimmed());
    }

    if (exitCode != 0 || exitStatus == QProcess::CrashExit) {
        logOutput->append(">>> ERROR: Process failed with exit code: " + QString::number(exitCode));
        updateBtn->setEnabled(true);
        return;
    }

    if (currentAction == "BUILD") {
        currentAction = "PUSH";
        logOutput->append("\n>>> BUILD SUCCESSFUL. STARTING PUSH: " + currentRepo);
        
        QString cmd = QString("docker push %1").arg(currentRepo);
        dockerProcess->start("/bin/bash", QStringList() << "-c" << cmd);
    } 
    else if (currentAction == "PUSH") {
        logOutput->append("\n>>> SUCCESS! Image deployed securely to Docker Hub.");
        updateBtn->setEnabled(true);
    }
}