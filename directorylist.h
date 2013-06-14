/***************************************************************************
    begin                : Tue Feb 4 2003
    copyright            : (C) 2003 - 2004 by Scott Wheeler
    email                : wheeler@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef DIRECTORYLIST_H
#define DIRECTORYLIST_H

#include <kdialog.h>
#include "ui_directorylistbase.h"

class QStringListModel;

class DirectoryListBase : public QWidget, public Ui::DirectoryListBase
{
public:
    DirectoryListBase(QWidget *parent) : QWidget(parent)
    {
        setupUi(this);
    }
};

class DirectoryList : public KDialog
{
    Q_OBJECT

public:
    struct Result
    {
        QStringList addedDirs;
        QStringList removedDirs;
        QStringList excludedDirs;
        DialogCode status;
        bool addPlaylists;
    };

    DirectoryList(QStringList directories, QStringList excludeDirectories, bool importPlaylists,
                  QWidget *parent = 0);
    virtual ~DirectoryList();

public slots:
    Result exec();

signals:
    void signalDirectoryAdded(const QString &directory);
    void signalDirectoryRemoved(const QString &directory);
    void signalExcludeDirectoryAdded(const QString &directory);
    void signalExcludeDirectoryRemoved(const QString &directory);

private slots:
    void slotAddDirectory();
    void slotRemoveDirectory();
    void slotAddExcludeDirectory();
    void slotRemoveExcludeDirectory();

private:
    static QStringList defaultFolders();

    QStringListModel *m_dirListModel;
    QStringListModel *m_excludedDirListModel;
    DirectoryListBase *m_base;
    Result m_result;
};

#endif

// vim: set et sw=4 tw=0 sta:
