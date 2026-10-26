#include "mainwindow.h"
#include <adwaita.h>

int main(int argc, char *argv[]) {
    g_autoptr(AdwApplication) app = adw_application_new("com.github.docker_auto_updater", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(MainWindow::onAppActivate), nullptr);
    return g_application_run(G_APPLICATION(app), argc, argv);
}
