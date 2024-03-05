#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ui_mainwindow.h"
#include <QMainWindow>
#include <QImageWriter>
#include <QCloseEvent>
#include <QProcess>

QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    QString Current_Path = qApp->applicationDirPath();
    QString file_path, file_name, file_extension, dir_path, mfile_path, mfile_name;
    QStringList files_path;
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
    int TileSize, Thread, total = 0;
    void setTotalFile();
    void setTime(int start, int start1);
    void setFileNameExtension();
    int startTime, countFrames();
    float getFramerate();
    QString notify;
    void configureAfterOpenning();
    void closeEvent (QCloseEvent *event);
    void setProgressbarValue(int current);
    ~MainWindow();

private slots:
    void on_pushButton_Remove_clicked();
    void on_comboBox_ToolsList_currentIndexChanged();
    void on_pushButton_Start_clicked();
    void on_pushButton_IncTileSize_clicked();
    void on_pushButton_DecTileSize_clicked();
    void on_listWidget_File_itemDoubleClicked();
    void OpenFile();
    void OpenFolder();
    void rightClickMenu_File(const QPoint &pos);
    void rightClickMenu_TimeTaken(const QPoint &pos);
    inline void Remaining() {ui->label_TimeTaken->setVisible(0); ui->label_Remaining->setVisible(1);}
    inline void TimeTaken() {ui->label_TimeTaken->setVisible(1); ui->label_Remaining->setVisible(0);}
    void rightClickMenu_Remaining(const QPoint &pos);
    void on_comboBox_Modal_currentIndexChanged();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
