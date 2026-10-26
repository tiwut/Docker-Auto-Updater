#include "mainwindow.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

struct IdleLogMsg {
    MainWindow *self;
    std::string text;
};

struct IdleStateMsg {
    MainWindow *self;
    bool building;
    std::string statusText;
};

struct IdleRepoListMsg {
    MainWindow *self;
    std::vector<std::string> repos;
};

void MainWindow::onAppActivate(GtkApplication *app, gpointer) {
    auto *mainWindow = new MainWindow(ADW_APPLICATION(app));
    gtk_window_present(GTK_WINDOW(mainWindow->getWindow()));
}

MainWindow::MainWindow(AdwApplication *app)
    : m_app(app),
      m_window(nullptr),
      m_headerBar(nullptr),
      m_clearBtn(nullptr),
      m_usernameEntry(nullptr),
      m_fetchBtn(nullptr),
      m_fetchSpinner(nullptr),
      m_filterEntry(nullptr),
      m_repoListBox(nullptr),
      m_repoPlaceholder(nullptr),
      m_targetRepoEntry(nullptr),
      m_tagEntry(nullptr),
      m_localPathEntry(nullptr),
      m_browseBtn(nullptr),
      m_buildxCheck(nullptr),
      m_platformsBox(nullptr),
      m_chkAmd64(nullptr),
      m_chkArm64(nullptr),
      m_chkArmV7(nullptr),
      m_chkArmV6(nullptr),
      m_chkI386(nullptr),
      m_chkRiscV64(nullptr),
      m_customPlatformEntry(nullptr),
      m_pushCheck(nullptr),
      m_pullCheck(nullptr),
      m_noCacheCheck(nullptr),
      m_engineDockerRadio(nullptr),
      m_enginePodmanRadio(nullptr),
      m_buildBtn(nullptr),
      m_cancelBtn(nullptr),
      m_statusSpinner(nullptr),
      m_statusLabel(nullptr),
      m_consoleView(nullptr),
      m_consoleBuffer(nullptr) {
    setupUI();
    applyCSS();
}

MainWindow::~MainWindow() {
    if (m_isRunning) {
        pid_t pid = m_currentChildPid.load();
        if (pid > 0) {
            kill(-pid, SIGTERM);
        }
    }
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
}

void MainWindow::applyCSS() {
    GtkCssProvider *provider = gtk_css_provider_new();
    const char *css = R"css(
        .console-container {
            border-radius: 8px;
            border: 1px solid alpha(currentColor, 0.15);
        }
        .console-view {
            font-family: 'JetBrains Mono', 'Fira Code', 'DejaVu Sans Mono', 'Courier New', monospace;
            font-size: 12px;
            padding: 8px;
        }
        .section-heading {
            font-weight: 700;
            font-size: 13px;
            margin-bottom: 2px;
        }
        .repo-row {
            padding: 8px 12px;
            border-bottom: 1px solid alpha(currentColor, 0.08);
            font-family: monospace;
        }
    )css";

    gtk_css_provider_load_from_string(provider, css);
    GdkDisplay *display = gdk_display_get_default();
    if (display) {
        gtk_style_context_add_provider_for_display(
            display,
            GTK_STYLE_PROVIDER(provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
        );
    }
    g_object_unref(provider);
}

void MainWindow::setupUI() {
    m_window = adw_application_window_new(GTK_APPLICATION(m_app));
    gtk_window_set_title(GTK_WINDOW(m_window), "Docker Auto Deployer & Updater");
    gtk_window_set_default_size(GTK_WINDOW(m_window), 740, 840);

    GtkWidget *toolbarView = adw_toolbar_view_new();
    adw_application_window_set_content(ADW_APPLICATION_WINDOW(m_window), toolbarView);

    m_headerBar = adw_header_bar_new();
    adw_toolbar_view_add_top_bar(ADW_TOOLBAR_VIEW(toolbarView), m_headerBar);

    m_clearBtn = gtk_button_new_with_label("Clear Log");
    gtk_widget_set_tooltip_text(m_clearBtn, "Clear console logs");
    g_signal_connect_swapped(m_clearBtn, "clicked", G_CALLBACK(+[](MainWindow *self) {
        self->onClearConsoleClicked();
    }), this);
    adw_header_bar_pack_end(ADW_HEADER_BAR(m_headerBar), m_clearBtn);

    GtkWidget *rootScrolled = gtk_scrolled_window_new();
    gtk_scrolled_window_set_propagate_natural_width(GTK_SCROLLED_WINDOW(rootScrolled), TRUE);
    adw_toolbar_view_set_content(ADW_TOOLBAR_VIEW(toolbarView), rootScrolled);

    GtkWidget *contentBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
    gtk_widget_set_margin_start(contentBox, 18);
    gtk_widget_set_margin_end(contentBox, 18);
    gtk_widget_set_margin_top(contentBox, 16);
    gtk_widget_set_margin_bottom(contentBox, 16);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(rootScrolled), contentBox);

    GtkWidget *repoCard = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_add_css_class(repoCard, "card");
    gtk_widget_set_margin_start(repoCard, 0);
    gtk_widget_set_margin_end(repoCard, 0);
    gtk_box_append(GTK_BOX(contentBox), repoCard);

    GtkWidget *repoInner = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(repoInner, 14);
    gtk_widget_set_margin_end(repoInner, 14);
    gtk_widget_set_margin_top(repoInner, 12);
    gtk_widget_set_margin_bottom(repoInner, 12);
    gtk_box_append(GTK_BOX(repoCard), repoInner);

    GtkWidget *repoHeader = gtk_label_new("Docker Hub Repositories");
    gtk_widget_add_css_class(repoHeader, "section-heading");
    gtk_widget_set_halign(repoHeader, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(repoInner), repoHeader);

    GtkWidget *topRow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *userLabel = gtk_label_new("Username:");
    m_usernameEntry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(m_usernameEntry), "tiwutdev");
    gtk_widget_set_hexpand(m_usernameEntry, TRUE);

    m_fetchBtn = gtk_button_new_with_label("Fetch Repositories");
    m_fetchSpinner = gtk_spinner_new();

    gtk_box_append(GTK_BOX(topRow), userLabel);
    gtk_box_append(GTK_BOX(topRow), m_usernameEntry);
    gtk_box_append(GTK_BOX(topRow), m_fetchSpinner);
    gtk_box_append(GTK_BOX(topRow), m_fetchBtn);
    gtk_box_append(GTK_BOX(repoInner), topRow);

    g_signal_connect_swapped(m_fetchBtn, "clicked", G_CALLBACK(+[](MainWindow *self) {
        self->onFetchRepositoriesClicked();
    }), this);
    g_signal_connect_swapped(m_usernameEntry, "activate", G_CALLBACK(+[](MainWindow *self) {
        self->onFetchRepositoriesClicked();
    }), this);

    m_filterEntry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(m_filterEntry), "Filter repositories...");
    g_signal_connect_swapped(m_filterEntry, "changed", G_CALLBACK(+[](MainWindow *self) {
        self->onFilterChanged();
    }), this);
    gtk_box_append(GTK_BOX(repoInner), m_filterEntry);

    GtkWidget *listScrolled = gtk_scrolled_window_new();
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(listScrolled), 110);
    gtk_scrolled_window_set_max_content_height(GTK_SCROLLED_WINDOW(listScrolled), 160);

    m_repoListBox = gtk_list_box_new();
    gtk_widget_add_css_class(m_repoListBox, "boxed-list");
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(m_repoListBox), GTK_SELECTION_SINGLE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(listScrolled), m_repoListBox);
    gtk_box_append(GTK_BOX(repoInner), listScrolled);

    m_repoPlaceholder = gtk_label_new("Select 'Fetch Repositories' to load remote images, or enter a target name below.");
    gtk_widget_add_css_class(m_repoPlaceholder, "dim-label");
    gtk_list_box_set_placeholder(GTK_LIST_BOX(m_repoListBox), m_repoPlaceholder);

    g_signal_connect_swapped(m_repoListBox, "row-selected", G_CALLBACK(+[](MainWindow *self, GtkListBoxRow *row) {
        self->onRepositorySelected(row);
    }), this);

    GtkWidget *imageCard = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_add_css_class(imageCard, "card");
    gtk_box_append(GTK_BOX(contentBox), imageCard);

    GtkWidget *imageInner = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(imageInner, 14);
    gtk_widget_set_margin_end(imageInner, 14);
    gtk_widget_set_margin_top(imageInner, 12);
    gtk_widget_set_margin_bottom(imageInner, 12);
    gtk_box_append(GTK_BOX(imageCard), imageInner);

    GtkWidget *imageHeader = gtk_label_new("Target Image & Source Directory");
    gtk_widget_add_css_class(imageHeader, "section-heading");
    gtk_widget_set_halign(imageHeader, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(imageInner), imageHeader);

    GtkWidget *imageRow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *targetLabel = gtk_label_new("Image Name:");
    m_targetRepoEntry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(m_targetRepoEntry), "e.g., my-service-app");
    gtk_widget_set_hexpand(m_targetRepoEntry, TRUE);

    GtkWidget *tagLabel = gtk_label_new("Tag:");
    m_tagEntry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(m_tagEntry), "latest");
    gtk_widget_set_size_request(m_tagEntry, 90, -1);

    gtk_box_append(GTK_BOX(imageRow), targetLabel);
    gtk_box_append(GTK_BOX(imageRow), m_targetRepoEntry);
    gtk_box_append(GTK_BOX(imageRow), tagLabel);
    gtk_box_append(GTK_BOX(imageRow), m_tagEntry);
    gtk_box_append(GTK_BOX(imageInner), imageRow);

    GtkWidget *pathRow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *pathLabel = gtk_label_new("Local Folder:");
    m_localPathEntry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(m_localPathEntry), "Directory containing Dockerfile or Containerfile...");
    gtk_widget_set_hexpand(m_localPathEntry, TRUE);

    m_browseBtn = gtk_button_new_with_label("Browse...");
    g_signal_connect_swapped(m_browseBtn, "clicked", G_CALLBACK(+[](MainWindow *self) {
        self->onBrowseFolderClicked();
    }), this);

    gtk_box_append(GTK_BOX(pathRow), pathLabel);
    gtk_box_append(GTK_BOX(pathRow), m_localPathEntry);
    gtk_box_append(GTK_BOX(pathRow), m_browseBtn);
    gtk_box_append(GTK_BOX(imageInner), pathRow);

    GtkWidget *buildxCard = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_add_css_class(buildxCard, "card");
    gtk_box_append(GTK_BOX(contentBox), buildxCard);

    GtkWidget *buildxInner = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(buildxInner, 14);
    gtk_widget_set_margin_end(buildxInner, 14);
    gtk_widget_set_margin_top(buildxInner, 12);
    gtk_widget_set_margin_bottom(buildxInner, 12);
    gtk_box_append(GTK_BOX(buildxCard), buildxInner);

    GtkWidget *buildxHeaderBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *buildxHeader = gtk_label_new("Build Engine & Platforms (Buildx)");
    gtk_widget_add_css_class(buildxHeader, "section-heading");
    gtk_widget_set_halign(buildxHeader, GTK_ALIGN_START);
    gtk_widget_set_hexpand(buildxHeader, TRUE);

    GtkWidget *engineLabel = gtk_label_new("Engine:");
    m_engineDockerRadio = gtk_check_button_new_with_label("Docker");
    m_enginePodmanRadio = gtk_check_button_new_with_label("Podman");
    gtk_check_button_set_group(GTK_CHECK_BUTTON(m_enginePodmanRadio), GTK_CHECK_BUTTON(m_engineDockerRadio));
    gtk_check_button_set_active(GTK_CHECK_BUTTON(m_engineDockerRadio), TRUE);

    gtk_box_append(GTK_BOX(buildxHeaderBox), buildxHeader);
    gtk_box_append(GTK_BOX(buildxHeaderBox), engineLabel);
    gtk_box_append(GTK_BOX(buildxHeaderBox), m_engineDockerRadio);
    gtk_box_append(GTK_BOX(buildxHeaderBox), m_enginePodmanRadio);
    gtk_box_append(GTK_BOX(buildxInner), buildxHeaderBox);

    m_buildxCheck = gtk_check_button_new_with_label("Enable Docker Buildx Multi-Platform compilation (--platform)");
    gtk_check_button_set_active(GTK_CHECK_BUTTON(m_buildxCheck), TRUE);
    g_signal_connect_swapped(m_buildxCheck, "toggled", G_CALLBACK(+[](MainWindow *self) {
        self->onBuildxToggled();
    }), this);
    gtk_box_append(GTK_BOX(buildxInner), m_buildxCheck);

    m_platformsBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_box_append(GTK_BOX(buildxInner), m_platformsBox);

    GtkWidget *platLabel = gtk_label_new("Target Architectures:");
    gtk_widget_set_halign(platLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(m_platformsBox), platLabel);

    GtkWidget *archGrid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(archGrid), 6);
    gtk_grid_set_column_spacing(GTK_GRID(archGrid), 18);

    m_chkAmd64 = gtk_check_button_new_with_label("linux/amd64 (x86_64, Intel/AMD)");
    gtk_check_button_set_active(GTK_CHECK_BUTTON(m_chkAmd64), TRUE);
    gtk_grid_attach(GTK_GRID(archGrid), m_chkAmd64, 0, 0, 1, 1);

    m_chkArm64 = gtk_check_button_new_with_label("linux/arm64 (ARM 64-bit, Apple Silicon / Pi 4/5)");
    gtk_check_button_set_active(GTK_CHECK_BUTTON(m_chkArm64), TRUE);
    gtk_grid_attach(GTK_GRID(archGrid), m_chkArm64, 1, 0, 1, 1);

    m_chkArmV7 = gtk_check_button_new_with_label("linux/arm/v7 (ARMv7 32-bit)");
    gtk_grid_attach(GTK_GRID(archGrid), m_chkArmV7, 0, 1, 1, 1);

    m_chkArmV6 = gtk_check_button_new_with_label("linux/arm/v6 (ARMv6, Pi 1/Zero)");
    gtk_grid_attach(GTK_GRID(archGrid), m_chkArmV6, 1, 1, 1, 1);

    m_chkI386 = gtk_check_button_new_with_label("linux/386 (x86 32-bit)");
    gtk_grid_attach(GTK_GRID(archGrid), m_chkI386, 0, 2, 1, 1);

    m_chkRiscV64 = gtk_check_button_new_with_label("linux/riscv64 (RISC-V 64-bit)");
    gtk_grid_attach(GTK_GRID(archGrid), m_chkRiscV64, 1, 2, 1, 1);

    gtk_box_append(GTK_BOX(m_platformsBox), archGrid);

    GtkWidget *customRow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *customLabel = gtk_label_new("Custom Platforms:");
    m_customPlatformEntry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(m_customPlatformEntry), "e.g., linux/ppc64le, linux/s390x (comma separated)");
    gtk_widget_set_hexpand(m_customPlatformEntry, TRUE);
    gtk_box_append(GTK_BOX(customRow), customLabel);
    gtk_box_append(GTK_BOX(customRow), m_customPlatformEntry);
    gtk_box_append(GTK_BOX(m_platformsBox), customRow);

    GtkWidget *optRow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 16);
    m_pushCheck = gtk_check_button_new_with_label("Push to registry (--push)");
    gtk_check_button_set_active(GTK_CHECK_BUTTON(m_pushCheck), TRUE);
    gtk_widget_set_tooltip_text(m_pushCheck, "Push built multi-platform manifest directly to Docker Hub");

    m_pullCheck = gtk_check_button_new_with_label("Always pull base image (--pull)");
    gtk_check_button_set_active(GTK_CHECK_BUTTON(m_pullCheck), TRUE);
    gtk_widget_set_tooltip_text(m_pullCheck, "Pull newest base image to incorporate latest security patches");

    m_noCacheCheck = gtk_check_button_new_with_label("No cache (--no-cache)");
    gtk_widget_set_tooltip_text(m_noCacheCheck, "Force clean rebuild without using docker layer cache");

    gtk_box_append(GTK_BOX(optRow), m_pushCheck);
    gtk_box_append(GTK_BOX(optRow), m_pullCheck);
    gtk_box_append(GTK_BOX(optRow), m_noCacheCheck);
    gtk_box_append(GTK_BOX(buildxInner), optRow);

    GtkWidget *actionBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_box_append(GTK_BOX(contentBox), actionBox);

    m_buildBtn = gtk_button_new_with_label("Build & Deploy");
    gtk_widget_add_css_class(m_buildBtn, "suggested-action");
    gtk_widget_set_hexpand(m_buildBtn, TRUE);
    g_signal_connect_swapped(m_buildBtn, "clicked", G_CALLBACK(+[](MainWindow *self) {
        self->onStartBuildClicked();
    }), this);

    m_cancelBtn = gtk_button_new_with_label("Cancel");
    gtk_widget_add_css_class(m_cancelBtn, "destructive-action");
    gtk_widget_set_sensitive(m_cancelBtn, FALSE);
    g_signal_connect_swapped(m_cancelBtn, "clicked", G_CALLBACK(+[](MainWindow *self) {
        self->onCancelBuildClicked();
    }), this);

    m_statusSpinner = gtk_spinner_new();
    m_statusLabel = gtk_label_new("Ready");

    gtk_box_append(GTK_BOX(actionBox), m_buildBtn);
    gtk_box_append(GTK_BOX(actionBox), m_cancelBtn);
    gtk_box_append(GTK_BOX(actionBox), m_statusSpinner);
    gtk_box_append(GTK_BOX(actionBox), m_statusLabel);

    GtkWidget *consoleHeaderRow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *consoleTitle = gtk_label_new("Console Output");
    gtk_widget_add_css_class(consoleTitle, "section-heading");
    gtk_widget_set_halign(consoleTitle, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(consoleHeaderRow), consoleTitle);
    gtk_box_append(GTK_BOX(contentBox), consoleHeaderRow);

    GtkWidget *consoleScrolled = gtk_scrolled_window_new();
    gtk_widget_add_css_class(consoleScrolled, "console-container");
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(consoleScrolled), 220);
    gtk_widget_set_vexpand(consoleScrolled, TRUE);

    m_consoleView = gtk_text_view_new();
    gtk_widget_add_css_class(m_consoleView, "console-view");
    gtk_text_view_set_editable(GTK_TEXT_VIEW(m_consoleView), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(m_consoleView), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(m_consoleView), TRUE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(m_consoleView), GTK_WRAP_WORD_CHAR);

    m_consoleBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(m_consoleView));
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(consoleScrolled), m_consoleView);
    gtk_box_append(GTK_BOX(contentBox), consoleScrolled);

    appendLog("[INFO] Docker Auto Deployer & Updater initialized.");
    appendLog("[INFO] Ready to compile and deploy container images.");
}

void MainWindow::onBuildxToggled() {
    gboolean active = gtk_check_button_get_active(GTK_CHECK_BUTTON(m_buildxCheck));
    gtk_widget_set_sensitive(m_platformsBox, active);
}

void MainWindow::onClearConsoleClicked() {
    clearLog();
}

void MainWindow::clearLog() {
    gtk_text_buffer_set_text(m_consoleBuffer, "", 0);
}

void MainWindow::appendLog(const std::string &text) {
    auto *msg = new IdleLogMsg{this, text};
    g_idle_add(+[](gpointer data) -> gboolean {
        auto *m = static_cast<IdleLogMsg*>(data);
        GtkTextIter end;
        gtk_text_buffer_get_end_iter(m->self->m_consoleBuffer, &end);
        
        std::string line = m->text;
        if (!line.empty() && line.back() != '\n') {
            line += "\n";
        }

        gtk_text_buffer_insert(m->self->m_consoleBuffer, &end, line.c_str(), -1);

        GtkTextMark *mark = gtk_text_buffer_create_mark(m->self->m_consoleBuffer, nullptr, &end, FALSE);
        gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(m->self->m_consoleView), mark, 0.0, TRUE, 0.0, 1.0);
        gtk_text_buffer_delete_mark(m->self->m_consoleBuffer, mark);

        delete m;
        return G_SOURCE_REMOVE;
    }, msg);
}

void MainWindow::setBuildingState(bool building, const std::string &statusText) {
    auto *msg = new IdleStateMsg{this, building, statusText};
    g_idle_add(+[](gpointer data) -> gboolean {
        auto *m = static_cast<IdleStateMsg*>(data);
        m->self->m_isRunning = m->building;

        gtk_widget_set_sensitive(m->self->m_buildBtn, !m->building);
        gtk_widget_set_sensitive(m->self->m_cancelBtn, m->building);
        gtk_widget_set_sensitive(m->self->m_fetchBtn, !m->building);

        if (m->building) {
            gtk_spinner_start(GTK_SPINNER(m->self->m_statusSpinner));
        } else {
            gtk_spinner_stop(GTK_SPINNER(m->self->m_statusSpinner));
        }

        if (!m->statusText.empty()) {
            gtk_label_set_text(GTK_LABEL(m->self->m_statusLabel), m->statusText.c_str());
        }

        delete m;
        return G_SOURCE_REMOVE;
    }, msg);
}

void MainWindow::onFilterChanged() {
    const char *filter = gtk_editable_get_text(GTK_EDITABLE(m_filterEntry));
    std::string needle = filter ? filter : "";
    std::transform(needle.begin(), needle.end(), needle.begin(), ::tolower);

    GtkWidget *child = gtk_widget_get_first_child(m_repoListBox);
    while (child != nullptr) {
        GtkWidget *next = gtk_widget_get_next_sibling(child);
        const char *repoName = (const char*)g_object_get_data(G_OBJECT(child), "repo-name");
        if (repoName) {
            std::string haystack = repoName;
            std::transform(haystack.begin(), haystack.end(), haystack.begin(), ::tolower);
            bool visible = needle.empty() || (haystack.find(needle) != std::string::npos);
            gtk_widget_set_visible(child, visible);
        }
        child = next;
    }
}

void MainWindow::onRepositorySelected(GtkListBoxRow *row) {
    if (!row) return;
    const char *repoName = (const char*)g_object_get_data(G_OBJECT(row), "repo-name");
    if (repoName) {
        gtk_editable_set_text(GTK_EDITABLE(m_targetRepoEntry), repoName);
    }
}

void MainWindow::updateRepoList(const std::vector<std::string> &repos) {
    auto *msg = new IdleRepoListMsg{this, repos};
    g_idle_add(+[](gpointer data) -> gboolean {
        auto *m = static_cast<IdleRepoListMsg*>(data);
        m->self->m_allRepos = m->repos;

        GtkWidget *child = gtk_widget_get_first_child(m->self->m_repoListBox);
        while (child != nullptr) {
            GtkWidget *next = gtk_widget_get_next_sibling(child);
            gtk_list_box_remove(GTK_LIST_BOX(m->self->m_repoListBox), child);
            child = next;
        }

        for (const auto &repo : m->repos) {
            GtkWidget *row = gtk_list_box_row_new();
            gtk_widget_add_css_class(row, "repo-row");

            GtkWidget *rowBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
            GtkWidget *icon = gtk_image_new_from_icon_name("package-x-generic-symbolic");
            GtkWidget *label = gtk_label_new(repo.c_str());
            gtk_widget_set_halign(label, GTK_ALIGN_START);
            gtk_widget_set_hexpand(label, TRUE);

            gtk_box_append(GTK_BOX(rowBox), icon);
            gtk_box_append(GTK_BOX(rowBox), label);
            gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), rowBox);

            g_object_set_data_full(G_OBJECT(row), "repo-name", g_strdup(repo.c_str()), g_free);
            gtk_list_box_append(GTK_LIST_BOX(m->self->m_repoListBox), row);
        }

        gtk_spinner_stop(GTK_SPINNER(m->self->m_fetchSpinner));
        gtk_widget_set_sensitive(m->self->m_fetchBtn, TRUE);

        delete m;
        return G_SOURCE_REMOVE;
    }, msg);
}

void MainWindow::onFetchRepositoriesClicked() {
    const char *userText = gtk_editable_get_text(GTK_EDITABLE(m_usernameEntry));
    std::string username = userText ? userText : "";
    username.erase(username.begin(), std::find_if(username.begin(), username.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
    username.erase(std::find_if(username.rbegin(), username.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), username.end());

    if (username.empty()) {
        showAlert("Input Required", "Please enter a Docker Hub username.");
        return;
    }

    gtk_spinner_start(GTK_SPINNER(m_fetchSpinner));
    gtk_widget_set_sensitive(m_fetchBtn, FALSE);
    appendLog("[INFO] Querying repositories for '" + username + "' from Docker Hub API...");

    std::thread([this, username]() {
        fetchRepositoriesWorker(username);
    }).detach();
}

void MainWindow::fetchRepositoriesWorker(const std::string &username) {
    std::string cmd = "curl -sSL \"https://hub.docker.com/v2/repositories/" + username + "/?page_size=100\"";
    FILE *fp = popen(cmd.c_str(), "r");
    if (!fp) {
        appendLog("[ERROR] Failed to execute curl process.");
        updateRepoList({});
        return;
    }

    std::string response;
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), fp) != nullptr) {
        response += buffer;
    }
    pclose(fp);

    std::vector<std::string> names;
    size_t resultsPos = response.find("\"results\"");
    if (resultsPos != std::string::npos) {
        size_t searchPos = resultsPos;
        while (true) {
            size_t nameKey = response.find("\"name\"", searchPos);
            if (nameKey == std::string::npos) break;

            size_t colon = response.find(':', nameKey + 6);
            if (colon == std::string::npos) break;

            size_t firstQuote = response.find('"', colon + 1);
            if (firstQuote == std::string::npos) break;

            size_t secondQuote = response.find('"', firstQuote + 1);
            if (secondQuote == std::string::npos) break;

            std::string name = response.substr(firstQuote + 1, secondQuote - firstQuote - 1);
            if (!name.empty()) {
                names.push_back(name);
            }
            searchPos = secondQuote + 1;
        }
    }

    if (names.empty()) {
        appendLog("[WARN] No public repositories retrieved for '" + username + "'.");
    } else {
        appendLog("[INFO] Retrieved " + std::to_string(names.size()) + " repositories.");
    }

    updateRepoList(names);
}

void MainWindow::onBrowseFolderClicked() {
#if GTK_CHECK_VERSION(4, 10, 0)
    GtkFileDialog *dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dialog, "Select Folder Containing Dockerfile");
    gtk_file_dialog_select_folder(dialog, GTK_WINDOW(m_window), nullptr,
        +[](GObject *source, GAsyncResult *res, gpointer user_data) {
            auto *self = static_cast<MainWindow*>(user_data);
            GError *error = nullptr;
            GFile *folder = gtk_file_dialog_select_folder_finish(GTK_FILE_DIALOG(source), res, &error);
            if (folder) {
                char *path = g_file_get_path(folder);
                if (path) {
                    gtk_editable_set_text(GTK_EDITABLE(self->m_localPathEntry), path);
                    g_free(path);
                }
                g_object_unref(folder);
            } else if (error) {
                g_error_free(error);
            }
        }, this);
#else
    GtkFileChooserNative *chooser = gtk_file_chooser_native_new(
        "Select Folder Containing Dockerfile",
        GTK_WINDOW(m_window),
        GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
        "_Select",
        "_Cancel"
    );
    g_signal_connect(chooser, "response", G_CALLBACK(+[](GtkNativeDialog *dialog, int response, gpointer user_data) {
        auto *self = static_cast<MainWindow*>(user_data);
        if (response == GTK_RESPONSE_ACCEPT) {
            GFile *file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(dialog));
            if (file) {
                char *path = g_file_get_path(file);
                if (path) {
                    gtk_editable_set_text(GTK_EDITABLE(self->m_localPathEntry), path);
                    g_free(path);
                }
                g_object_unref(file);
            }
        }
        g_object_unref(dialog);
    }), this);
    gtk_native_dialog_show(GTK_NATIVE_DIALOG(chooser));
#endif
}

std::string MainWindow::getSelectedPlatforms() const {
    std::vector<std::string> plats;

    if (gtk_check_button_get_active(GTK_CHECK_BUTTON(m_chkAmd64))) {
        plats.push_back("linux/amd64");
    }
    if (gtk_check_button_get_active(GTK_CHECK_BUTTON(m_chkArm64))) {
        plats.push_back("linux/arm64");
    }
    if (gtk_check_button_get_active(GTK_CHECK_BUTTON(m_chkArmV7))) {
        plats.push_back("linux/arm/v7");
    }
    if (gtk_check_button_get_active(GTK_CHECK_BUTTON(m_chkArmV6))) {
        plats.push_back("linux/arm/v6");
    }
    if (gtk_check_button_get_active(GTK_CHECK_BUTTON(m_chkI386))) {
        plats.push_back("linux/386");
    }
    if (gtk_check_button_get_active(GTK_CHECK_BUTTON(m_chkRiscV64))) {
        plats.push_back("linux/riscv64");
    }

    const char *custom = gtk_editable_get_text(GTK_EDITABLE(m_customPlatformEntry));
    if (custom && strlen(custom) > 0) {
        std::stringstream ss(custom);
        std::string item;
        while (std::getline(ss, item, ',')) {
            item.erase(item.begin(), std::find_if(item.begin(), item.end(), [](unsigned char ch) {
                return !std::isspace(ch);
            }));
            item.erase(std::find_if(item.rbegin(), item.rend(), [](unsigned char ch) {
                return !std::isspace(ch);
            }).base(), item.end());
            if (!item.empty()) {
                plats.push_back(item);
            }
        }
    }

    std::string res;
    for (size_t i = 0; i < plats.size(); ++i) {
        res += plats[i];
        if (i + 1 < plats.size()) res += ",";
    }
    return res;
}

void MainWindow::onStartBuildClicked() {
    const char *repoRaw = gtk_editable_get_text(GTK_EDITABLE(m_targetRepoEntry));
    std::string repoName = repoRaw ? repoRaw : "";
    repoName.erase(repoName.begin(), std::find_if(repoName.begin(), repoName.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
    repoName.erase(std::find_if(repoName.rbegin(), repoName.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), repoName.end());

    if (repoName.empty()) {
        showAlert("Error", "Please enter an Image Name or select one from the repository list.");
        return;
    }

    const char *pathRaw = gtk_editable_get_text(GTK_EDITABLE(m_localPathEntry));
    std::string localDir = pathRaw ? pathRaw : "";
    if (localDir.empty()) {
        showAlert("Error", "Please select the local folder containing the Dockerfile.");
        return;
    }

    const char *userRaw = gtk_editable_get_text(GTK_EDITABLE(m_usernameEntry));
    std::string username = userRaw ? userRaw : "";
    username.erase(username.begin(), std::find_if(username.begin(), username.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
    username.erase(std::find_if(username.rbegin(), username.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), username.end());

    const char *tagRaw = gtk_editable_get_text(GTK_EDITABLE(m_tagEntry));
    std::string tag = (tagRaw && strlen(tagRaw) > 0) ? tagRaw : "latest";

    std::string fullImage = username.empty() ? (repoName + ":" + tag) : (username + "/" + repoName + ":" + tag);

    bool useBuildx = gtk_check_button_get_active(GTK_CHECK_BUTTON(m_buildxCheck));
    bool doPush = gtk_check_button_get_active(GTK_CHECK_BUTTON(m_pushCheck));
    bool doPull = gtk_check_button_get_active(GTK_CHECK_BUTTON(m_pullCheck));
    bool noCache = gtk_check_button_get_active(GTK_CHECK_BUTTON(m_noCacheCheck));
    bool isPodman = gtk_check_button_get_active(GTK_CHECK_BUTTON(m_enginePodmanRadio));
    std::string engine = isPodman ? "podman" : "docker";

    std::string platforms = getSelectedPlatforms();
    if (useBuildx && platforms.empty()) {
        showAlert("Platform Required", "Please select at least one target platform architecture (e.g. linux/amd64).");
        return;
    }

    std::vector<std::string> commands;

    if (isPodman) {
        std::string cmd = "podman build";
        if (doPull) cmd += " --pull";
        if (noCache) cmd += " --no-cache";
        if (!platforms.empty()) {
            cmd += " --platform " + platforms;
        }
        cmd += " -t " + fullImage + " '" + localDir + "'";
        commands.push_back(cmd);

        if (doPush) {
            commands.push_back("podman push " + fullImage);
        }
    } else {
        if (useBuildx) {
            std::string cmd = "docker buildx build";
            if (!platforms.empty()) {
                cmd += " --platform " + platforms;
            }
            if (doPull) cmd += " --pull";
            if (noCache) cmd += " --no-cache";
            if (doPush) {
                cmd += " --push";
            }
            cmd += " -t " + fullImage + " '" + localDir + "'";
            commands.push_back(cmd);
        } else {
            std::string cmd = "docker build";
            if (doPull) cmd += " --pull";
            if (noCache) cmd += " --no-cache";
            cmd += " -t " + fullImage + " '" + localDir + "'";
            commands.push_back(cmd);

            if (doPush) {
                commands.push_back("docker push " + fullImage);
            }
        }
    }

    setBuildingState(true, "Building");
    appendLog("\n[INFO] Starting deployment sequence for " + fullImage);
    if (useBuildx) {
        appendLog("[INFO] Target Platforms: " + platforms);
    }
    appendLog("[INFO] Engine: " + engine);

    std::lock_guard<std::mutex> lock(m_threadMutex);
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }

    m_workerThread = std::thread([this, commands, fullImage]() {
        runCommandWorker(commands, fullImage);
    });
}

void MainWindow::runCommandWorker(const std::vector<std::string> &commands, const std::string &fullImage) {
    bool allSucceeded = true;

    for (size_t i = 0; i < commands.size(); ++i) {
        const auto &cmd = commands[i];
        appendLog("\n[CMD " + std::to_string(i + 1) + "/" + std::to_string(commands.size()) + "] " + cmd);

        int pipefd[2];
        if (pipe(pipefd) != 0) {
            appendLog("[ERROR] Failed to allocate communication pipe.");
            allSucceeded = false;
            break;
        }

        pid_t pid = fork();
        if (pid < 0) {
            appendLog("[ERROR] Failed to fork process.");
            close(pipefd[0]);
            close(pipefd[1]);
            allSucceeded = false;
            break;
        }

        if (pid == 0) {
            close(pipefd[0]);
            dup2(pipefd[1], STDOUT_FILENO);
            dup2(pipefd[1], STDERR_FILENO);
            close(pipefd[1]);
            setpgid(0, 0);

            execl("/bin/bash", "bash", "-c", cmd.c_str(), nullptr);
            _exit(127);
        }

        close(pipefd[1]);
        m_currentChildPid = pid;

        char buffer[1024];
        std::string lineAcc;
        ssize_t bytesRead;

        while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytesRead] = '\0';
            for (ssize_t b = 0; b < bytesRead; ++b) {
                if (buffer[b] == '\n') {
                    appendLog(lineAcc);
                    lineAcc.clear();
                } else if (buffer[b] == '\r') {
                } else {
                    lineAcc += buffer[b];
                }
            }
        }
        if (!lineAcc.empty()) {
            appendLog(lineAcc);
        }
        close(pipefd[0]);

        int status = 0;
        waitpid(pid, &status, 0);
        m_currentChildPid = 0;

        int exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
        if (exitCode != 0) {
            appendLog("[ERROR] Command failed with exit code " + std::to_string(exitCode));
            allSucceeded = false;
            break;
        }
    }

    if (allSucceeded) {
        appendLog("\n[SUCCESS] Image " + fullImage + " deployed successfully.");
        setBuildingState(false, "Success");
    } else {
        appendLog("\n[FAILURE] Deployment stopped due to errors.");
        setBuildingState(false, "Failed");
    }
}

void MainWindow::onCancelBuildClicked() {
    pid_t pid = m_currentChildPid.load();
    if (pid > 0) {
        appendLog("[INFO] Cancelling build process (PID: " + std::to_string(pid) + ")...");
        kill(-pid, SIGTERM);
        std::thread([pid]() {
            usleep(1500000);
            kill(-pid, SIGKILL);
        }).detach();
    }
}

void MainWindow::showAlert(const std::string &title, const std::string &message) {
#if GTK_CHECK_VERSION(4, 10, 0)
    GtkAlertDialog *alert = gtk_alert_dialog_new("%s", title.c_str());
    gtk_alert_dialog_set_detail(alert, message.c_str());
    gtk_alert_dialog_show(alert, GTK_WINDOW(m_window));
    g_object_unref(alert);
#else
    GtkWidget *dialog = gtk_message_dialog_new(
        GTK_WINDOW(m_window),
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_INFO,
        GTK_BUTTONS_OK,
        "%s",
        title.c_str()
    );
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s", message.c_str());
    g_signal_connect(dialog, "response", G_CALLBACK(+[](GtkDialog *d, int, gpointer) {
        gtk_window_destroy(GTK_WINDOW(d));
    }), nullptr);
    gtk_window_present(GTK_WINDOW(dialog));
#endif
}