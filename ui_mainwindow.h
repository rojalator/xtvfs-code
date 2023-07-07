/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.5.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *actionOpen;
    QAction *actionUp;
    QAction *actionAbout_XTVFS_Reader;
    QAction *actionExtract_Recording;
    QAction *actionExport_Planner_Database;
    QAction *actionExit;
    QWidget *centralWidget;
    QVBoxLayout *verticalLayout_2;
    QSplitter *splitter;
    QFrame *FilesystemFrame;
    QGridLayout *gridLayout;
    QLabel *label_2;
    QLabel *pathLbl;
    QTableWidget *tableWidget;
    QFrame *SkyDBFrame;
    QVBoxLayout *verticalLayout;
    QTableWidget *recordingsTW;
    QMenuBar *menuBar;
    QMenu *menuFile;
    QMenu *menuHelp;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QStringLiteral("MainWindow"));
        MainWindow->resize(1154, 1003);
        actionOpen = new QAction(MainWindow);
        actionOpen->setObjectName(QStringLiteral("actionOpen"));
        actionUp = new QAction(MainWindow);
        actionUp->setObjectName(QStringLiteral("actionUp"));
        actionUp->setEnabled(false);
        actionAbout_XTVFS_Reader = new QAction(MainWindow);
        actionAbout_XTVFS_Reader->setObjectName(QStringLiteral("actionAbout_XTVFS_Reader"));
        actionExtract_Recording = new QAction(MainWindow);
        actionExtract_Recording->setObjectName(QStringLiteral("actionExtract_Recording"));
        actionExport_Planner_Database = new QAction(MainWindow);
        actionExport_Planner_Database->setObjectName(QStringLiteral("actionExport_Planner_Database"));
        actionExit = new QAction(MainWindow);
        actionExit->setObjectName(QStringLiteral("actionExit"));
        centralWidget = new QWidget(MainWindow);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        verticalLayout_2 = new QVBoxLayout(centralWidget);
        verticalLayout_2->setSpacing(0);
        verticalLayout_2->setContentsMargins(11, 11, 11, 11);
        verticalLayout_2->setObjectName(QStringLiteral("verticalLayout_2"));
        verticalLayout_2->setContentsMargins(0, 0, 0, 0);
        splitter = new QSplitter(centralWidget);
        splitter->setObjectName(QStringLiteral("splitter"));
        splitter->setOrientation(Qt::Vertical);
        FilesystemFrame = new QFrame(splitter);
        FilesystemFrame->setObjectName(QStringLiteral("FilesystemFrame"));
        FilesystemFrame->setFrameShape(QFrame::StyledPanel);
        FilesystemFrame->setFrameShadow(QFrame::Raised);
        gridLayout = new QGridLayout(FilesystemFrame);
        gridLayout->setSpacing(6);
        gridLayout->setContentsMargins(11, 11, 11, 11);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        label_2 = new QLabel(FilesystemFrame);
        label_2->setObjectName(QStringLiteral("label_2"));

        gridLayout->addWidget(label_2, 0, 0, 1, 1);

        pathLbl = new QLabel(FilesystemFrame);
        pathLbl->setObjectName(QStringLiteral("pathLbl"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(pathLbl->sizePolicy().hasHeightForWidth());
        pathLbl->setSizePolicy(sizePolicy);

        gridLayout->addWidget(pathLbl, 0, 1, 1, 1);

        tableWidget = new QTableWidget(FilesystemFrame);
        tableWidget->setObjectName(QStringLiteral("tableWidget"));
        tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
        tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
        tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);

        gridLayout->addWidget(tableWidget, 1, 0, 1, 2);

        splitter->addWidget(FilesystemFrame);
        SkyDBFrame = new QFrame(splitter);
        SkyDBFrame->setObjectName(QStringLiteral("SkyDBFrame"));
        SkyDBFrame->setFrameShape(QFrame::StyledPanel);
        SkyDBFrame->setFrameShadow(QFrame::Raised);
        verticalLayout = new QVBoxLayout(SkyDBFrame);
        verticalLayout->setSpacing(6);
        verticalLayout->setContentsMargins(11, 11, 11, 11);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        recordingsTW = new QTableWidget(SkyDBFrame);
        if (recordingsTW->columnCount() < 5)
            recordingsTW->setColumnCount(5);
        QTableWidgetItem *__qtablewidgetitem = new QTableWidgetItem();
        recordingsTW->setHorizontalHeaderItem(0, __qtablewidgetitem);
        QTableWidgetItem *__qtablewidgetitem1 = new QTableWidgetItem();
        recordingsTW->setHorizontalHeaderItem(1, __qtablewidgetitem1);
        QTableWidgetItem *__qtablewidgetitem2 = new QTableWidgetItem();
        recordingsTW->setHorizontalHeaderItem(2, __qtablewidgetitem2);
        QTableWidgetItem *__qtablewidgetitem3 = new QTableWidgetItem();
        recordingsTW->setHorizontalHeaderItem(3, __qtablewidgetitem3);
        QTableWidgetItem *__qtablewidgetitem4 = new QTableWidgetItem();
        recordingsTW->setHorizontalHeaderItem(4, __qtablewidgetitem4);
        recordingsTW->setObjectName(QStringLiteral("recordingsTW"));
        recordingsTW->setSelectionBehavior(QAbstractItemView::SelectRows);
        recordingsTW->setSortingEnabled(true);
        recordingsTW->horizontalHeader()->setStretchLastSection(true);

        verticalLayout->addWidget(recordingsTW);

        splitter->addWidget(SkyDBFrame);

        verticalLayout_2->addWidget(splitter);

        MainWindow->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(MainWindow);
        menuBar->setObjectName(QStringLiteral("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 1154, 21));
        menuFile = new QMenu(menuBar);
        menuFile->setObjectName(QStringLiteral("menuFile"));
        menuHelp = new QMenu(menuBar);
        menuHelp->setObjectName(QStringLiteral("menuHelp"));
        MainWindow->setMenuBar(menuBar);
        mainToolBar = new QToolBar(MainWindow);
        mainToolBar->setObjectName(QStringLiteral("mainToolBar"));
        MainWindow->addToolBar(Qt::TopToolBarArea, mainToolBar);
        statusBar = new QStatusBar(MainWindow);
        statusBar->setObjectName(QStringLiteral("statusBar"));
        MainWindow->setStatusBar(statusBar);

        menuBar->addAction(menuFile->menuAction());
        menuBar->addAction(menuHelp->menuAction());
        menuFile->addAction(actionOpen);
        menuFile->addSeparator();
        menuFile->addAction(actionExit);
        menuHelp->addAction(actionAbout_XTVFS_Reader);
        mainToolBar->addAction(actionOpen);
        mainToolBar->addAction(actionUp);
        mainToolBar->addSeparator();
        mainToolBar->addAction(actionExport_Planner_Database);
        mainToolBar->addAction(actionExtract_Recording);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "XTVFS Reader", 0));
        actionOpen->setText(QApplication::translate("MainWindow", "Open...", 0));
#ifndef QT_NO_TOOLTIP
        actionOpen->setToolTip(QApplication::translate("MainWindow", "Open a FAT32 or XTVFS image file", 0));
#endif // QT_NO_TOOLTIP
        actionUp->setText(QApplication::translate("MainWindow", "Up", 0));
#ifndef QT_NO_TOOLTIP
        actionUp->setToolTip(QApplication::translate("MainWindow", "Traverse to the parent directory", 0));
#endif // QT_NO_TOOLTIP
        actionAbout_XTVFS_Reader->setText(QApplication::translate("MainWindow", "About XTVFS Reader...", 0));
        actionExtract_Recording->setText(QApplication::translate("MainWindow", "Extract Recording", 0));
        actionExport_Planner_Database->setText(QApplication::translate("MainWindow", "Export Planner Database", 0));
        actionExit->setText(QApplication::translate("MainWindow", "Exit", 0));
        label_2->setText(QApplication::translate("MainWindow", "Current Path:", 0));
        pathLbl->setText(QString());
        QTableWidgetItem *___qtablewidgetitem = recordingsTW->horizontalHeaderItem(0);
        ___qtablewidgetitem->setText(QApplication::translate("MainWindow", "Time", 0));
        QTableWidgetItem *___qtablewidgetitem1 = recordingsTW->horizontalHeaderItem(1);
        ___qtablewidgetitem1->setText(QApplication::translate("MainWindow", "Duration", 0));
        QTableWidgetItem *___qtablewidgetitem2 = recordingsTW->horizontalHeaderItem(2);
        ___qtablewidgetitem2->setText(QApplication::translate("MainWindow", "Name", 0));
        QTableWidgetItem *___qtablewidgetitem3 = recordingsTW->horizontalHeaderItem(3);
        ___qtablewidgetitem3->setText(QApplication::translate("MainWindow", "Channel", 0));
        QTableWidgetItem *___qtablewidgetitem4 = recordingsTW->horizontalHeaderItem(4);
        ___qtablewidgetitem4->setText(QApplication::translate("MainWindow", "Synopsis", 0));
        menuFile->setTitle(QApplication::translate("MainWindow", "File", 0));
        menuHelp->setTitle(QApplication::translate("MainWindow", "Help", 0));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
