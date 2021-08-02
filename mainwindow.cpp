#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "dragdropfilearea.h"

#include <QDesktopWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <string>
#include <QProcess>
#include <QStringList>

MainWindow::MainWindow(int argc, char **argv, QWidget *parent) :   // 直接通过构造器来搞
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("RPM 安装器");

    // 下面两行让窗口创建在屏幕正中间
    QDesktopWidget *desktop = QApplication::desktop();
    move((desktop->width()-this->width())/2,(desktop->height()-this->height())/2);

    setFixedSize(600,300);

    delete ui->mainToolBar;         // 不需要 toolbar ，删掉

    this->setArgc(argc);
    this->getArgc();
    if(argc==1) {
        // 没有启动参数输入，应该是啥也不干. 选择文件安装的界面应该在这里面绘制
        // TODO: 把这玩意抽出去，单独封装一下
        QWidget *mainContentWid = new QWidget;          // MainWindow 中的 Content，其实是个装 QVBoxLayout 的容器
                                                        // 这玩意的正确用法，或许应该是单拿出去做个类，和 main 拆开

        QVBoxLayout *mainLayout = new QVBoxLayout;      // 定义 QVBoxLayout
        mainContentWid->setLayout(mainLayout);          // 设置布局

        QPixmap pixmap;                                 // 多种格式的图片都可以存为 pixmap
        pixmap.load(":/icon_install_light.svg");        // 对 resources 中的引用【:/prefix/filename】
        DragDropFileArea *installIconLabel = new DragDropFileArea();
        installIconLabel->setPixmap(pixmap);
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
        buttonSelectRPM->setMinimumWidth(200);
        buttonSelectRPM->setMaximumWidth(200);

        // 通过这种方式，把按钮的事件（clicked）和处理函数“连接”起来
        connect(buttonSelectRPM, &QPushButton::clicked, this, &MainWindow::selectRpmBtnHandler);

        mainLayout->addWidget(installIconLabel, 0, Qt::AlignCenter);            // 嗯，原来如此
        mainLayout->addWidget(dropHereText, 0, Qt::AlignCenter);
        mainLayout->addSpacing(10);
        mainLayout->addWidget(splitLine, 0, Qt::AlignCenter);
        mainLayout->addSpacing(10);
        mainLayout->addWidget(buttonSelectRPM, 0, Qt::AlignCenter);

        this->setCentralWidget(mainContentWid);

    } else {
        // 这里有参数输入了，先不管连续输入多个的情况，只管装第一个
        this->setArgPath(std::string(argv[1]));
        this->getArgPath();
        this->sendNotify();

        QWidget *mainContentWid = new QWidget;          // MainWindow 中的 Content，其实是个装 QVBoxLayout 的容器
        QVBoxLayout *mainLayout = new QVBoxLayout;      // 定义 QVBoxLayout
        mainContentWid->setLayout(mainLayout);          // 设置布局

        QPixmap pixmap;
        pixmap.load(":/icon_install_light.svg");        // 这个应该是可以换成 RPM 内置的图标（如果有）
        QLabel *installIconLabel = new QLabel();
        installIconLabel->setPixmap(pixmap);

        QLabel *rpmInfoText = new QLabel();
        // TODO: 做一个单独的函数来取信息
        rpmInfoText->setText(this->getRPMSummary());
        rpmInfoText->setStyleSheet("QLabel{font-size:12px}");
        rpmInfoText->setMinimumWidth(330);
        rpmInfoText->setMaximumWidth(380);
        rpmInfoText->setWordWrap(true);

        QPushButton *buttonInstallRPM = new QPushButton("确认安装");
        buttonInstallRPM->setStyleSheet("QPushButton{color:#0099FF; background-color:transparent; font-size:12px}");
        buttonInstallRPM->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
        buttonInstallRPM->setMinimumWidth(200);
        buttonInstallRPM->setMaximumWidth(200);

        connect(buttonInstallRPM, &QPushButton::clicked, this, &MainWindow::installRPM);

        mainLayout->addWidget(installIconLabel, 0, Qt::AlignCenter);
        mainLayout->addWidget(rpmInfoText, 0, Qt::AlignCenter);
        mainLayout->addSpacing(10);
        mainLayout->addWidget(buttonInstallRPM, 0, Qt::AlignCenter);

        this->setCentralWidget(mainContentWid);

    }

}

MainWindow::~MainWindow()
{
    delete ui;
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
    qDebug("%s", fileName.toStdString().c_str());
    bool isSelectSuccess = true;
    if(isSelectSuccess) {   // 选择成功的情况
        this->setArgPath(fileName.toStdString());
        this->changeUIAfterRPMSelect();
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

bool MainWindow::installRPM()
{
    bool isSuccess = false;
    // TODO : 把这玩意换成真的安装了
    QProcess process;
    process.start("pkexec", QStringList() << "yum" <<"-y" <<"localinstall"<< QString(this->argPath.c_str()));
    process.waitForFinished();
    std::string res = process.readAllStandardOutput().toStdString();  // 中文安装完成，最后会输出“完毕！”，英文安装完成会输出“Complete!”
    qDebug("%s", qPrintable(QString().fromStdString(res)));
    if(res.find_last_of("Complete!")!=std::string::npos||res.find_last_of("完毕！")!=std::string::npos) {
        isSuccess = true;
        process.setReadChannel(QProcess::StandardOutput);
        process.start("notify-send", QStringList() << "安装完成！");
        process.waitForFinished();
        return isSuccess;
    } else {
        process.start("notify-send", QStringList() << "安装失败！");
        process.waitForFinished();
        return isSuccess;
    }
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

    // 解决可能存在的，Summary 长度超过一行的情况（应该不会出现吧
    for(std::string::iterator iter=rpmInfoStr.begin(); iter!=infoEnd; iter++) {
        if(*iter=='\n') *iter=' ';
    }

    summary = QString::fromStdString(rpmInfoStr);
    qDebug("%d", summary.length());
    qDebug("%s", qPrintable(summary));

    return summary;
}

bool MainWindow::changeUIAfterRPMSelect()
{
    bool status = true;
    QWidget *mainContentWid = new QWidget;          // MainWindow 中的 Content，其实是个装 QVBoxLayout 的容器
    QVBoxLayout *mainLayout = new QVBoxLayout;      // 定义 QVBoxLayout
    mainContentWid->setLayout(mainLayout);          // 设置布局

    QPixmap pixmap;
    pixmap.load(":/icon_install_light.svg");        // 这个应该是可以换成 RPM 内置的图标（如果有）
    QLabel *installIconLabel = new QLabel();
    installIconLabel->setPixmap(pixmap);

    QLabel *rpmInfoText = new QLabel();
    // TODO: 做一个单独的函数来取信息
    rpmInfoText->setText(this->getRPMSummary());
    rpmInfoText->setStyleSheet("QLabel{font-size:12px}");
    rpmInfoText->setMinimumWidth(330);
    rpmInfoText->setMaximumWidth(380);
    rpmInfoText->setWordWrap(true);

    QPushButton *buttonInstallRPM = new QPushButton("确认安装");
    buttonInstallRPM->setStyleSheet("QPushButton{color:#0099FF; background-color:transparent; font-size:12px}");
    buttonInstallRPM->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
    buttonInstallRPM->setMinimumWidth(200);
    buttonInstallRPM->setMaximumWidth(200);

    connect(buttonInstallRPM, &QPushButton::clicked, this, &MainWindow::installRPM);

    mainLayout->addWidget(installIconLabel, 0, Qt::AlignCenter);
    mainLayout->addWidget(rpmInfoText, 0, Qt::AlignCenter);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(buttonInstallRPM, 0, Qt::AlignCenter);

    this->setCentralWidget(mainContentWid);
    return status;
}

void MainWindow::dropFileHandler(QString filename)
{
    qDebug("%s", qPrintable(filename));
    this->setArgPath(filename.toStdString());
    this->changeUIAfterRPMSelect();
}
