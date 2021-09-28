#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "dragdropfilearea.h"
#include "installthread.h"
#include "pkgdetaildialog.h"

#include <QDesktopWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <string>
#include <QProcess>
#include <QStringList>
#include <QSvgRenderer>
#include <QPainter>
#include <QTextEdit>
#include <QScrollArea>

MainWindow::MainWindow(int argc, char **argv, QWidget *parent) :   // 直接通过构造器来搞
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("RPM 安装器");
    setWindowIcon(QPixmap(":/box-seam.png"));

    // 下面两行让窗口创建在屏幕正中间
    QDesktopWidget *desktop = QApplication::desktop();
    setFixedSize(550,350);
    move((desktop->width()-this->width())/2,(desktop->height()-this->height())/2);

    delete ui->mainToolBar;         // 不需要 toolbar ，删掉
    delete ui->statusBar;

    this->setArgc(argc);
    this->getArgc();
    if(argc==1) {
        // 没有启动参数输入，应该是啥也不干. 选择文件安装的界面应该在这里面绘制
        QWidget *mainContentWid = new QWidget;          // MainWindow 中的 Content，其实是个装 QVBoxLayout 的容器
                                                        // 这玩意的正确用法，或许应该是单拿出去做个类，和 main 拆开

        QVBoxLayout *mainLayout = new QVBoxLayout;      // 定义 QVBoxLayout
        mainContentWid->setLayout(mainLayout);          // 设置布局

        QPixmap *pixmap = new QPixmap();                // 多种格式的图片都可以存为 pixmap

        QSvgRenderer svgRender(QString(":/icon_install_light_resized.svg"));   // 使用 painter 和 renderer
        QImage image(200, 200, QImage::Format_ARGB32);
        image.fill(0x00FFFFFF);
        QPainter painter(&image);
        svgRender.render(&painter);

        pixmap->convertFromImage(image);

        DragDropFileArea *installIconLabel = new DragDropFileArea();
        installIconLabel->setPixmap(*pixmap);
        connect(installIconLabel, &DragDropFileArea::fileDropped, this, &MainWindow::dropFileHandler);

        QPixmap splitLinePixmap;
        splitLinePixmap.load(":/split_line.svg");
        QLabel * splitLine = new QLabel();
        splitLine->setPixmap(splitLinePixmap);

        QLabel *dropHereText = new QLabel();
        dropHereText->setText("拖拽 rpm 包到此");

        // 按钮尺寸相关见：https://blog.csdn.net/hyongilfmmm/article/details/83015729
        QPushButton *buttonSelectRPM = new QPushButton("选择 rpm 包文件");
        buttonSelectRPM->setStyleSheet("QPushButton{color:#0099FF; background-color:transparent; font-size:12px}");
        buttonSelectRPM->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
        buttonSelectRPM->setFixedWidth(200);

        // 通过这种方式，把按钮的事件（clicked）和处理函数“连接”起来
        connect(buttonSelectRPM, &QPushButton::clicked, this, &MainWindow::selectRpmBtnHandler);

        mainLayout->addWidget(installIconLabel, 0, Qt::AlignCenter);
        mainLayout->addWidget(dropHereText, 0, Qt::AlignCenter);
        mainLayout->addSpacing(10);
        mainLayout->addWidget(splitLine, 0, Qt::AlignCenter);
        mainLayout->addSpacing(10);
        mainLayout->addWidget(buttonSelectRPM, 0, Qt::AlignCenter);
        mainLayout->addStretch();

        this->setCentralWidget(mainContentWid);

    } else {
        // 这里有参数输入了，先不管连续输入多个的情况，只管装第一个
        this->setArgPath(std::string(argv[1]));
        loadRpmInfo();
    }

}

MainWindow::~MainWindow()
{
    delete ui;
    delete installThread;
    installThread = nullptr;
}

bool MainWindow::setArgc(int argsC)
{
    this->argCount=argsC;
    return true;
}

int MainWindow::getArgc()
{
    qDebug("well:%d", this->argCount);
    return this->argCount;
}

bool MainWindow::setArgPath(std::string argPath)
{
    this->argPath=argPath;
    return true;
}

std::string MainWindow::getArgPath()
{
    qDebug("this->argPath = %s", this->argPath.c_str());
    return this->argPath;
}

std::string MainWindow::selectRpmBtnHandler()
{
    std::string res = "";
    QString fileName = QFileDialog::getOpenFileName(nullptr, "打开文件", QDir::homePath(), "*.rpm");
    bool isSelectSuccess = true;
    if(fileName.isEmpty()) {
        isSelectSuccess = false;
    }
    if(isSelectSuccess) {   // 选择成功的情况
        this->setArgPath(fileName.toStdString());
        loadRpmInfo();      // 开始加载信息，显示加载中界面
    } else {
        // 这里留给没选择 RPM 包的情况
    }

    return res;
}

void MainWindow::sendNotify()
{
    QProcess process;
    process.start("notify-send", QStringList() << QString(this->argPath.c_str()));
    process.waitForFinished();

    QString res = process.readAllStandardOutput();
    qDebug("%s", qPrintable(res));
}

void MainWindow::installRPM()
{
    qDebug("=====begin install=====");
    QWidget *installingWid = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout;
    installingWid->setLayout(mainLayout);
    QLabel *installingText = new QLabel("安装中，日志如下");

    textArea = new QTextEdit();
    textArea->setReadOnly(true);
    textArea->setFixedWidth(780);
    textArea->setFixedHeight(420);

    mainLayout->addStretch();
    mainLayout->addWidget(installingText, 0, Qt::AlignCenter);
    mainLayout->addSpacing(5);
    mainLayout->addWidget(textArea, 0, Qt::AlignCenter);
    mainLayout->addStretch();

    setCentralWidget(installingWid);

    installThread = new InstallThread(ThreadMode::installPackage, rpmArray);
    installThread->start();

    connect(installThread, &InstallThread::appendLog, this, &MainWindow::appendLog);
    connect(installThread, &InstallThread::updatePkgInfo, this, &MainWindow::updateRPMArray);
    connect(installThread, &InstallThread::installFinish, this, &MainWindow::onInstallFinish);

}

void MainWindow::updateRPMArray(QVector<RPMInfoStruct> rpmArray)
{
    this->rpmArray = rpmArray;
}

void MainWindow::onInstallFinish()
{
    qDebug("=====finished=====");
    QWidget *installedWid = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout;
    installedWid->setLayout(mainLayout);
    QLabel *installedText = new QLabel(rpmArray[0].actionNotify);
    QPushButton *finishBtn = new QPushButton("完成");
    finishBtn->setFixedWidth(200);
    connect(finishBtn, &QPushButton::clicked, this, &MainWindow::exitOnFinished);

    mainLayout->addStretch();
    mainLayout->addWidget(installedText, 0, Qt::AlignCenter);
    mainLayout->addSpacing(15);
    mainLayout->addWidget(textArea, 0, Qt::AlignCenter);
    mainLayout->addSpacing(15);
    mainLayout->addWidget(finishBtn, 0, Qt::AlignCenter);
    mainLayout->addStretch();
    setCentralWidget(installedWid);
}

void MainWindow::appendLog(QString log)
{
    if(log.isEmpty()) return;
    textArea->append(log);
    textArea->moveCursor(QTextCursor::End);
}

QString MainWindow::getRPMSummary()
{
    QString summary = QString("");
    std::string rpmInfoStr = "";
    QProcess process;
    process.start("rpm", QStringList() << "-qip" << QString(this->argPath.c_str()));
    process.waitForFinished();
    rpmInfoStr = process.readAllStandardOutput().toStdString();
    size_t summaryIndex = rpmInfoStr.find("\nSummary");
    size_t descriptionIndex = rpmInfoStr.find("Description")-1;

    rpmInfoStr = rpmInfoStr.substr(summaryIndex+1, descriptionIndex-summaryIndex);
    rpmInfoStr = rpmInfoStr.substr(rpmInfoStr.find(": ")+2);
    std::string::iterator infoEnd = rpmInfoStr.end();

    for(std::string::iterator iter=rpmInfoStr.begin(); iter!=infoEnd; iter++) {
        if(*iter=='\n') *iter=' ';
    }

    summary = QString::fromStdString(rpmInfoStr);
    qDebug("%d", summary.length());
    qDebug("%s", qPrintable(summary));

    return summary;
}

void MainWindow::loadRpmInfo()
{
    QWidget *loadingWid = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout;
    loadingWid->setLayout(mainLayout);
    QLabel *loadingLabel = new QLabel("加载中，请稍候");
    mainLayout->addStretch();
    mainLayout->addWidget(loadingLabel, 0, Qt::AlignCenter);
    mainLayout->addStretch();

    setCentralWidget(loadingWid);

    RPMInfoStruct tmp;
    tmp.dir = QString::fromStdString(argPath);
    rpmArray.push_back(tmp);

    installThread = new InstallThread(ThreadMode::getInfo, rpmArray);
    installThread->start();
    connect(installThread, &InstallThread::multiPkgInfo, this, &MainWindow::onRPMInfoLoaded);

}

void MainWindow::onRPMInfoLoaded(QVector<RPMInfoStruct> rpmArray) {
    delete installThread;
    installThread = nullptr;

    this->rpmArray = rpmArray;

    // 调整窗口尺寸，适配新的横版 UI
    QDesktopWidget *desktop = QApplication::desktop();
    setFixedSize(800,550);
    move((desktop->width()-this->width())/2,(desktop->height()-this->height())/2);

    QWidget *mainContentWid = new QWidget;          // MainWindow 中的 Content，其实是个装 QVBoxLayout 的容器
    QHBoxLayout *mainLayout = new QHBoxLayout();      // 定义 QVBoxLayout
    mainContentWid->setLayout(mainLayout);          // 设置布局
    mainLayout->setMargin(0);

    QPixmap pixmap;

    QSvgRenderer svgRender(QString(":/rpm.svg"));   // 使用 painter 和 renderer
    QImage image(120, 120, QImage::Format_ARGB32);
    image.fill(0x00FFFFFF);
    QPainter painter(&image);
    svgRender.render(&painter);

    pixmap.convertFromImage(image);

    QLabel *installIconLabel = new QLabel();
    installIconLabel->setFixedSize(QSize(120, 100));
    installIconLabel->setPixmap(pixmap);

    QWidget *leftFrame = new QWidget();
    leftFrame->setFixedWidth(300);
    QVBoxLayout *leftFrameLayout = new QVBoxLayout(leftFrame);

    QLabel *titleText = new QLabel();
    titleText->setText(subHeaderStyle(this->rpmArray[0].name));

    leftFrameLayout->addStretch(1);
    leftFrameLayout->addWidget(installIconLabel, 0, Qt::AlignCenter);
    leftFrameLayout->addSpacing(20);
    leftFrameLayout->addWidget(titleText, 0, Qt::AlignCenter);
    leftFrameLayout->addSpacing(40);

    QPushButton *buttonInstallRPM = new QPushButton();
    buttonInstallRPM->setFixedWidth(200);
    if(rpmArray[0].status==readyToInstall) {
        buttonInstallRPM->setText("开始安装");
        connect(buttonInstallRPM, &QPushButton::clicked, this, &MainWindow::installRPM);
    } else {
        QLabel *cannotInstallDescription = new QLabel(this->rpmArray[0].statusDescription+"   "+this->rpmArray[0].actionNotify);
        cannotInstallDescription->setStyleSheet("QLabel{font-size:16px; color: green}");
        buttonInstallRPM->setText("完成");
        connect(buttonInstallRPM, &QPushButton::clicked, this, &MainWindow::exitOnFinished);
        leftFrameLayout->addWidget(cannotInstallDescription, 0, Qt::AlignCenter);
    }

    leftFrameLayout->addWidget(buttonInstallRPM, 0, Qt::AlignCenter);
    leftFrameLayout->addStretch(3);


    QWidget *detailFrame = new QWidget();

    QVBoxLayout *detailLayout = new QVBoxLayout(detailFrame);
    detailFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QLabel *archLabel = new QLabel(subHeaderStyle("架构"));
    QLabel *arch = new QLabel(marginStyle(rpmArray[0].arch));
    QLabel *licenseLabel = new QLabel(subHeaderStyle("开源协议"));
    QLabel *license = new QLabel(marginStyle(rpmArray[0].license));
    QLabel *summaryLabel = new QLabel(subHeaderStyle("简介"));
    QLabel *summary = new QLabel(marginStyle(rpmArray[0].summary));
    summary->setWordWrap(true);
    QLabel *descriptionLabel = new QLabel(subHeaderStyle("描述"));
    QLabel *description = new QLabel(marginStyle(rpmArray[0].description));
    description->setWordWrap(true);
    QLabel *versionReleaseLabel = new QLabel(subHeaderStyle("版本"));
    QLabel *versionRelease = new QLabel(marginStyle(rpmArray[0].versionRelease));
    QLabel *requiresLabel = new QLabel(subHeaderStyle("Requires"));
    QLabel *requiresList = new QLabel(styledList(rpmArray[0].pkgRequires));
    QScrollArea *requiresScrollArea = new QScrollArea();
    requiresScrollArea->setWidget(requiresList);
    QLabel *ProvidesLabel = new QLabel(subHeaderStyle("Requires"));
    QLabel *ProvidesList = new QLabel(styledList(rpmArray[0].pkgProvides));
    QScrollArea *providesScrollArea = new QScrollArea();
    providesScrollArea->setWidget(ProvidesList);



    detailLayout->addWidget(archLabel);
    detailLayout->addWidget(arch);
    detailLayout->addWidget(licenseLabel);
    detailLayout->addWidget(license);
    detailLayout->addWidget(summaryLabel);
    detailLayout->addWidget(summary);
    detailLayout->addWidget(descriptionLabel);
    detailLayout->addWidget(description);
    detailLayout->addWidget(versionReleaseLabel);
    detailLayout->addWidget(versionRelease);
    detailLayout->addWidget(requiresLabel);
    detailLayout->addWidget(requiresScrollArea, 1);
    detailLayout->addWidget(ProvidesLabel);
    detailLayout->addWidget(providesScrollArea, 1);
    detailLayout->addStretch();


    mainLayout->addWidget(leftFrame);
    mainLayout->addWidget(detailFrame);


    this->setCentralWidget(mainContentWid);

}

QString MainWindow::headerStyle(QString content)
{
    content = "<div style='font-size: xx-large; font-weight: bold'>"
            + content
            + "</div><hr>";
    return content;
}

QString MainWindow::subHeaderStyle(QString content)
{
    content = "<div style='font-weight: bold'>"
            + content
            + "</div>";
    return content;
}

QString MainWindow::marginStyle(QString content)
{
    QString ret;
    ret = "<div style='margin-left: 20'>";
    ret += content;
    ret += "</div>";
    return ret;
}

QString MainWindow::docblockStyle(QString content)
{
    QString ret = "<div style='margin-left: 20'><div style='border: 1px solid #ffffff; border-radius:3px'>";
    ret += content;
    ret += "</div></div>";
    return ret;
}

QString MainWindow::styledList(QVector<QString> list)
{
    QString ret = "<div style='margin-left: 20'><div style='border: 1px solid #a1a1a1; border-radius:3px'>";
    for(int i=0; i<list.size(); i++) {
        ret += "<div>" + list[i] + "</div>";
    }
    ret += "</div></div>";
    return ret;
}


void MainWindow::showMoreInfoDialog()
{
    PkgDetailDialog *Dialog = new PkgDetailDialog(this);
    Dialog->initData(rpmArray[0]);
    Dialog->initUI();
    Dialog->setModal(true);
    Dialog->show();
}

void MainWindow::dropFileHandler(QString filename)
{
    this->setArgPath(filename.toStdString());
    loadRpmInfo();      // 开始加载信息，显示加载中界面
}

void MainWindow::exitOnFinished()
{
    QApplication::quit();
}
