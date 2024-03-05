#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QProcess>
#include <QMessageBox>
#include <QFileInfo>
#include <ctime>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QDir>
#include <QMenu>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle("Enhancing Tool");
    ui->pushButton_Start->setDisabled(1);
    ui->pushButton_Stop->setDisabled(1);
    ui->lineEdit_TileSize->setText("256");
    ui->lineEdit_TileSize->setReadOnly(1);
    ui->progressBar_Progress->setTextVisible(0);
    ui->groupBox_FrameInterpolation->setVisible(0);
    ui->comboBox_modal->setCurrentIndex(9);
    ui->listWidget_File->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->listWidget_File, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(rightClickMenu_File(const QPoint &)));
    connect(ui->pushButton_Add, SIGNAL(clicked()), this, SLOT(OpenFile()));
    ui->spinBox_DenoiseLevel->setRange(-1, 3);
    ui->spinBox_DenoiseLevel->setValue(3);
    ui->label_DenoiseLevel->setVisible(0);
    ui->spinBox_DenoiseLevel->setVisible(0);
    ui->label_SyncGapMode->setVisible(0);
    ui->comboBox_SyncGapMode->setVisible(0);
    ui->label_TimeTaken->setVisible(1);
    ui->label_Remaining->setVisible(0);
    ui->label_TimeTaken->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->label_TimeTaken, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(rightClickMenu_TimeTaken(const QPoint &)));
    ui->label_Remaining->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->label_Remaining, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(rightClickMenu_Remaining(const QPoint &)));
}

MainWindow::~MainWindow()
{
    delete ui;
}

QString readStdOutput(QProcess *process)
{
    QString StdError = process->readAllStandardError().simplified().toLower(),
            StdOutput = process->readAllStandardOutput().simplified().toLower();
    if(StdError != "" || StdOutput != "")
        return StdError + StdOutput;
    else return "";
}

bool ConfirmToStop()
{
    QMessageBox msg(QMessageBox::Warning, "Confirmation", "Do you want to stop current process?", QMessageBox::Yes | QMessageBox::No);
    msg.setStyleSheet("QMessageBox{background-color: rgb(46, 47, 48)}"
                      "QPushButton{background-color: rgb(81, 82, 83); color: rgb(255, 255, 255); font: 10.5pt \"Arial\";"
                      "border-radius: 8px; width: 71px; height: 29px}"
                      "QPushButton:hover{background-color: rgb(77, 78, 79)}"
                      "QPushButton:pressed{background-color: rgb(65, 66, 67)}"
                      "QLabel{color: rgb(255, 255, 255); font: 10.5pt \"Arial\"}");
    if(msg.exec() == QMessageBox::Yes)
        return true;
    else return false;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QMessageBox msg(QMessageBox::Question, "Confirmation", "Do you want to close?", QMessageBox::Yes | QMessageBox::No);
        msg.setStyleSheet("QMessageBox{background-color: rgb(46, 47, 48)}"
                          "QPushButton{background-color: rgb(81, 82, 83); color: rgb(255, 255, 255); font: 10.5pt \"Arial\";"
                          "border-radius: 8px; width: 71px; height: 29px}"
                          "QPushButton:hover{background-color: rgb(77, 78, 79)}"
                          "QPushButton:pressed{background-color: rgb(65, 66, 67)}"
                          "QLabel{color: rgb(255, 255, 255); font: 10.5pt \"Arial\"}");
    if(msg.exec() == QMessageBox::Yes)
    {
        QProcess *close = new QProcess;
        close->start("taskkill /im \"Enhancing Tool.exe\" /f /t");
        event->accept();
    }
    else event->ignore();
}

int MainWindow::countFrames()
{
    QString cmd = "\"" + Current_Path + "/ffprobe.exe\" -v error -select_streams v:0 -count_frames -show_entries "
                  "stream=nb_read_frames -print_format default=nokey=1:noprint_wrappers=1 \"" + file_path + "\"";
    QProcess *CountFrames = new QProcess;
    CountFrames->start(cmd);
    while(CountFrames->state() != 0)
    {
        if(ui->pushButton_Stop->isDown() && ConfirmToStop())
        {
            CountFrames->kill();
            notify = "Stopped";
        }
        QCoreApplication::processEvents();      //to avoid GUI freezing
        setTime(startTime, 0);
    }

    QByteArray Frames = CountFrames->readAllStandardOutput();
    bool ok;
    int frames = Frames.simplified().toInt(&ok, 10);

    return frames;
}

float MainWindow::getFramerate()
{
    QString cmd = "\"" + Current_Path + "/ffprobe.exe\" -v error -select_streams v "
                  "-of default=noprint_wrappers=1:nokey=1 -show_entries stream=r_frame_rate \"" + file_path + "\"";
    QProcess *GetFramerate = new QProcess;
    GetFramerate->start(cmd);
    while(GetFramerate->state() != 0)
    {
        QCoreApplication::processEvents();      //to avoid GUI freezing
        setTime(startTime, 0);
    }

    QByteArray FPS = GetFramerate->readAllStandardOutput();
    std::string str = FPS.simplified().toStdString();
    std::string fu = str.substr(0, str.find('/')),
                fd = str.substr(str.find('/') + 1);
    float fps = float(std::stoi(fu)) / std::stoi(fd);;

    return fps;
}

void MainWindow::setTime(int start, int start1)
{
    int current = time(0),
        elapsed = current - start,
        h = elapsed / 3600,
        m = (elapsed % 3600) / 60,
        s = (elapsed % 3600) % 60;
    QString hh = QString::number(h, 10),
            mm = QString::number(m, 10),
            ss = QString::number(s, 10);
    if(h < 10) hh = "0" + hh;
    if(m < 10) mm = "0" + mm;
    if(s < 10) ss = "0" + ss;
    ui->label_TimeTaken->setText("Time taken: " + hh + ":" + mm + ":" + ss);

    if(ui->progressBar_Progress->value() != 0)
    {
        elapsed = current - start1;
        int remain = float(elapsed) / ui->progressBar_Progress->value() * 100 - elapsed,
            h = remain / 3600,
            m = (remain % 3600) / 60,
            s = (remain % 3600) % 60;
        hh = QString::number(h, 10); if(h < 10) hh = "0" + hh;
        mm = QString::number(m, 10); if(m < 10) mm = "0" + mm;
        ss = QString::number(s, 10); if(s < 10) ss = "0" + ss;
        ui->label_Remaining->setText("Remaining: " + hh + ":" + mm + ":" + ss);
    }
    else ui->label_Remaining->setText("Remaining: unknown");
}

void MainWindow::setFileNameExtension()
{
    file_name = file_path;
    file_extension = "";

    int i = file_name.size() - 1;
    while(i >=0 && file_name.at(i) != '.')
    {
        file_extension = file_name.at(i) + file_extension;
        file_name.remove(i ,1);
        i--;
    }
    file_name.remove(file_name.size() - 1, 1);
    ui->lineEdit_OutFormat->setText(file_extension);
}

void MainWindow::setTotalFile()
{
    total = ui->listWidget_File->count();
    ui->label_Total->setText("Total: " + QString::number(total, 10));
}

void MainWindow::setProgressbarValue(int current)
{
    int previous = ui->progressBar_Progress->value();
    while(previous < current)
    {
        previous++;
        ui->progressBar_Progress->setValue(previous);
        QCoreApplication::processEvents();
    }
}

void MainWindow::rightClickMenu_File(const QPoint &pos)
{
    QMenu Menu(this);
    QAction AddFile("Add file", this);
    QAction AddFolder("Add folder", this);
    connect(&AddFile, SIGNAL(triggered()), this, SLOT(OpenFile()));
    connect(&AddFolder, SIGNAL(triggered()), this, SLOT(OpenFolder()));
    Menu.addAction(&AddFile);
    Menu.addAction(&AddFolder);
    Menu.setStyleSheet("QMenu{color: rgb(255, 255, 255); background-color: rgb(65, 66, 67); border: 1px solid white}"
                       "QMenu::item:selected{background-color: rgb(81, 82, 83);}");
    Menu.exec(ui->listWidget_File->mapToGlobal(pos));
}

void MainWindow::rightClickMenu_TimeTaken(const QPoint &pos)
{
    QMenu Menu(this);
    QAction showRemaining("Show \"Remaining\"", this);
    connect(&showRemaining, SIGNAL(triggered()), this, SLOT(Remaining()));
    Menu.addAction(&showRemaining);
    Menu.setStyleSheet("QMenu{color: rgb(255, 255, 255); background-color: rgb(65, 66, 67); border: 1px solid white; font-size: 10pt}"
                       "QMenu::item:selected{background-color: rgb(81, 82, 83);}");
    Menu.exec(ui->label_TimeTaken->mapToGlobal(pos));
}

void MainWindow::rightClickMenu_Remaining(const QPoint &pos)
{
    QMenu Menu(this);
    QAction showTimeTaken("Show \"Time taken\"", this);
    connect(&showTimeTaken, SIGNAL(triggered()), this, SLOT(TimeTaken()));
    Menu.addAction(&showTimeTaken);
    Menu.setStyleSheet("QMenu{color: rgb(255, 255, 255); background-color: rgb(65, 66, 67); border: 1px solid white; font-size: 10pt}"
                       "QMenu::item:selected{background-color: rgb(81, 82, 83);}");
    Menu.exec(ui->label_Remaining->mapToGlobal(pos));
}

void MainWindow::OpenFile()
{
    files_path = QFileDialog::getOpenFileNames(this, "Open File", QString(),
                    "All Files (*.*) ;; Image Files (*.png *.jpg *.jpeg) ;; Video Files (*.mp4 *.mkv)");
    if(files_path.length() > 0)
    {
        ui->listWidget_File->clear();
        int i = 0;
        file_path = files_path.at(i);
        while(i < files_path.length())
        {
            file_path = files_path.at(i);
            ui->listWidget_File->addItem(file_path);
            i++;
        }
        setFileNameExtension();
        configureAfterOpenning();
    }
}

void MainWindow::OpenFolder()
{    
    dir_path = QFileDialog::getExistingDirectory(this, "Open Folder", QString(), QFileDialog::ShowDirsOnly);

    if(dir_path != "")
    {
        ui->listWidget_File->clear();
        files_path.erase(files_path.begin(), files_path.end());
        QDir dir(dir_path);
        foreach(QFileInfo var, dir.entryInfoList())
            if(var.isFile()) ui->listWidget_File->addItem(var.absoluteFilePath());
        ui->lineEdit_OutFormat->setText("png");
        configureAfterOpenning();
    }
}

void MainWindow::configureAfterOpenning()
{
    setTotalFile();
    ui->pushButton_Start->setEnabled(1);
    ui->progressBar_Progress->setValue(0);
    ui->progressBar_Progress->setTextVisible(0);
    ui->label_TimeTaken->setText("Time taken: 00:00:00");

    if(file_extension != "mp4" && file_extension != "mkv")
    {
        ui->comboBox_ToolsList->setCurrentIndex(0);
        ui->comboBox_ToolsList->setVisible(0);
        ui->label_ToolsList->setVisible(0);
    }

    if(file_extension == "mp4" || file_extension == "mkv")
    {
        QFileInfo fileinfo(file_path);
        mfile_path = fileinfo.absolutePath();
        mfile_name = fileinfo.baseName();
        ui->comboBox_ToolsList->setVisible(1);
        ui->label_ToolsList->setVisible(1);
        ui->comboBox_Modal->setCurrentIndex(2);
    }
}

void MainWindow::on_pushButton_Remove_clicked()
{
    ui->listWidget_File->clear();
    ui->pushButton_Start->setDisabled(1);
    ui->progressBar_Progress->setValue(0);
    ui->progressBar_Progress->setTextVisible(0);
    ui->label_TimeTaken->setText("Time taken: 00:00:00");
    ui->comboBox_ToolsList->setVisible(1);
    ui->label_ToolsList->setVisible(1);
    ui->lineEdit_OutFormat->clear();

    file_path = ""; file_name = ""; file_extension = ""; dir_path = ""; total = 0;
    files_path.erase(files_path.begin(), files_path.end());
    setTotalFile();
}

void MainWindow::on_comboBox_ToolsList_currentIndexChanged()
{
    switch(ui->comboBox_ToolsList->currentIndex())
    {
        case 0:
        {
            ui->groupBox_Upscaler->setVisible(1);
            ui->groupBox_FrameInterpolation->setVisible(0);
            break;
        }
        case 1:
        {
            ui->groupBox_Upscaler->setVisible(0);
            ui->groupBox_FrameInterpolation->setVisible(1);
            break;
        }
    }
}

void MainWindow::on_pushButton_Start_clicked()
{
    startTime = time(0);
    setAcceptDrops(0);
    ui->pushButton_Start->setDisabled(1);
    ui->pushButton_Stop->setEnabled(1);
    ui->pushButton_Add->setDisabled(1);
    ui->pushButton_Remove->setDisabled(1);
    ui->frame->setDisabled(1);
    notify = "Finished";
    disconnect(ui->listWidget_File, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(rightClickMenu_File(const QPoint &)));

    switch(ui->comboBox_ToolsList->currentIndex())
    {
        case 0:
        {
            bool ok;
            TileSize = ui->lineEdit_TileSize->text().QString::toInt(&ok, 10);
            file_extension = '.' + ui->lineEdit_OutFormat->text();
            if(file_extension == ".mp4" || file_extension == ".mkv")
                ui->progressBar_Progress->setFormat("Preparing... (%p%)");
            else ui->progressBar_Progress->setFormat("Processing... (%p%)");
            ui->progressBar_Progress->setValue(0);
            ui->progressBar_Progress->setTextVisible(1);
            ui->groupBox_Upscaler->setEnabled(0);

            QString thread = "", engine = "", m = "", c = "",
                    g = "", x = "_x", s = " -s ", n = " -n ",
                    t = " -t " + ui->lineEdit_TileSize->text();
            if(ui->lineEdit_Thread->text() != "")
            {
                thread = ui->lineEdit_Thread->text().at(0);
                Thread = thread.toInt(&ok, 10);
            }
            else Thread = 2;
            if(ui->lineEdit_GpuID->text() != "") g = " -g " + ui->lineEdit_GpuID->text();

            switch(ui->comboBox_Modal->currentIndex())
            {
                case 0: case 1: case 4: case 6: case 7: case 8: case 11: case 14:
                {
                    x += "4"; s += "4"; break;
                }
                case 3: case 10: case 13:
                {
                    x += "3"; s += "3"; break;
                }
                case 2: case 5: case 9: case 12:
                {
                    x += "2"; s += "2"; break;
                }
            }

            if(ui->comboBox_Modal->currentIndex() < 9)
            {
                engine = "/realesrgan-ncnn-vulkan/realesrgan-ncnn-vulkan.exe";
                n += ui->comboBox_Modal->currentText();
            }
            else
            {
                engine = "/realcugan-ncnn-vulkan/realcugan-ncnn-vulkan.exe";
                n += QString::number(ui->spinBox_DenoiseLevel->value(), 10);
                c = " -c " + QString::number(ui->comboBox_SyncGapMode->currentIndex(), 10);

                if(ui->comboBox_Modal->currentIndex() < 12)
                    m = " -m models-se";
                else m = " -m models-pro";
            }

            if(file_extension == ".mp4" || file_extension == ".mkv")
            {
                int frames = countFrames();
                if(notify == "Stopped") goto end;
                int startTime1 = time(0);
                QString folder_path = mfile_path + "/" + mfile_name + "_frames",
                        format = "png",
                        cmd = "\"" + Current_Path + "/ffmpeg.exe\" -i \"" + file_path + "\" "
                            + "\"" + folder_path + "\"/%08d." + format;

                QDir dir(mfile_path);
                QDir dir1(folder_path);
                if(dir1.exists()) dir1.removeRecursively();
                dir.mkdir(mfile_name + "_frames");

                ui->progressBar_Progress->setFormat("1 of 3: Extracting frames... (%p%)");

                QProcess *ExtractFrames = new QProcess;
                ExtractFrames->start(cmd);
                while(ExtractFrames->state() != 0)
                {
                    QDir dir1(folder_path);
                    int current = (float(dir1.count() - 2) / frames) * 100;
                    setProgressbarValue(current);

                    if(ui->pushButton_Stop->isDown() && ConfirmToStop())
                    {
                        notify = "Stopped";
                        ExtractFrames->kill();
                        goto end;
                    }
                    QCoreApplication::processEvents();      //to avoid GUI freezing
                    setTime(startTime, startTime1);
                }

                ui->progressBar_Progress->setValue(0);
                ui->progressBar_Progress->setFormat("2 of 3: Processing frames... (%p%)");

                QDir dir2(folder_path + x);
                if(dir2.exists()) dir2.removeRecursively();
                dir.mkdir(mfile_name + "_frames" + x);

                QString j = "";
                if(ui->lineEdit_Thread->text() != "") j = " -j " + ui->lineEdit_Thread->text();

                retry_1:
                startTime1 = time(0);
                cmd = "\"" + Current_Path + engine + "\" -i \"" + folder_path + "\" -o \""
                            + folder_path + x + "\"" + m + s + n + c + t + g + j + " -f png";
                QProcess *Upscaler = new QProcess;
                Upscaler->start(cmd);
                while(Upscaler->state() != 0)
                {
                    QString output = readStdOutput(Upscaler);
                    if(output.contains("failed"))
                    {
                        notify = "Failed";
                        Upscaler->kill();
                        break;
                    }
                    else
                    {
                        notify = "Finished";
                        QDir dir2(folder_path + x);
                        int current = (float(dir2.count() - 2) / frames) * 100;
                        setProgressbarValue(current);
                    }

                    if(ui->pushButton_Stop->isDown() && ConfirmToStop())
                    {
                        notify = "Stopped";
                        Upscaler->kill();
                        goto end;
                    }
                    QCoreApplication::processEvents();      //to avoid GUI freezing
                    setTime(startTime, startTime1);
                }

                if(notify == "Failed")
                {
                    if(TileSize > 256)
                    {
                        TileSize /= 2;
                        t = " -t " + QString::number(TileSize, 10);
                        ui->lineEdit_TileSize->setText(QString::number(TileSize, 10));
                    }
                    else if(Thread > 1)
                    {
                        Thread--;
                        thread = QString::number(Thread, 10);
                        ui->lineEdit_Thread->setText(thread + ":" + thread + ":" + thread);
                        j = " -j " + ui->lineEdit_Thread->text();
                    }
                    else if(TileSize > 128)
                    {
                        TileSize /= 2;
                        t = " -t " + QString::number(TileSize, 10);
                        ui->lineEdit_TileSize->setText(QString::number(TileSize, 10));
                    }
                    else if(TileSize > 0)
                    {
                        TileSize = 0;
                        t = " -t " + QString::number(TileSize, 10);
                        ui->lineEdit_TileSize->setText(QString::number(TileSize, 10));
                    }
                    else goto end;

                    goto retry_1;
                }

                ui->progressBar_Progress->setValue(0);
                ui->progressBar_Progress->setFormat("3 of 3: Encoding frames... (%p%)");

                dir1.removeRecursively();
                QFile file(file_name + x + file_extension);
                if(file.exists()) file.remove();

                startTime1 = time(0);
                cmd = "\"" + Current_Path + "/ffmpeg.exe\" -framerate " + QString::number(getFramerate(), 'g', 4) + " -i \""
                    + folder_path + x + "\"/%08d." + format + " -i \"" + file_path
                    + "\" -c:a copy -crf 20 -c:v libx264 -pix_fmt yuv420p \"" + file_name + x + file_extension + "\"";
                QProcess *EncodeFrames = new QProcess;
                EncodeFrames->start(cmd);
                while(EncodeFrames->state() != 0)
                {
                    QString output = readStdOutput(EncodeFrames);
                    if(output.contains("frame= "))
                    {
                        output.remove(output.indexOf("frame= "), 7);
                        output.remove(output.indexOf(" fps="), output.length() - 1);
                        bool ok;
                        int current = output.QString::toFloat(&ok) / frames * 100;
                        setProgressbarValue(current);
                    }

                    if(ui->pushButton_Stop->isDown() && ConfirmToStop())
                    {
                        notify = "Stopped";
                        EncodeFrames->kill();
                        goto end;
                    }
                    QCoreApplication::processEvents();      //to avoid GUI freezing
                    setTime(startTime, startTime1);
                }
                dir2.removeRecursively();
            }
            else if(files_path.length() > 0)
            {
                int i = 0;
                while(i < files_path.length())
                {
                    file_path = files_path.at(i);
                    QFileInfo fileinfo(file_path);
                    file_name = fileinfo.absolutePath()+ "/" + fileinfo.baseName();

                    retry_2:
                    int startTime1 = time(0);
                    QString cmd = "\"" + Current_Path + engine + "\" -i \"" + file_path + "\" -o \""
                                + file_name + x + file_extension + "\"" + m + s + n + c + t + g;
                    QProcess *Upscaler = new QProcess;
                    Upscaler->start(cmd);
                    while(Upscaler->state() != 0)
                    {
                        QString output = readStdOutput(Upscaler);
                        if(output.contains("failed"))
                        {
                            notify = "Failed";
                            Upscaler->kill();
                            break;
                        }
                        else
                        {
                            notify = "Finished";
                            if(output.contains("%"))
                            {
                                bool ok;
                                int current = (output.remove(output.length() - 1, 1).toFloat(&ok) + 100 * i) / files_path.length();
                                setProgressbarValue(current);
                            }
                        }

                        if(ui->pushButton_Stop->isDown() && ConfirmToStop())
                        {
                            notify = "Stopped";
                            Upscaler->kill();
                            goto end;
                        }
                        QCoreApplication::processEvents();      //to avoid GUI freezing
                        setTime(startTime, startTime1);
                    }

                    if(notify == "Failed")
                    {
                        if(TileSize > 32)
                        {
                            TileSize /= 2;
                            t = " -t " + QString::number(TileSize, 10);
                            ui->lineEdit_TileSize->setText(QString::number(TileSize, 10));
                        }
                        else if(TileSize > 0)
                        {
                            TileSize = 0;
                            t = " -t " + QString::number(TileSize, 10);
                            ui->lineEdit_TileSize->setText(QString::number(TileSize, 10));
                        }
                        else goto end;

                        goto retry_2;
                    }
                    i++;
                }
            }
            else
            {
                QDir dir(dir_path);
                QDir dir1(dir_path + "/" + x);
                if(dir1.exists()) dir1.removeRecursively();
                dir.mkdir(x);

                QString j = "", f = "";
                if(ui->lineEdit_Thread->text() != "") j = " -j " + ui->lineEdit_Thread->text();
                if(ui->lineEdit_OutFormat->text() != "") f = " -f " + ui->lineEdit_OutFormat->text();

                retry_3:
                int startTime1 = time(0);
                QString cmd = "\"" + Current_Path + engine + "\" -i \"" + dir_path + "\" -o \""
                            + dir_path + "/" + x + "\"" + m + s + n + c + t + g + j + f;
                QProcess *Upscaler = new QProcess;
                Upscaler->start(cmd);
                while(Upscaler->state() != 0)
                {
                    QString output = readStdOutput(Upscaler);
                    if(output.contains("failed"))
                    {
                        notify = "Failed";
                        Upscaler->kill();
                        break;
                    }
                    else
                    {
                        notify = "Finished";
                        QDir dir1(dir_path + "/" + x);
                        int current = (float(dir1.count() - 2) / total) * 100;
                        setProgressbarValue(current);
                    }

                    if(ui->pushButton_Stop->isDown() && ConfirmToStop())
                    {
                        notify = "Stopped";
                        Upscaler->kill();
                        goto end;
                    }
                    QCoreApplication::processEvents();      //to avoid GUI freezing
                    setTime(startTime, startTime1);
                }                

                if(notify == "Failed")
                {
                    if(TileSize > 256)
                    {
                        TileSize /= 2;
                        t = " -t " + QString::number(TileSize, 10);
                        ui->lineEdit_TileSize->setText(QString::number(TileSize, 10));
                    }
                    else if(Thread > 1)
                    {
                        Thread--;
                        thread = QString::number(Thread, 10);
                        ui->lineEdit_Thread->setText(thread + ":" + thread + ":" + thread);
                        j = " -j " + ui->lineEdit_Thread->text();
                    }
                    else if(TileSize > 128)
                    {
                        TileSize /= 2;
                        t = " -t " + QString::number(TileSize, 10);
                        ui->lineEdit_TileSize->setText(QString::number(TileSize, 10));
                    }
                    else if(TileSize > 0)
                    {
                        TileSize = 0;
                        t = " -t " + QString::number(TileSize, 10);
                        ui->lineEdit_TileSize->setText(QString::number(TileSize, 10));
                    }
                    else goto end;

                    goto retry_3;
                }
            }
            break;
        }
        case 1:
        {
            ui->progressBar_Progress->setFormat("Preparing... (%p%)");
            ui->progressBar_Progress->setValue(0);
            ui->progressBar_Progress->setTextVisible(1);
            ui->groupBox_FrameInterpolation->setEnabled(0);

            QString thread = "", u = "", g = "", j = "", engine = "",
                    x = "_" + QString::number(getFramerate()*2, 'g', 4) + "FPS",
                    m = " -m " + ui->comboBox_modal->currentText();
            if(ui->lineEdit_thread->text() != "")
            {
                thread = ui->lineEdit_thread->text().at(0);
                bool ok;
                Thread = thread.toInt(&ok, 10);
            }
            else Thread = 2;
            file_extension = '.' + ui->lineEdit_OutFormat->text();
            if(ui->lineEdit_GpuID->text() != "") g = " -g " + ui->lineEdit_GpuID->text();
            if(ui->lineEdit_thread->text() != "") j = " -j " + ui->lineEdit_thread->text();
            if(ui->checkBox_UHD->isChecked()) u = " -u";
            if(ui->comboBox_modal->currentIndex() < 11)
                engine = "/rife-ncnn-vulkan/rife-ncnn-vulkan.exe";
            else engine = "/ifrnet-ncnn-vulkan/ifrnet-ncnn-vulkan.exe";

            int frames = countFrames();
            if(notify == "Stopped") goto end;
            QString folder_path = mfile_path + "/" + mfile_name + "_frames",
                    format = "png",
                    cmd = "\"" + Current_Path + "/ffmpeg.exe\" -i \"" + file_path + "\" "
                        + "\"" + folder_path + "\"/%08d." + format;

            QDir dir(mfile_path);
            QDir dir1(folder_path);
            if(dir1.exists()) dir1.removeRecursively();
            dir.mkdir(mfile_name + "_frames");

            ui->progressBar_Progress->setFormat("1 of 3: Extracting frames... (%p%)");

            int startTime1 = time(0);
            QProcess *ExtractFrames = new QProcess;
            ExtractFrames->start(cmd);
            while(ExtractFrames->state() != 0)
            {
                QDir dir1(folder_path);
                int current = (float(dir1.count() - 2) / frames) * 100;
                setProgressbarValue(current);

                if(ui->pushButton_Stop->isDown() && ConfirmToStop())
                {
                    notify = "Stopped";
                    ExtractFrames->kill();
                    goto end;
                }
                QCoreApplication::processEvents();      //to avoid GUI freezing
                setTime(startTime, startTime1);
            }

            ui->progressBar_Progress->setValue(0);
            ui->progressBar_Progress->setFormat("2 of 3: Interpolating frames... (%p%)");

            QDir dir2(folder_path + x);
            if(dir2.exists()) dir2.removeRecursively();
            dir.mkdir(mfile_name + "_frames" + x);

            retry_4:
            startTime1 = time(0);
            cmd = "\"" + Current_Path + engine + "\" -i \"" + folder_path + "\" -o \"" + folder_path + x + "\"" + m + g + j + u;
            QProcess *Interpolate = new QProcess;
            Interpolate->start(cmd);
            while(Interpolate->state() != 0)
            {
                QString output = readStdOutput(Interpolate);
                if(output.contains("failed"))
                {
                    notify = "Failed";
                    Interpolate->kill();
                    break;
                }
                else
                {
                    notify = "Finished";
                    QDir dir2(folder_path + x);
                    int current = (float(dir2.count() - 2) / (frames * 2)) * 100;
                    setProgressbarValue(current);
                }

                if(ui->pushButton_Stop->isDown() && ConfirmToStop())
                {
                    notify = "Stopped";
                    Interpolate->kill();
                    goto end;
                }
                QCoreApplication::processEvents();      //to avoid GUI freezing
                setTime(startTime, startTime1);
            }

            if(notify == "Failed")
            {
                if(Thread > 1)
                {
                    Thread--;
                    thread = QString::number(Thread, 10);
                    ui->lineEdit_thread->setText(thread + ":" + thread + ":" + thread);
                    j = " -j " + ui->lineEdit_thread->text();
                    goto retry_4;
                }
                else goto end;
            }

            ui->progressBar_Progress->setValue(0);
            ui->progressBar_Progress->setFormat("3 of 3: Encoding frames... (%p%)");

            dir1.removeRecursively();
            QFile file(file_name + x + file_extension);
            if(file.exists()) file.remove();

            startTime1 = time(0);
            cmd = "\"" + Current_Path + "/ffmpeg.exe\" -framerate " + QString::number(getFramerate()*2, 'g', 4) + " -i \""
                + folder_path + x + "\"/%08d." + format + " -i \"" + file_path
                + "\" -c:a copy -crf 20 -c:v libx264 -pix_fmt yuv420p \"" + file_name + x + file_extension + "\"";
            QProcess *EncodeFrames = new QProcess;
            EncodeFrames->start(cmd);
            while(EncodeFrames->state() != 0)
            {
                QString output = readStdOutput(EncodeFrames);
                if(output.contains("frame= "))
                {
                    output.remove(output.indexOf("frame= "), 7);
                    output.remove(output.indexOf(" fps="), output.length() - 1);
                    bool ok;
                    int current = output.QString::toFloat(&ok) / (frames * 2) * 100;
                    setProgressbarValue(current);
                }

                if(ui->pushButton_Stop->isDown() && ConfirmToStop())
                {
                    notify = "Stopped";
                    EncodeFrames->kill();
                    goto end;
                }
                QCoreApplication::processEvents();      //to avoid GUI freezing
                setTime(startTime, startTime1);
            }
            dir2.removeRecursively();
            break;
        }
    }
    setProgressbarValue(100);
    end: ui->progressBar_Progress->setFormat(notify + " (%p%)");
    ui->groupBox_Upscaler->setEnabled(1);
    ui->groupBox_FrameInterpolation->setEnabled(1);
    ui->frame->setEnabled(1);
    ui->pushButton_Stop->setDisabled(1);
    ui->pushButton_Add->setEnabled(1);
    ui->pushButton_Remove->setEnabled(1);
    ui->label_TimeTaken->setVisible(1);
    ui->label_Remaining->setVisible(0);
    ui->label_Remaining->setText("Remaining: unknown");
    connect(ui->listWidget_File, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(rightClickMenu_File(const QPoint &)));
    if(ui->checkBox_ReProcess->isChecked())
        ui->pushButton_Start->setEnabled(1);
    setAcceptDrops(1);

    QMessageBox msg(QMessageBox::Information, "Notification", notify + "\n", QMessageBox::Ok);
    msg.setStyleSheet("QMessageBox{background-color: rgb(46, 47, 48)}"
                      "QPushButton{background-color: rgb(81, 82, 83); color: rgb(255, 255, 255); font: 10.5pt \"Arial\";"
                      "border-radius: 8px; width: 71px; height: 29px}"
                      "QPushButton:hover{background-color: rgb(77, 78, 79)}"
                      "QPushButton:pressed{background-color: rgb(65, 66, 67)}"
                      "QLabel{color: rgb(255, 255, 255); font: 10.5pt \"Arial\"}");
    msg.exec();
}

void MainWindow::on_pushButton_IncTileSize_clicked()
{
    bool ok;
    TileSize = ui->lineEdit_TileSize->text().QString::toInt(&ok, 10);
    if(TileSize < 1048576)
    {
        if(TileSize == 0) ui->lineEdit_TileSize->setText("32");
        else
        {
            TileSize *= 2;
            ui->lineEdit_TileSize->setText(QString::number(TileSize, 10));
        }
    }
}

void MainWindow::on_pushButton_DecTileSize_clicked()
{
    bool ok;
    TileSize = ui->lineEdit_TileSize->text().QString::toInt(&ok, 10);
    if(TileSize > 32)
    {
        TileSize /= 2;
        ui->lineEdit_TileSize->setText(QString::number(TileSize, 10));
    }
    else ui->lineEdit_TileSize->setText("0");
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if(event->mimeData()->hasUrls()) event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    ui->listWidget_File->clear();
    files_path.erase(files_path.begin(), files_path.end());
    foreach(const QUrl &url, event->mimeData()->urls())
    {
        QString path = url.toLocalFile();
        QFileInfo fileinfo(path);
        if(fileinfo.isFile())
        {
            files_path << path;
            file_path = path;
            setFileNameExtension();
            ui->listWidget_File->addItem(path);
        }
        else
        {
            dir_path = path;
            QDir dir(dir_path);
            foreach(QFileInfo var, dir.entryInfoList())
                if(var.isFile()) ui->listWidget_File->addItem(var.absoluteFilePath());
            ui->lineEdit_OutFormat->setText("png");
        }
    }
    configureAfterOpenning();
}

void MainWindow::on_listWidget_File_itemDoubleClicked()
{
    ui->listWidget_File->clearSelection();
}

void MainWindow::on_comboBox_Modal_currentIndexChanged()
{
    if(ui->comboBox_Modal->currentIndex() < 9)
    {
        ui->label_DenoiseLevel->setVisible(0);
        ui->spinBox_DenoiseLevel->setVisible(0);
        ui->label_SyncGapMode->setVisible(0);
        ui->comboBox_SyncGapMode->setVisible(0);
    }
    else
    {
        ui->label_DenoiseLevel->setVisible(1);
        ui->spinBox_DenoiseLevel->setVisible(1);
        ui->label_SyncGapMode->setVisible(1);
        ui->comboBox_SyncGapMode->setVisible(1);
    }
}
