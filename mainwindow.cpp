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
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "filesystem.h"

#include <QFileDialog>
#include <QMessageBox>

#include <fstream>
#include <sstream>

using namespace fs;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    diskImage(NULL),
    db( QSqlDatabase::addDatabase("QSQLITE") )
{
    ui->setupUi(this);

    ui->FilesystemFrame->hide();
    ui->SkyDBFrame->hide();
}

MainWindow::~MainWindow()
{
    delete ui;
}

// Format is len(1 byte) + unknown(1 byte) + "13ff1a19"(4 bytes) + data(len bytes)
// So string data starts 12 bytes in
QString blobToString(const QVariant &v)
{
    QString output;
    bool okay;

//    QString s(v.toString().mid(12));
    QString s(v.toString());

    const int len = s.left(2).toInt(&okay, 16);

    for (int i=s.length()-(2*len); i<s.length(); i+=2)
    {
        const int c = s.mid(i, 2).toInt(&okay, 16);
        if (isprint(c))
            output.append(c);
    }

    //qDebug() << s << "->" << len << output;
    return output;
}

void MainWindow::openSkyDB()
{
    db.setDatabaseName(pcatfile.fileName());

    if (!db.open())
    {
        QMessageBox::critical(0, qApp->tr("Cannot open database"),
                              qApp->tr("Unable to establish a database connection.\n"
                                       "This utility needs SQLite support. Please read "
                                       "the Qt SQL driver documentation for information how "
                                       "to build it.\n\n"
                                       "Click Cancel to exit."), QMessageBox::Cancel);
    }
    else
    {
        QSqlQuery infoQuery("SELECT DB_SCHEMA_MAJOR_VERSION, DB_SCHEMA_MINOR_VERSION FROM DB_INFO");
        if ( infoQuery.next() )
        {
            const int dbMajorVersion = infoQuery.value(0).toInt();
            const int dbMinorVersion = infoQuery.value(1).toInt();
            qDebug() << "Database version = " << dbMajorVersion << "." << dbMinorVersion;
        }

        // service_type of 5 is pre-recorded on-demand; 25 is channel BDL (whatever that is); 0, 1, 16 are unknown

        QSqlQuery query("SELECT item.event_id,service_type,event_name,local_start_time,duration,channel_name,shrec_locator,synopsis,av_content.av_content_id,vfile_locator "
                        "FROM item,booking_info,av_content "
                        "WHERE item.event_id = booking_info.event_id and booking_info.av_content_id = av_content.av_content_id and service_type!=5  and service_type!=25");
        int row=0;
        while (query.next())
        {
            qDebug() << query.value(0).toInt() << query.value(1).toString() << blobToString(query.value(2))
                     << query.value(3).toInt() << query.value(4).toInt() << blobToString(query.value(5))
                     << query.value(6).toString() << blobToString(query.value(7)) << query.value(8).toString() << query.value(9).toString();

            const int eventId = query.value(0).toInt();
            const QString eventName(blobToString(query.value(2)));
            const int startTime = query.value(3).toInt();
            const int duration = query.value(4).toInt();
            const QString channel(blobToString(query.value(5)));
            const QString synopsis(blobToString(query.value(7)));
            ui->recordingsTW->setRowCount(row+1);
            //time, duration, name, channel, synopsis
            //ui->recordingsTW->setItem(row, 0, new QTableWidgetItem(QDateTime::fromTime_t(startTime).toString(Qt::ISODate)));
            ui->recordingsTW->setItem(row, 0, new QTableWidgetItem(QDateTime::fromTime_t(startTime).toString("yyyy-MM-dd hh:mm")));
            ui->recordingsTW->setItem(row, 1, new QTableWidgetItem(QString::number(duration/60000)+" mins"));
            ui->recordingsTW->setItem(row, 2, new QTableWidgetItem(eventName));
            ui->recordingsTW->setItem(row, 3, new QTableWidgetItem(channel));
            ui->recordingsTW->setItem(row, 4, new QTableWidgetItem(synopsis));
            ui->recordingsTW->item(row, 0)->setData(Qt::UserRole, QVariant(eventId));
            row++;
        }
        ui->recordingsTW->resizeColumnsToContents();
        ui->recordingsTW->resizeRowsToContents();
        qDebug() << row << "recordings found";
        ui->statusBar->showMessage(QString("%1 recordings found").arg(row), 10000);
    }

    ui->SkyDBFrame->show();
}


void MainWindow::closeImage()
{
    ui->SkyDBFrame->hide();
    ui->FilesystemFrame->hide();
    if (diskImage)
        delete diskImage;
}

#include <set>
void MainWindow::on_actionOpen_triggered()
{
    std::string filepath; /// @todo Suggest the old one

    filepath = QFileDialog::getOpenFileName(this, tr("Select FAT32 / XTVFS image to open"),
                                            QString::fromStdString(filepath),
                                            tr("All Files (*.*)") ).toStdString();
    if (filepath.empty())
        return; // User cancelled the loading

    closeImage();
    diskImage = new fs::Xtvfs();
    bool okay  = diskImage->open(filepath);
    if (!okay)
    {
        // Xtvfs didn't work, try Fat32 instead
        delete diskImage;
        diskImage = new fs::Fat32();
        okay  = diskImage->open(filepath);
    }

    if (!okay)
    {
        ui->statusBar->showMessage("Error opening " + QString::fromStdString(filepath), 10000);
        return;
    }

    ui->FilesystemFrame->show();
    showDirectory();

    // Is this a Sky DB disk image?
    DirEntry skyDbFileInfo = diskImage->infoFor("FSN_DATA/PCAT.DB");
    if (!skyDbFileInfo.filename.empty())
    {
        // Copy the pcat.db file to a temporary
        pcatfile.open();
        std::ofstream f(pcatfile.fileName().toStdString().c_str(), std::ios::out | std::ios::binary);
        qDebug() << "About to copy the pcat file to" << pcatfile.fileName();
        bool okay = diskImage->copyFile(f, "FSN_DATA/PCAT.DB");
        f.close();

        if (okay)
            // Open sky db
            openSkyDB();
        else
            qDebug() << "Error copying the pcat file";
    }
    else
        ui->SkyDBFrame->hide();

#if defined(TEST_FAT_FOR_REUSED_CLUSTERS)
    // Was using this to validate the FAT chain reader - seeing if there was a clash in clusters used.
    char *names[] = {"/s1/stream.str",
                     "/s2/stream.str",
                     "/s3/stream.str",
                     "/s4/stream.str",
                     "/s5/stream.str",
                     "/s6/stream.str",
                     "/s7/stream.str",
                     "/s8/stream.str",
                     "/s9/stream.str",
                     "/s10/stream.str",
                     "/s11/stream.str",
                     "/s12/stream.str",
                     "/s13/stream.str",
                     "/s14/stream.str",
                     "/s15/stream.str",
                     NULL};
    std::set<size_t> allClusters;
    for (int i=0; names[i] != NULL; ++i)
    {
        const std::list<size_t> clusters = dynamic_cast<Xtvfs*>(diskImage)->getAllocationChain(names[i]);
        // 3008 * 512
        for (std::list<size_t>::const_iterator c=clusters.begin(); c!=clusters.end(); ++c)
        {
            if (allClusters.find(*c) != allClusters.end())
            {
                qDebug() << "!!! Found a reused cluster at " << *c;
            }
            allClusters.insert(*c);
        }
    }
    qDebug() << "Total of" << allClusters.size() << "clusters";
//    std::list<size_t> clusters2 = dynamic_cast<Xtvfs*>(diskImage)->getAllocationChain("/s9/stream.str");
#endif
}


QString MainWindow::currentPath() const
{
    QString path;

    for (DirStack::const_iterator i=m_dirStack.begin(); i!=m_dirStack.end(); ++i)
        path.append( QString::fromStdString(fs::from11CharFormat(i->filename)) + "/" );

    return path;
}


void MainWindow::showDirectory(size_t startCluster)
{
    DirEntries entries = diskImage->readDirectory(startCluster);
    ui->statusBar->showMessage(QString("Read %1 entries").arg(entries.size()), 10000);

    DirEntry entry;
    ui->tableWidget->clear();
    ui->tableWidget->setRowCount(entries.size());
    ui->tableWidget->setColumnCount(3);
    QStringList headers;
    headers << "Filename" << "Attributes" << "Size";
    ui->tableWidget->setHorizontalHeaderLabels(headers);
    int row=0;
    foreach (entry, entries)
    {
        ui->tableWidget->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(entry.toString())));
        ui->tableWidget->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(entry.attribToString())));
        ui->tableWidget->setItem(row, 2, new QTableWidgetItem(QString::number(entry.filesize)));

        row++;
    }

    ui->actionUp->setEnabled(!m_dirStack.empty());
    ui->pathLbl->setText("/" + currentPath());
}

void MainWindow::on_tableWidget_activated(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    const QString name = ui->tableWidget->item(index.row(), 0)->text();
    ui->statusBar->showMessage("Something pressed:" + name, 10000);
    if (name == "..")
    {
        ui->actionUp->trigger();
    }
    else
    {
        DirEntry fileInfo = diskImage->infoFor((currentPath() + name).toStdString());
        if (fileInfo.isDirectory())
        {
            m_dirStack.push_back(fileInfo);
            showDirectory(fileInfo.firstCluster);
        }
        else if (!fileInfo.isDevice() && !fileInfo.isLFN())
        {
            // Ask if the file should be copied?
            QString savePath = QFileDialog::getSaveFileName(this, tr("Copy file as..."), name,
                                                            tr("All Files (*.*)") );
            if (savePath != "")
            {
                const QString srcPath = currentPath() + name;
                ui->statusBar->showMessage(QString("Copying %1 to %2").arg(srcPath,savePath), 10000);
                // Do the copying!
                std::ofstream f(savePath.toStdString().c_str(), std::ios::out | std::ios::binary);
                const bool okay = diskImage->copyFile(f, srcPath.toStdString());
                f.close();
                if (!okay)
                {
                    ui->statusBar->showMessage("Error copying the file", 10000);
                    QMessageBox::critical(this, "Error copying", QString("Error copying from:\n  %1\nto:\n  %2").arg(srcPath,savePath));
                }
                else
                    ui->statusBar->showMessage("File copied okay", 10000);
            }
        }
    }
}

void MainWindow::on_actionUp_triggered()
{
    m_dirStack.pop_back();

    if (!m_dirStack.empty())
    {
        DirEntry fileInfo = m_dirStack.back();
        showDirectory(fileInfo.firstCluster);
    }
    else
        showDirectory();
}

void MainWindow::on_actionAbout_XTVFS_Reader_triggered()
{
    QMessageBox::about(this, qApp->applicationName(),
                       "XTVFS / FAT32 Image Reader");
}

void MainWindow::on_actionExport_Planner_Database_triggered()
{
    QString csvPath = QFileDialog::getSaveFileName(this, tr("Export planner as..."), "planner.csv",
                                                       tr("CSV Files (*.csv)") );
    if (csvPath == "")
        return;

    QFile f( csvPath );

    if (f.open(QFile::WriteOnly | QFile::Truncate))
    {
        QTextStream data( &f );
        QStringList strList;

        QTableWidget * table = ui->recordingsTW;

        for( int c = 0; c < table->columnCount(); ++c )
        {
            strList <<
                    "\" " +
                    table->horizontalHeaderItem(c)->data(Qt::DisplayRole).toString() +
                    "\" ";
        }

        data << strList.join(",") << "\n";

        for( int r = 0; r < table->rowCount(); ++r )
        {
            strList.clear();
            for( int c = 0; c < table->columnCount(); ++c )
            {
                strList << "\" "+table->item( r, c )->text()+"\" ";
            }
            data << strList.join( "," )+"\n";
        }
        f.close();
    }
}

void MainWindow::on_actionExtract_Recording_triggered()
{
    QTableWidgetItem *item = ui->recordingsTW->selectedItems().front();
    const int eventId = item->data(Qt::UserRole).toInt();
    qDebug() << "eventId =" << eventId;

    QSqlQuery query("SELECT item.event_id,service_type,event_name,local_start_time,duration,channel_name,shrec_locator,synopsis,av_content.av_content_id,vfile_locator "
                    "FROM item,booking_info,av_content "
                    "WHERE item.event_id = booking_info.event_id and booking_info.av_content_id = av_content.av_content_id and service_type!=5  and service_type!=25 and item.event_id=" + QString::number(eventId));
    if (!query.next())
    {
        qDebug() << "Error querying db for extraction";
        return;

    }

    qDebug() << query.value(0).toInt() << query.value(1).toString() << blobToString(query.value(2))
             << query.value(3).toInt() << query.value(4).toInt() << blobToString(query.value(5))
             << query.value(6).toString() << blobToString(query.value(7)) << query.value(8).toString() << query.value(9).toString();

    const QString eventName(blobToString(query.value(2)));

    QString savePath = QFileDialog::getSaveFileName(this, tr("Extract file as..."), eventName + ".STR",
                                                    tr("All Files (*.*)") );
    if (savePath.isEmpty())
        return;

    const QString shrecLocator(query.value(6).toString());
    qDebug() << "Locator = " << shrecLocator << ", splits to" << shrecLocator.split(":");
    const QString stream = shrecLocator.split(":").back();

    qDebug() << "About to save stream" << stream << "to" << savePath;
    const QString videoFile(stream+"/STREAM.STR");

    ui->statusBar->showMessage(QString("Saving %1 to %2").arg(eventName,savePath), 10000);
#if defined(READ_EXTENT_FILE)
    // Read the extent file - use this to check the calculations of the video sector locations used in the copy.
    // See http://wiki.ph-mb.com/wiki/Video_FAT, e.g. in "/s9/stream.exn"
    const QString extentFile(stream+"/STREAM.EXN");
    std::stringstream exn;
    const bool exn_okay = diskImage->copyFile(exn, extentFile.toStdString());
    if (exn_okay)
    {
        qDebug() << "Extent file" << extentFile << "is" << exn.str().size() << "bytes";
        const unsigned int records = exn.str().size() / 8;
        unsigned long start, end;
        for (unsigned int i=0; i<records; i++)
        {
            exn.read((char*)&start, sizeof(start));
            exn.read((char*)&end, sizeof(end));
            qDebug() << start << end;
        }
    }
    else
        qDebug() << "Error reading the extent file:" << extentFile;

return;
#endif

//    std::ofstream f(savePath.toStdString().c_str(), std::ios::out | std::ios::binary);
//    const bool okay = diskImage->copyFile(f, videoFile.toStdString());
//    f.close();
//    const bool okay = dynamic_cast<Xtvfs*>(diskImage)->copyVideoFile(savePath.toStdString() size_t startCluster, size_t bytesToCopy);
    const bool okay = diskImage->copyFile(videoFile.toStdString(), savePath.toStdString());

    if (!okay)
    {
        ui->statusBar->showMessage("Error copying the file", 10000);
        QMessageBox::critical(this, "Error copying", QString("Error copying from:\n  %1\nto:\n  %2").arg(videoFile,savePath));
    }
    else
        ui->statusBar->showMessage("File copied okay", 10000);

    // Now remux the stream with ffmpeg?
    //ffmpeg -i oldfile.mpg -vcodec copy -acodec copy newfile.mpg
}

void MainWindow::on_actionExit_triggered()
{
    qApp->quit();
}

void MainWindow::on_recordingsTW_doubleClicked(const QModelIndex &index)
{
    on_actionExtract_Recording_triggered();
}
