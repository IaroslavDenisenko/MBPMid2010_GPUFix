#include "mainwindow.h"
#include "ui_mainwindow.h"

//#define TEST
#define REAL

//#define WINDOWS
#define MAC

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::exit()
{
    close();
    qApp->quit();
}

void MainWindow::on_patchButton_clicked()
{
    bool ok;
    QProcess process;
    QString errorOutput;
    QString command;
    QInputDialog *passwordDialog;
    passwordDialog = new QInputDialog;
    QString passwordDialogLabel = "Enter User Password :";
    bool isErrorPatching=false;

    //Search kext file
    if(searchKernelExtensionFile(&kernelFile))
    {
        //Display Warning Message
#ifndef TEST
        int answer = QMessageBox::question(this, "Warning", "This will patch the kernel configuration file.\nAre you sure you want to procede ?", QMessageBox::Yes | QMessageBox::No);
        if(answer == QMessageBox::Yes)
#else
        if(1)
#endif
        {
            do
            {
#ifndef TEST
                password = passwordDialog->getText(this,tr("Password"),passwordDialogLabel,QLineEdit::Password,"",&ok);

                command = "sudo -S pwd";
                process.start(command);

                process.write(password.toLocal8Bit());
                process.write("\n");
                process.waitForFinished(1000);
                errorOutput = process.readAllStandardError();
                qDebug() << errorOutput;
                process.closeWriteChannel();

                if(errorOutput.contains("try again"))
                {
                    qDebug() << "Wrong password, try again.\n";
                    passwordDialogLabel="Wrong password, try again.\nEnter User Password :";
                }
#else
                ok = 1;
#endif

                if(!ok)
                {
                    process.close();
                    return;
                }
            }while(errorOutput.contains("try again"));

            logger->write(" ********** Starting MBP GPU Fix **********\n");
            //Disable signing extension verification
            disableKextSigning();
            
            //Mount system with write permission
            if(!mountSystemWritePermission())
            {
                logger->write("********************* MBP GPU Fix FAILED *********************\n");
                return;
            }

            isErrorPatching = patchKernelExtensionFile(&kernelFile);
#ifndef TEST
            if(isErrorPatching == false)
            {
                loadKernelExtension(&kernelFile);
            }
            else
            {
                logger->write("********************* MBP GPU Fix FAILED *********************\n");
            }
#endif
        }
        else
        {
            logger->write(" ********** Discarded MBP GPU Fix **********\n");
            return;
        }
    }
    else
    {
        return;
    }
}

void MainWindow::on_restoreButton_clicked()
{
    //Display Warning Message
    int answer = QMessageBox::question(this, "Warning", "This will restore the old configuration.\nAre you sure you want to procede ?", QMessageBox::Yes | QMessageBox::No);

    if (answer == QMessageBox::Yes)
    {

    }
    else
    {
        return;
    }
}

bool MainWindow::init()
{
    bool isInitOk = true;
    MainWindow::setWindowTitle (APP_NAME);

    //TODO : enable button when functionnality will be avaible
    ui->restoreButton->setEnabled(false);

    //Initialize logger
    if (QFile::exists("log.txt"))
    {
        QFile::remove("log.txt");
    }
    QString fileName = "log.txt";
    logger = new Logger(this, fileName, this->ui->logWindow);
    logger->setShowDateTime(false);

    //Configure GitHub icom



    QString gitHubLogoPath = ":/ressource/githubicon.png";
    QPixmap pix = QPixmap (gitHubLogoPath);
    //int width = this->ui->labelgithubIcon->width();
    //int height = this->ui->labelgithubIcon->height();
    //pix = pix.scaled(width, height,Qt::KeepAspectRatio, Qt::SmoothTransformation);

    QIcon gitHubButtonIcon(pix);
    this->ui->gitHubButton->setIcon(gitHubButtonIcon);
    this->ui->gitHubButton->setIconSize(pix.rect().size());
    this->ui->gitHubButton->setFixedSize(pix.rect().size());
    //this->ui->labelgithubIcon->setText("<a href=\"https://github.com/julian-poidevin/MBPMid2010_GPUFix/\">GitHub Link</a>");
    //this->ui->labelgithubIcon->setTextFormat(Qt::RichText);
    //this->ui->labelgithubIcon->setTextInteractionFlags(Qt::TextBrowserInteraction);
    //this->ui->labelgithubIcon->setOpenExternalLinks(true);

    //Configure version label
    QString versionNumber = VERSION;
    QString versionPrefix = "v";
    QString versionName = versionPrefix + versionNumber;
    this->ui->versionButton->setText(versionName);

    //Search for compatibility
    if(isCompatibleVersion(getMBPModelVersion()))
    {
        isInitOk = true;
        logger->write("➔ Compatibility : OK ✓\n");
    }
    else
    {
        logger->write("➔ Compatibility : NOK ✗\n");
        QMessageBox::information(this,"Mac not compatible","Sorry, your Mac is not compatible.\nThe application will close");
        isInitOk = false;
        return isInitOk;
    }

    //Search for SIP Status
    if(isSIPEnabled())
    {
        ui->patchButton->setEnabled(false);
        ui->restoreButton->setEnabled(false);

        QMessageBox msgBox;
        msgBox.setInformativeText("The System Integrity Protection is enabled\nPlease follow the instructions to disable it");
        msgBox.setWindowTitle("SIP Enabled");

        QAbstractButton* pButtonYes = msgBox.addButton(tr("Take me to tutorial"), QMessageBox::YesRole);
        msgBox.addButton(tr("Nope"), QMessageBox::NoRole);
        msgBox.setIcon(QMessageBox::Information);
        msgBox.exec();

        if (msgBox.clickedButton()== pButtonYes)
        {
            QString link = "https://www.youtube.com/watch?v=Wmhal4shmVo";
            QDesktopServices::openUrl(QUrl(link));
        }
    }

    return isInitOk;
}

QString MainWindow::getMBPModelVersion()
{
    QString MBPModelVersion;
    QProcess process;

    logger->write(" | Checking compatibility\n");

    //Execute commande line
    process.start("sysctl -n hw.model");

    //Wait forever until finished
    process.waitForFinished(-1);

    //Get command line output
    MBPModelVersion = process.readAllStandardOutput();

    //Close process
    process.close();

    //Remove carriage return ("\n") from string
    MBPModelVersion = MBPModelVersion.simplified();

    logger->write("MBPModelVersion :  " + MBPModelVersion);

    return MBPModelVersion;
}

bool MainWindow::isSIPEnabled(void)
{
    QString SIPStatus;
    QProcess process;
    QOperatingSystemVersion macVersion = QOperatingSystemVersion::current();

    logger->write(" | macOS version : \n");
    logger->write(macVersion.name() + " " + QString::number(macVersion.majorVersion()) + "." +  QString::number(macVersion.minorVersion()) + "\n");

    logger->write(" | Checking SIP Status\n");

    //SIP as been introduced since El Capitan
    if(macVersion >= QOperatingSystemVersion::OSXElCapitan)
    {
        //Execute commande line
        process.start("csrutil status");

        //Wait forever until finished
        process.waitForFinished(-1);

        //Get command line output
        SIPStatus = process.readAllStandardOutput();

        //Close process
        process.close();
    }
    else
    {
        logger->write("No SIP for this OS\n");
        return false;
    }

#ifndef WINDOWS
    if(SIPStatus.contains("disable"))
    {
        logger->write("SIP Disabled\n");
        return false;
    }
    else
    {
        logger->write("SIP Enabled\n");
        return true;
    }
#else
    return false;
#endif
}

int MainWindow::disableKextSigning()
{
    QProcess process;
    QString command;
    QStringList arguments;

    logger->write("Disabling Kext Signing verification : ");
    command = "sudo nvram boot-args=kext-dev-mode=1";
    arguments.clear();
    return executeProcess(&process,command,arguments);
}

int MainWindow::mountSystemWritePermission()
{
    QProcess process;
    QString command;
    QStringList arguments;
    QOperatingSystemVersion macVersion = QOperatingSystemVersion::current();

    //This operation is only needed for MacOS version >= 10.15
    if(macVersion >= QOperatingSystemVersion::MacOSCatalina)
    {
        logger->write("Mounting system with write permissions : ");
        command = "sudo mount -uw /";
        arguments.clear();
        return executeProcess(&process,command,arguments);
    }
}

int MainWindow::executeProcess(QProcess* process, QString command, QStringList arguments)
{
    QString errorOutput;
    QString stdOutput;

    //Execute commande line
    if (arguments.isEmpty())
    {
        process->start(command);
    }
    else
    {
        process->start(command,arguments);
    }

    process->write(password.toLocal8Bit());
    process->write("\n");
    process->waitForFinished(1000);
    errorOutput = process->readAllStandardError();
    stdOutput = process->readAllStandardOutput();
    process->closeWriteChannel();

    if (process->exitCode() == 0)
    {
        logger->write("✓ " + stdOutput + "\n");
        return 0;
    }
    else
    {
        qDebug() << errorOutput;
        logger->write("✗ : " + errorOutput + "\n");
        errorOutput.clear();
        return -1;
    }

    //Close process
    process->close();
}


//Parse system directory searching for AppleGraphicsPowerManagement.kext file
bool MainWindow::searchKernelExtensionFile(QFile* kernelExtensionFile)
{
    bool isFileFound;

#ifdef TEST
#ifdef MAC
    QDir kextPath("/Users/Julian/Documents/Dev/Projects/MBPMid2010_GPUFix/");
#endif
#ifdef WINDOWS
    QDir kextPath("C:/Users/jpoidevin/Desktop/Documents Pro/03 - Dev Temp/MBPMid2010_GPUFix/MBPMid2010_GPUFix/");
#endif
#endif
#ifdef REAL
    QDir kextPath("/System/Library/Extensions/AppleGraphicsPowerManagement.kext/");
#endif

    QStringList listOfFiles;

    //Print Current app directory
    qDebug() << "Current Dir :" <<kextPath.absolutePath();

    //Recursively search for "Info.plist" file in appPath
    QDirIterator it(kextPath.absolutePath(),
                    QStringList() << "Info.plist",
                    QDir::NoSymLinks | QDir::Files,
                    QDirIterator::Subdirectories);

    logger->write(" | Searching for AppleGraphicsPowerManagement.kext\n");

    //Check if the file was found
    if(it.hasNext())
    {
        while(it.hasNext())
        {
            it.next();
            if (it.filePath().contains("AppleGraphicsPowerManagement"))
            {
                listOfFiles.push_back(it.filePath());
            }
        }
    }

    //Print files found
    qDebug() << "Files found :"<< listOfFiles;

    if(listOfFiles.length() <= 1 && listOfFiles.length() > 0)
    {
        //qDebug() << "Moins de 1";
        logger->write("AppleGraphicsPowerManagement.kext found\n");
        kernelExtensionFile->setFileName(listOfFiles.at(0));
        isFileFound = true;
    }
    else
    {
        //qDebug () << "No file was found...";
        logger->write("AppleGraphicsPowerManagement.kext not found\n");
        isFileFound = false;
    }

    //Start search manually and only allow loading of the perfect named file (or kext)
    if(!isFileFound)
    {
        QMessageBox::information(this,"File not found","Any corresponding file was found, please search for the file");

        //TODO : FileDialog won't let user browse into .kext files Contents
        QString dir = QFileDialog::getOpenFileName(this, tr("Open Info.plist file"),
                                                   "/System/Library/Extensions/AppleGraphicsPowerManagement.kext/",
                                                   "Property List Files (Info.plist)");
        if(!(dir.isNull()))
        {
            //kernelExtensionFile->setFileName(dir);
            isFileFound = true;
        }
        else
        {
            isFileFound = false;
        }
    }

    return isFileFound;
}

bool MainWindow::isCompatibleVersion(QString modelVersion)
{
    //Compare version with compatible versions of MBPs
    bool isCompatibleVersion;

#ifdef MAC

    //TODO : Search in a list if several models compatible
    if(modelVersion == "MacBookPro6,2")
    {
        isCompatibleVersion = true;
    }
    else
    {
        isCompatibleVersion = false;
    }

#endif
#ifdef WINDOWS
    isCompatibleVersion = true;
#endif

    return isCompatibleVersion;
}

void MainWindow::backupOldKernelExtension()
{
    //Save File to current location adding .bak extension
    //qDebug() << "File Name" << kernelFile.fileName();

    //Save original file in kernelExtension file folder
    QFile::copy(kernelFile.fileName(), kernelFile.fileName() + ".bak");
}

bool MainWindow::patchKernelExtensionFile(QFile *kernelFile)
{
    //TODO
    //backupOldKernelExtension();

#ifdef MAC
#define PATCHED_FILE_PATH "/tmp/PatchedInfo.plist"
#endif
#ifdef WINDOWS
#define PATCHED_FILE_PATH "C:/temp/PatchedInfo.plist"
#endif

    bool isErrorPatching = false;

    logger->write("Copying Info.plist file\n");

    //Remove file if already exists
    if (QFile::exists(PATCHED_FILE_PATH))
    {
        QFile::remove(PATCHED_FILE_PATH);
        logger->write("Previous Info.plist file removed\n");
    }

    //Copy file in tmp dir for patch
    QFile::copy(kernelFile->fileName(), PATCHED_FILE_PATH);

    QFile tmpFile(PATCHED_FILE_PATH);
    if(!tmpFile.open(QIODevice::ReadWrite | QIODevice::Text))
    {
        logger->write("Could not open Info.plist file\n");
        qDebug() << "Could not open tmp File";
        isErrorPatching = true;
        return isErrorPatching;
    }

    //The QDomDocument class represents an XML document.
    QDomDocument xmlBOM;

    // Set data into the QDomDocument before processing
    xmlBOM.setContent(&tmpFile);

    /* Definition of struct and enum to automaticaly parse file*/
    typedef enum
    {
        FindChild,
        FindSibling,
        NextSibling,
        FirstChild,
        ModifyIntValue,
        FillArray,
        RemoveSibling,
    }EActions;

    typedef struct{
        QString nodeName;
        QVector<int> ArrayValues;
        EActions ActionToPerform;
    }nodeTree;

    QVector<nodeTree> confTree={
        {"MacBookPro6,2"        , {}            , FindChild      },
        {"dict"                 , {}            , NextSibling    },
        {"LogControl"           , {}            , FindChild      },
        {""                     , {1}           , ModifyIntValue },
        {"Vendor10deDevice0a29" , {}            , FindSibling    },
        {"BoostPState"          , {}            , FindSibling    },
        {""                     , {2,2,2,2}     , FillArray      },
        {"BoostTime"            , {}            , FindSibling    },
        {""                     , {2,2,2,2}     , FillArray      },
        {"Heuristic"            , {}            , FindSibling    },
        {"IdleInterval"         , {}            , FindSibling    },
        {""                     , {10}          , ModifyIntValue },
        {"P3HistoryLength"      , {}            , RemoveSibling  },
        {"SensorSampleRate"     , {}            , FindSibling    },
        {""                     , {10}          , ModifyIntValue },
        {"Threshold_High"       , {}            , FindSibling    },
        {""                     , {0,0,100,200} , FillArray      },
        {"Threshold_High_v"     , {}            , FindSibling    },
        {""                     , {0,0,98,100}  , FillArray      },
        {"Threshold_Low"        , {}            , FindSibling    },
        {""                     , {0,0,0,200}   , FillArray      },
        {"Threshold_Low_v"      , {}            , FindSibling    },
        {""                     , {0,0,4,200}   , FillArray      }
    };

    logger->write("Patching Info.plist\n");

    QDomElement currentNode = xmlBOM.firstChildElement("plist");
    QDomElement nextNode;
    QDomElement removedNode;

    for (int i = 0;(i < confTree.size()) && (isErrorPatching == false); ++i)
    {
        //qDebug() << confTree.at(i).nodeName << confTree.at(i).ActionToPerform;
        switch (confTree.at(i).ActionToPerform){
        case FindChild:
            nextNode = findElementChild(currentNode,confTree.at(i).nodeName);
            if(!nextNode.isNull())
            {
                qDebug() << "FindChild - " << nextNode.tagName() << "|" << nextNode.text();
                logger->write(" - FindChild  - " + nextNode.tagName() + "|" + nextNode.text() + "\n");
            }
            else
            {
                isErrorPatching = true;
                qDebug() << "FindChild - ERROR \n";
                logger->write(" - FindChild  - ERROR \n");
            }
            break;

        case FindSibling:
            nextNode = findElementSibling(currentNode,confTree.at(i).nodeName);
            if(!nextNode.isNull())
            {
                qDebug() << "FindSibling - " << nextNode.tagName() << "|" << nextNode.text();
                logger->write(" - FindSibling  - " + nextNode.tagName() + "|" + nextNode.text() + "\n");
            }
            else
            {
                isErrorPatching = true;
                qDebug() << "FindSibling - ERROR \n";
                logger->write(" - FindSibling  - ERROR \n");
            }
            break;

        case NextSibling:
            nextNode = currentNode.nextSiblingElement(confTree.at(i).nodeName);
            if(!nextNode.isNull())
            {
                qDebug() << "NextSibling - " << nextNode.tagName();
                logger->write(" - NextSibling  - " + nextNode.tagName() + "\n");
            }
            else
            {
                isErrorPatching = true;
                qDebug() << "NextSibling - ERROR \n";
                logger->write(" - NextSibling  - ERROR \n");
            }
            break;

        case FirstChild:
            nextNode = currentNode.firstChildElement(confTree.at(i).nodeName);
            if(!nextNode.isNull())
            {
                qDebug() << "FirstChild - " << nextNode.tagName();
                logger->write(" - FirstChild  - " + nextNode.tagName() + "\n");
            }
            else
            {
                isErrorPatching = true;
                qDebug() << "FirstChild - ERROR \n";
                logger->write(" - FirstChild  - ERROR \n");
            }
            break;

        case ModifyIntValue:
            currentNode = currentNode.nextSiblingElement("integer");

            if(!currentNode.isNull())
            {
                currentNode.firstChild().setNodeValue(QString::number(confTree.at(i).ArrayValues[0]));
                //qDebug() << "Integer - " << currentNode.firstChild().nodeValue();
                //logger->write(" - Integer  - " + currentNode.firstChild().nodeValue() + "\n");

                nextNode = currentNode;

                qDebug() << "ModifyIntValue - " << nextNode.tagName() << "|" << nextNode.text();
                logger->write(" - ModifyIntValue  - " + nextNode.tagName() + "|" + nextNode.text() + "\n");
            }
            else
            {
                isErrorPatching = true;
                qDebug() << "ModifyIntValue - ERROR \n";
                logger->write(" - ModifyIntValue  - ERROR \n");
            }

            break;

        case FillArray:

            currentNode = currentNode.nextSiblingElement("array").firstChildElement("integer");

            if(!currentNode.isNull())
            {
                currentNode.firstChild().setNodeValue(QString::number(confTree.at(i).ArrayValues[0]));
                currentNode.nextSibling().firstChild().setNodeValue(QString::number(confTree.at(i).ArrayValues[1]));
                currentNode.nextSibling().nextSibling().firstChild().setNodeValue(QString::number(confTree.at(i).ArrayValues[2]));
                currentNode.nextSibling().nextSibling().nextSibling().firstChild().setNodeValue(QString::number(confTree.at(i).ArrayValues[3]));

                nextNode = currentNode.parentNode().toElement();
            }
            else
            {
                isErrorPatching = true;
                qDebug() << "FillArray - ERROR \n";
                logger->write(" - FillArray  - ERROR \n");
            }

            break;

        case RemoveSibling:
            removedNode = findElementSibling(currentNode,confTree.at(i).nodeName);

            if(!removedNode.isNull())
            {
                qDebug() << "RemoveSiblingLabel - " << removedNode.tagName() << "|" << removedNode.text();
                logger->write(" - RemoveSiblingLabel  - " + removedNode.tagName() + "|" + removedNode.text() + "\n");

                currentNode.parentNode().removeChild(removedNode);

                removedNode = currentNode.nextSiblingElement("integer");

                if(!removedNode.isNull())
                {
                    qDebug() << "RemoveSiblingValue - " << removedNode.tagName() << "|" << removedNode.text();
                    logger->write(" - RemoveSiblingValue  - " + removedNode.tagName() + "|" + removedNode.text() + "\n");

                    currentNode.parentNode().removeChild(removedNode);
                }
                else
                {
                    qDebug() << "RemoveSiblingValue - Not found";
                    logger->write(" - RemoveSiblingValue  - Not found");
                }
            }
            else
            {
                qDebug() << "RemoveSiblingLabel - " << confTree.at(i).nodeName << "Not found";
                logger->write(" - RemoveSiblingLabel - " + confTree.at(i).nodeName +  " Not found \n");
            }

            break;

        default:
            break;
        }

        currentNode = nextNode;
    }

    if(isErrorPatching != true)
    {
        logger->write("Info.plist successfully patched\n");

        // Write changes to same file
        tmpFile.resize(0);
        QTextStream stream;
        stream.setDevice(&tmpFile);
        xmlBOM.save(stream, 4);
    }
    else
    {
        logger->write("Info.plist patching failed\n");
    }

    tmpFile.close();

    return isErrorPatching;
}

int MainWindow::loadKernelExtension(QFile *kernelFile)
{
    //Use Kext Utility or command lines utils to load the file in Kernel
    //See here : http://osxdaily.com/2015/06/24/load-unload-kernel-extensions-mac-os-x/

    int processStatus = 0;

    logger->write(" | Loading Kernel Extension\n");

    /* Copy real kext into tmp file */
    QProcess process;
    //QProcess *process = new QProcess(this);
    QString command;
    QStringList arguments;
    QDir kextDir(kernelFile->fileName());
    kextDir.cdUp();
    kextDir.cdUp();

    logger->write("Removing existing kext in tmp : ");
    command = "sudo -S rm -rf /tmp/AppleGraphicsPowerManagement.kext";
    arguments.clear();
    processStatus |= executeProcess(&process,command,arguments);

    logger->write("Copying actuel kext into tmp : ");
    command = "sudo -S cp -rf /System/Library/Extensions/AppleGraphicsPowerManagement.kext /tmp/AppleGraphicsPowerManagement.kext";
    arguments.clear();
    processStatus |= executeProcess(&process,command,arguments);

    logger->write("Copying patched Info.plist into kext : ");
    /*** Copy patched file into kext ***/
    command = "sudo -S cp -f /tmp/PatchedInfo.plist /tmp/AppleGraphicsPowerManagement.kext/Contents/Info.plist";
    arguments.clear();
    //Execute commande line
    processStatus |= executeProcess(&process,command,arguments);

    logger->write("Changing permission of kext : ");
    /*** Change permission of modified kext File ***/
    command = "sudo -S chown -R -v root:wheel /tmp/AppleGraphicsPowerManagement.kext";
    arguments.clear();
    //Execute commande line
    processStatus |= executeProcess(&process,command,arguments);

    logger->write("Removing existing kext : ");
    command = "sudo -S rm -rf /System/Library/Extensions/AppleGraphicsPowerManagement.kext";
    arguments.clear();
    processStatus |= executeProcess(&process,command,arguments);

    logger->write("Copying patched kext into Extension : ");
    /*** Copy patched file into kext ***/
    command = "sudo -S cp -rf /tmp/AppleGraphicsPowerManagement.kext /System/Library/Extensions/AppleGraphicsPowerManagement.kext";
    arguments.clear();
    //Execute commande line
    processStatus |= executeProcess(&process,command,arguments);

    logger->write("Loading modified kext : ");
    /*** Finally load kext file ***/
    command = "sudo -S kextload -v /tmp/AppleGraphicsPowerManagement.kext";
    arguments.clear();
    //Execute commande line
    processStatus |= executeProcess(&process,command,arguments);

    if(processStatus == 0)
    {
        logger->write("********************* MBP GPU Fixed Successfully *********************\n");
    }
    else
    {
        logger->write("********************* MBP GPU Fix FAILED *********************\n");
    }

    //Close process
    process.close();

    ui->patchButton->setEnabled(false);

    return processStatus;
}

int MainWindow::restoreOldKernelExtension(QFile *kernelFile)
{
    //Restore.bak extension
    int Status = 0;

    //QFile::copy(kernelFile->fileName()  + ".bak", kernelFile->fileName());

    return Status;
}

QDomElement MainWindow::findElementChild(QDomElement parent, const QString &textToFind)
{
    for(QDomElement elem = parent.firstChildElement(); !elem.isNull(); elem = elem.nextSiblingElement())
    {
        if(elem.text()==textToFind) return elem;

        QDomElement e = findElementChild(elem, textToFind);

        if(!e.isNull()) return e;
    }

    return QDomElement();
}

QDomElement MainWindow::findElementSibling(QDomElement parent, const QString &textToFind)
{
    for(QDomElement elem = parent.nextSiblingElement(); !elem.isNull(); elem = elem.nextSiblingElement())
    {
        if(elem.text()==textToFind) return elem;

        QDomElement e = findElementChild(elem, textToFind);

        if(!e.isNull()) return e;
    }

    return QDomElement();
}

void MainWindow::on_gitHubButton_clicked()
{
    QString link = "https://github.com/julian-poidevin/MBPMid2010_GPUFix";
    QDesktopServices::openUrl(QUrl(link));

    return;
}

void MainWindow::on_versionButton_clicked()
{
    QString link = "http://github.com/julian-poidevin/MBPMid2010_GPUFix/blob/master/CHANGELOG.md";
    QDesktopServices::openUrl(QUrl(link));

    return;
}

void MainWindow::on_pushButton_clicked()
{
    QString link = "https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=VR3QQDC6GMDCQ";
    QDesktopServices::openUrl(QUrl(link));

    return;
}
