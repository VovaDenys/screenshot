#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    screenshot = new Screenshoter();
    thread = new QThread(this);
    db = new DataBase();
    db->connectDB();

    db->selectFromTable(screenshot);
    showScreenshot(screenshot->getMainShot(),LEFT);
    showScreenshot(screenshot->getPrevShot(),RIGHT);

    db->connectTableView();
    ui->DBTable->setModel(db->getModel());
    ui->DBTable->showGrid();
    ui->DBTable->setSortingEnabled(true);

    setItemsSize();

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(onTimeOut()));

    connect(this,SIGNAL(destroyed()),thread,SLOT(quit()));
    //connect(thread, &QThread::finished, screenshot, &QObject::deleteLater);
    connect(this,SIGNAL(startComputaion()),screenshot,SLOT(runComputation()));
    connect(screenshot,SIGNAL(resultsReady()),this,SLOT(handleResults()));

    screenshot->moveToThread(thread);
    thread->start();
}

MainWindow::~MainWindow()
{
    delete ui;
    delete screenshot;
    delete thread;
    db->closeDB();
}

void Screenshoter::setMainShot(QPixmap *map)
{
    main_shot = map;
}

void Screenshoter::setPrevShot(QPixmap *map)
{
    if(prev_shot != nullptr)
        delete prev_shot;
    if(map != nullptr)
    {
        prev_shot = map;
    }
    else
    {
        prev_shot = main_shot;
    }
}

double Screenshoter::getAverageError()
{
    return averageRGBError;
}

double Screenshoter::getRelativeError()
{
    return relativeError;
}

QPixmap *Screenshoter::getMainShot()
{
    return main_shot;
}

QPixmap *Screenshoter::getPrevShot()
{
    return prev_shot;
}

QPixmap *Screenshoter::getDiffShot()
{
    return diff_shot;
}

QByteArray *Screenshoter::getImageByteArray()
{
    return imageByteArray;
}

void Screenshoter::computeHash()
{
    QCryptographicHash crypto(QCryptographicHash::Md5);
    const QByteArray &tmp = *imageByteArray;
    //crypto->addData(tmp);
    //crypto.hash(tmp, QCryptographicHash::Md5);
    QCryptographicHash::hash(tmp, QCryptographicHash::Md5);
    hash = new QByteArray ( crypto.result());
}

QByteArray *Screenshoter::getHash()
{
    return hash;
}

bool MainWindow::isMainShotExist()
{
    if(screenshot->getMainShot()->isNull() || screenshot->getMainShot() == nullptr)
    {
        return false;
    }
    return true;
}

bool MainWindow::isPreviousShotExist()
{
    if(screenshot->getPrevShot()->isNull() || screenshot->getMainShot() == nullptr)
    {
        return false;
    }
    return true;
}

void MainWindow::setItemsSize()
{
    QScreen *screen = QGuiApplication::primaryScreen();
    int screen_width = screen->geometry().width();
    int screen_height = screen->geometry().height();

    ui->centralwidget->resize(screen_width,screen_height);
    ui->centralwidget->setMinimumWidth(screen_width/2);
    ui->centralwidget->setMinimumHeight(screen_height/2);
    ui->tabWidget->resize(screen_width/1.1,screen_height/1.1);
    ui->imgDB->resize(screen_width/2,screen_height/2);
    ui->img_2->resize(screen_width/3,screen_height/3);
    ui->img2_2->resize(screen_width/3,screen_height/3);
    ui->DBTable->resize(450, screen_height/1.1);
}

void Screenshoter::compareScreenshots()
{
    QImage main =  main_shot->toImage();
    QImage previous = prev_shot->toImage();

    double pix_number = (double)main.height() * (double)main.width();
    double TotalErrors = 0.0;
    double AlphaError = 0.0;
    double RedError = 0.0;
    double GreenError = 0.0;
    double BlueError = 0.0;
    averageRGBError = 0.0;
    relativeError = 0.0;

    for (int y = 0; y < main.height(); ++y)
    {
        QRgb *line1 = reinterpret_cast<QRgb*>(main.scanLine(y));
        QRgb *line2 = reinterpret_cast<QRgb*>(previous.scanLine(y));
        for (int x = 0; x < main.width(); ++x)
        {
            bool A = qAlpha(line1[x]) == qAlpha(line2[x]);
            bool R = qRed(line1[x]) == qRed(line2[x]);
            bool G = qGreen(line1[x]) == qGreen(line2[x]);
            bool B = qBlue(line1[x]) == qBlue(line2[x]);

            if(!A || !R || !G || !B)
            {
                line2[x] = qRgba(qRed(line2[x]), qGreen(0), qBlue(line2[x]), qAlpha(line2[x]));
                TotalErrors++;

                if(!A)
                    AlphaError++;
                if(!R)
                    RedError++;
                if(!G)
                    GreenError++;
                if(!B)
                    BlueError++;
            }
        }
    }

    //AlphaError = AlphaError / pix_number * 100.0;
    RedError = RedError / pix_number * 100.0;
    GreenError = GreenError / pix_number * 100.0;
    BlueError = BlueError / pix_number * 100.0;
    averageRGBError = (RedError + GreenError + BlueError) / 3.0;
    relativeError = (TotalErrors * 100) / pix_number;

    diff_shot = new QPixmap (QPixmap::fromImage(previous,Qt::AutoColor));
    //QThread::sleep(3);
}

bool Screenshoter::convertToByteArray()
{
   if( imageByteArray != nullptr )
   {
       imageByteArray->clear();
   }

   QBuffer imageBuffer(imageByteArray);
   imageBuffer.open( QIODevice::ReadWrite );
   return main_shot->save( &imageBuffer, "PNG",100);
}

void Screenshoter::runComputation()
{
    compareScreenshots();
    emit resultsReady();
}

void MainWindow::showScreenshot(const QPixmap *pix, Side s)
{
    if(pix == nullptr || pix->isNull())
    {
       showStatusBarMsg("there no screenshot to show");
    }
    else
    {
        int w = pix->width()/3;
        int h = pix->height()/3;
        if(s == MainWindow::LEFT)
        {
            ui->img_2->setPixmap(pix->scaled(w,h,Qt::KeepAspectRatio,Qt::SmoothTransformation));
        }
        else
        {
            ui->img2_2->setPixmap(pix->scaled(w,h,Qt::KeepAspectRatio,Qt::SmoothTransformation));
        }
    }
}

void MainWindow::showScreenshot(const QImage *image, Side s)
{

    if(image == nullptr || image->isNull())
    {
        showStatusBarMsg("there no screenshot to show");
    }
    else
    {
        QPixmap pix = QPixmap::fromImage(*image,Qt::AutoColor);
        int w = pix.width()/3;
        int h = pix.height()/3;
        if(s == MainWindow::LEFT)
        {
            ui->img_2->setPixmap(pix.scaled(w,h,Qt::KeepAspectRatio,Qt::SmoothTransformation));
        }
        else
        {
            ui->img2_2->setPixmap(pix.scaled(w,h,Qt::KeepAspectRatio,Qt::SmoothTransformation));
        }
    }
}

void MainWindow::showImgFromDB(const QPixmap *img)
{
    int w = img->width()/2;
    int h = img->height()/2;
    ui->imgDB->setPixmap(img->scaled(w,h,Qt::KeepAspectRatio,Qt::SmoothTransformation));
}

void MainWindow::showStatusBarMsg(const QString msg)
{
    MainWindow::ui->statusbar->showMessage(msg,5000);
}

void MainWindow::on_StartStopButton_2_clicked()
{
    if(timer->isActive())
    {
        timer->stop();
        ui->StartStopButton_2->setText("START");
    }
    else
    {
        timer->start(60000);
        ui->StartStopButton_2->setText("STOP");
    }

}
void MainWindow::on_Update_clicked()
{
    db->selectImage(screenshot);
}

void MainWindow::on_DBTable_clicked(const QModelIndex &index)
{
    db->setTableIndex(index.row());
    db->selectImage(screenshot);
    showImgFromDB(screenshot->getImgFromDB());
}

void MainWindow::on_ClearScreen_2_clicked()
{
    QPixmap tmp;
    tmp.fill(Qt::white);
    ui->img_2->setPixmap(tmp);
    ui->img2_2->setPixmap(tmp);
}

void MainWindow::on_SaveCurrentScreenshot_2_clicked()
{
    if(!isMainShotExist())
    {
        showStatusBarMsg("there nothing to save");
        return;
    }
    const QString dirName = "Screenshots";
    //const QString filePath = QDir::homePath();
    const QString currentPath = QDir::currentPath();
    QDir dir(currentPath);

    if(dir.mkdir(dirName))
        showStatusBarMsg("successfully created");

    const QDateTime now = QDateTime::currentDateTimeUtc();
    const QString timestamp = now.toString("yy.MM.dd. hh_mm_ss");
    const QString fileName = QString::fromLatin1("%1/%2/screenshot-%3.png").arg(dir.absolutePath()).arg(dirName).arg(timestamp);

    if(screenshot->getMainShot()->save(fileName,"PNG", 100) )
        showStatusBarMsg("successfully saved");
    else
        showStatusBarMsg("something went wrong");
}

void MainWindow::on_ShowOlderImg_2_clicked()
{
    showScreenshot(screenshot->getPrevShot(),RIGHT);
}

void MainWindow::on_ShowNewerImg_2_clicked()
{
    showScreenshot(screenshot->getMainShot(),LEFT);
}

void MainWindow::on_CompareImages_2_clicked()
{
    if(!isMainShotExist() || !isPreviousShotExist())
    {
        showStatusBarMsg("need more screenshots");
        return;
    }
    int width1 = screenshot->getMainShot()->width();
    int height1 = screenshot->getMainShot()->height();
    int width2 = screenshot->getPrevShot()->width();
    int height2 = screenshot->getPrevShot()->height();

    if( (width1 != width2) || (height1 != height2) )
    {
        showStatusBarMsg("need screenshots with same size");
        return;
    }

    ui->CompareImages_2->setEnabled(false);
    emit startComputaion();
}

void MainWindow::handleResults()
{
    QString averageError = "Average RGB error: ";
    QString relativeError = "Relative error: ";
    relativeError += QString::number(screenshot->getRelativeError()) +"%" ;
    averageError += QString::number(screenshot->getAverageError()) +"%";

    ui->Data_2->setText(averageError);
    ui->Percentage_2->setText(relativeError);

    ui->CompareImages_2->setEnabled(true);

    if( !screenshot->convertToByteArray() )
    {
        showStatusBarMsg("byte array is empty");
        return;
    }
    screenshot->computeHash();
    db->insertIntoTable(screenshot);
}

void MainWindow::onTimeOut()
{
    QScreen *screen = QGuiApplication::primaryScreen();
    QPixmap *p = new QPixmap(screen->grabWindow(QWidget::winId()));

    screenshot->setPrevShot();
    screenshot->setMainShot(p);
    on_ShowNewerImg_2_clicked();
    on_ShowOlderImg_2_clicked();
    if(!isMainShotExist() || !isPreviousShotExist())
    {
        return;
    }
    emit startComputaion();
}

void MainWindow::on_Difference_2_clicked()
{
    if(screenshot->getDiffShot() != nullptr)
        showScreenshot(screenshot->getDiffShot(), MainWindow::RIGHT);
}

void DataBase::connectTableView()
{
    model = new QSqlTableModel(this, dBase);
    model->setTable(TABLE_NAME);

    model->select();
    model->setSort(4,Qt::DescendingOrder);
}

bool DataBase::connectDB()
{
    createDBDir();

    dbPath = homePath + DATABASE_NAME;

    if(!QFile(dbPath).exists())
    {
        if(openDB())
            return createTable();
        else
            return false;
    }
    else
    {
        return openDB();
    }
}

bool DataBase::openDB()
{
    dBase = QSqlDatabase::addDatabase("QSQLITE");
    dBase.setDatabaseName(dbPath);

    if(!dBase.open())
    {
        //qDebug()<<dBase.lastError().text();
        return false;
    }
    return true;
}

void DataBase::closeDB()
{
    dBase.close();
}

bool DataBase::createDBDir()
{
    const QString dirName = "DB/";
    homePath = QDir::homePath();
    QString OS = QSysInfo::productType();
    if ( OS == "macos" || OS == "osx" )
        homePath += "/Documents/";
    QDir dir(homePath);
    homePath += dirName;

    if(!dir.mkdir(dirName))
        return false;

    return true;
    //qDebug()<<"dir "+dirName+" successfully created at "+homePath;
}

bool DataBase::createTable()
{
    query = new QSqlQuery (dBase);
    QString createTable = "CREATE TABLE Screenshots(img BLOB, hash BLOB, similarity NUMBER, "
                          "id integer PRIMARY KEY AUTOINCREMENT UNIQUE) ";

    if( !query->exec(createTable))
    {
        return false;
    }
    return true;
}

bool DataBase::insertIntoTable(Screenshoter *scr)
{
    if(!dBase.open())
    {
        //qDebug()<<db.lastError().text();
        return false;
    }
    query = new QSqlQuery (dBase);
    query->prepare("INSERT INTO Screenshots (img, hash, similarity) "
                   "VALUES (:img, :hash, :similarity)");
    query->bindValue(":img", *scr->getImageByteArray());
    query->bindValue(":hash", *scr->getHash());
    query->bindValue(":similarity", scr->getRelativeError());
    if( !query->exec() )
    {
        //qDebug()<<query->lastError().text();
        //qDebug()<<query->lastError().driverText();
        delete query;
        return false;
    }
    delete query;
    return true;
}

bool DataBase::selectFromTable(Screenshoter *scr)
{
    if(!dBase.open())
    {
        //qDebug()<<dBase.lastError().text();
        return false;
    }

    query = new QSqlQuery (dBase);
    query->setForwardOnly(true);
    query->prepare("SELECT img FROM Screenshots ORDER BY id DESC LIMIT 2");

    if( !query->exec() )
    {
        //qDebug()<<query->lastError().text();
        //qDebug()<<query->lastError().driverText();
        delete query;
        return false;
    }
    query->next();

    QByteArray outByteArray = query->value( 0 ).toByteArray();
    scr->getMainShot()->loadFromData( outByteArray );

    query->next();
    outByteArray.clear();

    outByteArray = query->value( 0 ).toByteArray();
    scr->getPrevShot()->loadFromData( outByteArray );

    delete query;
    return true;
}

void DataBase::selectImage(Screenshoter *scr)
{
    model->select();
    QByteArray outByteArray = model->record(tableIndex).value(0).toByteArray();
    scr->getImgFromDB()->loadFromData( outByteArray );
}

QSqlTableModel *DataBase::getModel()
{
    return model;
}






