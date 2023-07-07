/* This file is part of XTVFS Reader.
 * Copyright (C) 2014 S. Blackburn
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * XTVFS Reader is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with XTVFS Reader.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QModelIndex>
#include <QtSql>
#include <QTemporaryFile>

#include "filesystem.h"
#include <deque>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
private slots:
    void on_actionOpen_triggered();

    void on_tableWidget_activated(const QModelIndex &index);

    void on_actionUp_triggered();

    void on_actionAbout_XTVFS_Reader_triggered();

    void on_actionExport_Planner_Database_triggered();

    void on_actionExtract_Recording_triggered();

    void on_actionExit_triggered();

    void on_recordingsTW_doubleClicked(const QModelIndex &index);

private:
    Ui::MainWindow *ui;
    fs::FileSystem *diskImage;

    QSqlDatabase db;
    QTemporaryFile pcatfile;

    typedef std::deque<fs::DirEntry> DirStack;
    DirStack m_dirStack;

    QString currentPath() const;
    void showDirectory(size_t startCluster = (size_t)-1);
    void openSkyDB();
    void closeImage();

};

#endif // MAINWINDOW_H
