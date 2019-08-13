#include "mainwindow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStatusBar>
#include <QMenuBar>
#include <QHeaderView>
#include <QLabel>

#include <QFileDialog>
#include <QFontDialog>
#include <QFont>
#include <QMessageBox>

#include <QApplication>

#include <QCloseEvent>
#include <QTextCodec>

#include <QSettings>
#include <QFileInfo>

#include <QDebug>

#include "TableCopier.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      model(nullptr)
{
    setCentralWidget(new QWidget);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget());
    {
        QHBoxLayout *hLayout = new QHBoxLayout;
        layout->addLayout(hLayout);

        hLayout->addWidget(new QLabel(tr("Encoding")));
        codecComboBox = new QComboBox;
        hLayout->addWidget(codecComboBox);

        for (int mib : QTextCodec::availableMibs()) {
            const QTextCodec *codec = QTextCodec::codecForMib(mib);
            codecComboBox->addItem(codec->name(), codec->mibEnum());
        }
        connect(codecComboBox, SIGNAL(currentIndexChanged(int)), SLOT(updateDelegateCodec()));
    }
    {
        QFont f("Monospace");
        f.setStyleHint(QFont::Monospace);
        dataView = new QTableView;
        layout->addWidget(dataView);
        dataView->setItemDelegate(&delegate);
//        dataView->horizontalHeader()->hide();
        dataView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        dataView->setFont(f);

        new TableCopier(dataView);
    }

    setMenuBar(new QMenuBar);
    {
        {
            QMenu * fileMenu = menuBar()->addMenu("File");
            fileMenu->addAction("Open", this, MainWindow::read,  QKeySequence(tr("Ctrl+O")));
            fileMenu->addAction("Diff", this, MainWindow::diff, QKeySequence(tr("Ctrl+D")));
            fileMenu->addAction("Save", this, MainWindow::save, QKeySequence(tr("Ctrl+S")));
            fileMenu->addAction("Save as...", this, MainWindow::saveAs);
        }
        {
            QMenu * settingsMenu = menuBar()->addMenu("Settings");
            settingsMenu->addAction("Fonts", this, selectFont);
        }
    }

    setStatusBar(new QStatusBar);
    setMinimumSize(800, 400);
    updateDelegateCodec();
}

MainWindow::~MainWindow()
{
}

void MainWindow::read()
{
    QString fileName = getOpenFileName();
    if(!fileName.isEmpty()){
        ByteArrayListModel *tmpModel = new ByteArrayListModel(this);

        if(tmpModel->open(fileName)){

            if(dataView->model()){
                dataView->model()->deleteLater();
            }

            dataView->setModel(tmpModel);
            model = tmpModel;

            setWindowTitle(titleBase());
            statusBar()->showMessage(QString("File %1 is opened").arg(
                                         tmpModel->getFilename()), 10000);

            connect(tmpModel, SIGNAL(cacheChanged()), SLOT(cacheChanged()));
            connect(tmpModel, SIGNAL(cacheSaved()), SLOT(cacheSaved()));
        }else{
            QMessageBox::critical(0, tr("File is not opened"), tr("Can't to open the file"));
            tmpModel->deleteLater();
            tmpModel = nullptr;
        }
    }else{
        statusBar()->showMessage("File opening error", 10000);
    }
}

void MainWindow::diff()
{
    QString fileName1 = getOpenFileName();
    QString fileName2 = fileName1.isEmpty() ? QString() : getOpenFileName();

    if((!fileName1.isEmpty()) && (!fileName2.isEmpty())){

        ByteArrayListModel *model1 = new ByteArrayListModel(this);
        ByteArrayListModel *model2 = new ByteArrayListModel(this);
        ByteArrayDiffModel *tmpModel = new ByteArrayDiffModel(model1, model2);

        if(model1->open(fileName1) &&
                model2->open(fileName2) &&
                (fileName1 != fileName2)){

            if(dataView->model()){
                dataView->model()->deleteLater();
            }

            dataView->setModel(tmpModel);
            this->model = tmpModel;

            setWindowTitle(titleBase());
            statusBar()->showMessage(
                        QString("Files %1, %2 is opened").arg(
                            model1->getFilename()).arg(
                            model2->getFilename()), 10000);

            connect(tmpModel, SIGNAL(cacheChanged()), SLOT(cacheChanged()));
            connect(tmpModel, SIGNAL(cacheSaved()), SLOT(cacheSaved()));

            if(model1->getHash() == model2->getHash()){
                QMessageBox::information(this, "Files are equal", "Files are equal");
            }else{
                QMessageBox::critical(this, "Files are different", "Files are different");
            }
        }else{
            tmpModel->deleteLater();
            tmpModel = nullptr;
        }
    }else{
        statusBar()->showMessage("File opening error", 10000);
    }
}

QString MainWindow::getOpenFileName()
{
    QSettings settings("ilyayunkin", QApplication::applicationName());
    QString valueName = "dirName";
    QString dir = settings.value(valueName).toString();

    QString fileName = QFileDialog::getOpenFileName(this, "Open file", dir,
                                                    "Executable (*.exe);;"
                                                    "DLL (*.dll);;"
                                                    "Binary (*.bin);;"
                                                    "All (*.*)");
    if(!fileName.isEmpty()){
        settings.setValue(valueName, QFileInfo(fileName).absolutePath());
    }

    return fileName;
}

void MainWindow::saveAs()
{
    if(model != nullptr){
        const QString filename = QFileDialog::getSaveFileName(0, "Save file", QString(),
                                                              "Executable (*.exe);;"
                                                              "DLL (*.dll);;"
                                                              "Binary (*.bin);;"
                                                              "All (*.*)");

        if(!filename.isEmpty()){
            if(!model->saveAs(filename)){
                QMessageBox::critical(0, tr("File writing error"), tr("Can't to write the file"));
            }else{
                setWindowTitle(titleBase());
            }
        }
    }
}

void MainWindow::save()
{
    if(model != nullptr){
        if(!model->save()){
            QMessageBox::critical(0, tr("File writing error"), tr("Can't to write the file"));
        }
    }
}

void MainWindow::selectFont()
{
    bool got = false;
    QFont font = QFontDialog::getFont(&got, QApplication::font(), 0, QString(),
                                      QFontDialog::MonospacedFonts);
    if(got){
        dataView->setFont(font);
    }
}

void MainWindow::cacheChanged()
{
    setWindowTitle(titleBase() + "*");
}

void MainWindow::cacheSaved()
{
    setWindowTitle(titleBase());
}

void MainWindow::updateDelegateCodec()
{
    const int mib = codecComboBox->itemData(codecComboBox->currentIndex()).toInt();
    QTextCodec *codec = QTextCodec::codecForMib(mib);
    delegate.setCodec(codec);
    dataView->viewport()->update();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if ((model != nullptr) && model->isEdited()) {
        int button = QMessageBox::question(nullptr, "Close request",
                              "There are unsaved changes.\r\nSure to close the application?",
                              QMessageBox::Ok,
                              QMessageBox::Cancel);
        if(button == QMessageBox::Ok){
            event->accept();
        }else {
            event->ignore();
        }
    }
}

QString MainWindow::titleBase() const
{
    QString str = QApplication::applicationName();

    if(model != nullptr){
        str+= QString(" - ") + model->getFilename();
    }

    return str;
}
