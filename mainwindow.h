#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <adwaita.h>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>

class MainWindow {
public:
    static void onAppActivate(GtkApplication *app, gpointer user_data);

    explicit MainWindow(AdwApplication *app);
    ~MainWindow();

    GtkWidget* getWindow() const { return m_window; }

private:
    void setupUI();
    void applyCSS();

    void onFetchRepositoriesClicked();
    void onRepositorySelected(GtkListBoxRow *row);
    void onBrowseFolderClicked();
    void onStartBuildClicked();
    void onCancelBuildClicked();
    void onClearConsoleClicked();
    void onBuildxToggled();
    void onFilterChanged();

    void fetchRepositoriesWorker(const std::string &username);
    void runCommandWorker(const std::vector<std::string> &commands, const std::string &fullImage);

    void appendLog(const std::string &text);
    void clearLog();
    void setBuildingState(bool building, const std::string &statusText = "");
    void updateRepoList(const std::vector<std::string> &repos);

    std::string getSelectedPlatforms() const;
    void showAlert(const std::string &title, const std::string &message);

    AdwApplication *m_app;
    GtkWidget *m_window;
    GtkWidget *m_headerBar;
    GtkWidget *m_clearBtn;

    GtkWidget *m_usernameEntry;
    GtkWidget *m_fetchBtn;
    GtkWidget *m_fetchSpinner;
    GtkWidget *m_filterEntry;
    GtkWidget *m_repoListBox;
    GtkWidget *m_repoPlaceholder;
    std::vector<std::string> m_allRepos;

    GtkWidget *m_targetRepoEntry;
    GtkWidget *m_tagEntry;
    GtkWidget *m_localPathEntry;
    GtkWidget *m_browseBtn;

    GtkWidget *m_buildxCheck;
    GtkWidget *m_platformsBox;
    GtkWidget *m_chkAmd64;
    GtkWidget *m_chkArm64;
    GtkWidget *m_chkArmV7;
    GtkWidget *m_chkArmV6;
    GtkWidget *m_chkI386;
    GtkWidget *m_chkRiscV64;
    GtkWidget *m_customPlatformEntry;

    GtkWidget *m_pushCheck;
    GtkWidget *m_pullCheck;
    GtkWidget *m_noCacheCheck;

    GtkWidget *m_engineDockerRadio;
    GtkWidget *m_enginePodmanRadio;

    GtkWidget *m_buildBtn;
    GtkWidget *m_cancelBtn;
    GtkWidget *m_statusSpinner;
    GtkWidget *m_statusLabel;

    GtkWidget *m_consoleView;
    GtkTextBuffer *m_consoleBuffer;

    std::atomic<bool> m_isRunning{false};
    std::atomic<pid_t> m_currentChildPid{0};
    std::thread m_workerThread;
    std::mutex m_threadMutex;
};

#endif
