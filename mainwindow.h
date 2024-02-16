#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPixmap>
#include <QScreen>
#include <QWidget>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QThread>
#include <QTimer>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlTableModel>
#include <QSqlRecord>
#include <QString>
#include <QByteArray>
#include <QBuffer>
//#include <QDebug>
#include <QCryptographicHash>

#define DATABASE_NAME   "scrDB"
#define TABLE_NAME      "Screenshots"
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class Screenshoter : public QObject
{
    Q_OBJECT
public:
    explicit Screenshoter(QObject *parent = nullptr){}
private:
    QPixmap *main_shot = new QPixmap();
    QPixmap *prev_shot = new QPixmap();
    QPixmap *diff_shot = new QPixmap();
    QPixmap *imgDB = new QPixmap();
    QByteArray *imageByteArray = new QByteArray();
    QByteArray *hash;
    double averageRGBError;
    double relativeError;
public:
    void setMainShot(QPixmap *map);
    void setPrevShot(QPixmap *map = nullptr);
    double getAverageError();
    double getRelativeError();
    QPixmap *getMainShot();
    QPixmap *getPrevShot();
    QPixmap *getDiffShot();
    QPixmap *getImgFromDB(){return imgDB;}
    QByteArray *getImageByteArray();
    void computeHash();
    QByteArray *getHash();
    void compareScreenshots();
    bool convertToByteArray();
signals:
    void resultsReady();
public slots:
    void runComputation();
};


class DataBase : public QObject
{
public:
    explicit DataBase(){}
    bool connectDB();
    bool openDB();
    void closeDB();
    bool createDBDir();
    bool createTable();
    bool insertIntoTable(Screenshoter *scr);
    bool selectFromTable(Screenshoter *scr);   
    bool selectAll();
    void selectImage(Screenshoter *scr);
    QSqlTableModel *getModel();
    void connectTableView();
    void setTableIndex(uint index){tableIndex = index;}
    uint getTableIndex(){return tableIndex;}
private:
    QString homePath;
    QString dbPath;
    QSqlDatabase dBase;
    QSqlQuery * query;
    QSqlTableModel * model;
    uint tableIndex;
};


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public:
    enum Side { LEFT, RIGHT };
    Q_ENUM(Side)
    void showScreenshot(const QPixmap *pix, Side s = LEFT);
    void showScreenshot(const QImage *img, Side s = LEFT);
    void showImgFromDB(const QPixmap *img);
    void showStatusBarMsg(const QString msg);
    bool isMainShotExist();
    bool isPreviousShotExist();
    void setItemsSize();
private slots:
    void on_StartStopButton_2_clicked();

    void on_ClearScreen_2_clicked();

    void on_SaveCurrentScreenshot_2_clicked();

    void on_ShowOlderImg_2_clicked();

    void on_ShowNewerImg_2_clicked();

    void on_CompareImages_2_clicked();

    void on_Difference_2_clicked();

    void on_Update_clicked();

    void on_DBTable_clicked(const QModelIndex &index);

signals:
    void startComputaion();
public slots:
    void handleResults();
    void onTimeOut();
private:
    Ui::MainWindow *ui;
    Screenshoter *screenshot;
    QThread *thread;
    DataBase *db;
    QTimer *timer;
};
#endif // MAINWINDOW_H
