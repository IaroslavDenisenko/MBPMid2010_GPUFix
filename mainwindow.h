#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#define VERSION "0.6.1"
#define APP_NAME "MBPMid2010-GPU-Fix"

#include <QMainWindow>
//#include <QWidget>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QFile>
#include <QTextStream>
#include <QApplication>
#include <QProcess>
#include <iostream>
#include <QDir>
#include <QDebug>
#include <QDirIterator>
#include <QStack>
#include <QFileDialog>
#include <QSettings>
#include <QList>
#include <QtXml>
#include <QDesktopServices>
#include <QLibrary>
#include <QInputDialog>
#include <QLabel>
#include <QPixmap>
#include <QSysInfo>

#include "logger.h"

using namespace std; // Indique quel espace de noms on va utiliser

//GoogleC++ Style Guide : https://google.github.io/styleguide/cppguide.html

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    bool init();

private slots:
    void on_patchButton_clicked();
    void on_restoreButton_clicked();
    void on_gitHubButton_clicked();
    void on_versionButton_clicked();
    void on_pushButton_clicked();

    void exit();

private:
    Ui::MainWindow *ui;

    QFile kernelFile;
    Logger *logger;
    QString password;

    QString getMBPModelVersion(void);
    bool isCompatibleVersion(QString modelVersion);
    bool searchKernelExtensionFile(QFile* kernelExtensionFile);
    void backupOldKernelExtension(void);
    bool patchKernelExtensionFile(QFile* kernelFile);
    int loadKernelExtension(QFile* kernelFile);
    int restoreOldKernelExtension(QFile* kernelFile);
    QDomElement findElementChild(QDomElement parent, const QString &textToFind);
    QDomElement findElementSibling(QDomElement parent, const QString &textToFind);
    bool isSIPEnabled(void);
    int disableKextSigning(void);
    int mountSystemWritePermission(void);
    int executeProcess(QProcess* process,QString command,QStringList arguments);
};

#endif // MAINWINDOW_H
