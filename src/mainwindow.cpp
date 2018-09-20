#include "mainwindow.h"

#include <QVBoxLayout>
#include <QStatusBar>
#include <QMenuBar>
#include <QHeaderView>

#include <QFileDialog>
#include <QFontDialog>
#include <QMessageBox>

#include <QApplication>

#include <QCloseEvent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      model(nullptr)
{
    setCentralWidget(new QWidget);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget());

    dataView = new QTableView;
    layout->addWidget(dataView);
    dataView->setItemDelegate(&delegate);
    dataView->horizontalHeader()->hide();
    dataView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    setMenuBar(new QMenuBar);
    {
        {
            QMenu * fileMenu = menuBar()->addMenu("File");
            {
                QAction *openAction = fileMenu->addAction("Open");
                connect(openAction, SIGNAL(triggered(bool)), SLOT(read()));
            }
            {
                saveAction = fileMenu->addAction("Save");
            }
        }
        {
            QMenu * settingsMenu = menuBar()->addMenu("Settings");
            {
                QAction *fontsAction = settingsMenu->addAction("Fonts");
                connect(fontsAction, SIGNAL(triggered(bool)), SLOT(selectFont()));
            }
        }
    }

    setStatusBar(new QStatusBar);
}

MainWindow::~MainWindow()
{
}

void MainWindow::read()
{
    QString fileName = QFileDialog::getOpenFileName(0, "Open file", QString(),
                                                    "Executable (*.exe);;"
                                                    "DLL (*.dll);;"
                                                    "Binary (*.bin);;"
                                                    "All (*.*)");
    if(!fileName.isEmpty()){
        model = new ByteArrayListModel(this);

        if(model->open(fileName)){

            if(dataView->model()){
                dataView->model()->deleteLater();
            }

            dataView->setModel(model);

            setWindowTitle(QApplication::applicationName() + " - " +
                           model->getFilename());
            statusBar()->showMessage(QString("File %1 is opened").arg(
                                         model->getFilename()), 10000);

            connect(model, SIGNAL(cacheChanged()), SLOT(cacheChanged()));
            connect(model, SIGNAL(cacheSaved()), SLOT(cacheSaved()));
            connect(saveAction, SIGNAL(triggered(bool)),
                    model, SLOT(saveCacheToFile()));
        }else{
            model->deleteLater();
            model = nullptr;
        }
    }else{
        statusBar()->showMessage("File opening error", 10000);
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
    setWindowTitle(windowTitle() + "*");
}

void MainWindow::cacheSaved()
{
    setWindowTitle(windowTitle().remove("*"));
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
