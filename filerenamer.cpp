/*
 * filerenamer.cpp - (c) 2003 Frerich Raabe <raabe@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include "filerenamer.h"
#include "playlistitem.h"
#include "tag.h"

#include <kapplication.h>
#include <kconfig.h>
#include <kdialogbase.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <kio/netaccess.h>
#include <klocale.h>
#include <kmacroexpander.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>

#include <qdir.h>
#include <qhbox.h>
#include <qheader.h>
#include <qlabel.h>
#include <qvbox.h>
#include <qpainter.h>
#include <qsimplerichtext.h>
#include <qstylesheet.h>

class FileRenamer::ConfirmationDialog : public KDialogBase
{
public:
    ConfirmationDialog(const QMap<QString, QString> &files,
                       QWidget *parent = 0, const char *name = 0)
        : KDialogBase(parent, name, true, i18n("Warning"), Ok | Cancel)
    {
        QVBox *vbox = makeVBoxMainWidget();
        QHBox *hbox = new QHBox(vbox);

        QLabel *l = new QLabel(hbox);
        l->setPixmap(SmallIcon("messagebox_warning", 32));

        l = new QLabel(i18n("You are about to rename the following files. "
                            "Are you sure you want to continue?"), hbox);
        hbox->setStretchFactor(l, 1);

        KListView *lv = new KListView(vbox);

        lv->addColumn(i18n("Original Name"));
        lv->addColumn(i18n("New Name"));

        int lvHeight = 0;

        QMap<QString, QString>::ConstIterator it = files.begin();
        for(; it != files.end(); ++it) {
            KListViewItem *i = it.key() != it.data()
                ? new KListViewItem(lv, it.key(), it.data())
                : new KListViewItem(lv, it.key(), i18n("No Change"));
            lvHeight += i->height();
        }

        lvHeight += lv->horizontalScrollBar()->height() + lv->header()->height();
        lv->setMinimumHeight(QMIN(lvHeight, 400));
        resize(QMIN(width(), 500), QMIN(minimumHeight(), 400));
    }
};

FileRenamer::Config::Config(KConfigBase *cfg)
    : m_grp(cfg, "FileRenamer")
{
}

QString FileRenamer::Config::filenameScheme() const
{
    return m_grp.readEntry("FilenameScheme", QDir::homeDirPath() + "/Music/%a%A%T%t%c");
}

void FileRenamer::Config::setFilenameScheme(const QString &scheme)
{
    m_grp.writeEntry("FilenameScheme", scheme);
}

QString FileRenamer::Config::getToken(TokenType type) const
{
    QString fallback;
    switch(type) {
        case Title: fallback = "%s"; break;
        case Artist: fallback = "%s/"; break;
        case Album: fallback = "%s/"; break;
        case Track: fallback = "[%s] "; break;
        case Comment: fallback = " (%s)"; break;
    }
    return m_grp.readEntry(tokenToString(type) + "Token", fallback);
}

void FileRenamer::Config::setToken(TokenType type, const QString &value)
{
    m_grp.writeEntry(tokenToString(type) + "Token", value);
}

bool FileRenamer::Config::tokenNeedsValue(TokenType type) const
{
    bool fallback = type != Title ? true : false;
    return m_grp.readBoolEntry("Need" + tokenToString(type) + "Value", fallback);
}

void FileRenamer::Config::setTokenNeedsValue(TokenType type, bool needsValue)
{
    m_grp.writeEntry("Need" + tokenToString(type) + "Value", needsValue);
}

QString FileRenamer::tokenToString(TokenType type)
{
    switch(type) {
        case Title: return "Title";
        case Artist: return "Artist";
        case Album: return "Album";
        case Track: return "Track";
        case Comment: return "Comment";
    }
    return QString::null;
}

FileRenamer::FileRenamer()
    : m_cfg(kapp->config())
{
}

FileRenamer::FileRenamer(PlaylistItem *item)
    : m_cfg(kapp->config())
{
    rename(item);
}

QString FileRenamer::expandToken(TokenType type, const QString &value_) const
{
    const bool needValue = m_cfg.tokenNeedsValue(type);

    QString value = value_;
    QString token = m_cfg.getToken(type);
    if(value.find(QDir::separator()) > -1) {
        kdWarning() << "Found token value with dir separators!" << endl;
        value.replace(QDir::separator(), "");
    }

    if((type == Track) && needValue && (value.toUInt() == 0))
        return QString::null;

    if((needValue) && value.isEmpty())
        return QString::null;

    token.replace("%s", value);
    return token;
}

void FileRenamer::rename(PlaylistItem *item)
{
    if(item == 0 || item->file().tag() == 0)
        return;

    QString newFilename = rename(item->file().absFilePath(), *item->file().tag());

    // Don't try to rename the file if the name didn't change.
    if(QFileInfo(newFilename).absFilePath() == item->file().absFilePath())
        return;

    QString message = i18n("<qt>You are about to rename the file<br/><br/>'%1'<br/><br/> to <br/>"
                           "<br/>'%2'<br/><br/>Are you sure you want to continue?</qt>");
    message = message.arg(QStyleSheet::escape(item->file().absFilePath()));
    message = message.arg(QStyleSheet::escape(newFilename));

    if(KMessageBox::warningContinueCancel(0, message, i18n("Warning"), KStdGuiItem::cont(),
                                          "ShowFileRenamerWarning") == KMessageBox::Continue)
    {
        if(moveFile(item->file().absFilePath(), newFilename))
            item->setFile(FileHandle(newFilename));
        else
            KMessageBox::error(0, i18n("<qt>Failed to rename the file<br/><br/>'%1'<br/><br/>to<br/><br/>'%2'</qt>")
                               .arg(item->file().absFilePath()).arg(newFilename));
    }
}

void FileRenamer::rename(const PlaylistItemList &items)
{
    QMap<QString, QString> map;
    QMap<QString, PlaylistItem *> itemMap;
    QStringList errorFiles;

    PlaylistItemList::ConstIterator it = items.begin();
    for(; it != items.end(); ++it) {
        if(!*it || !(*it)->file().tag())
            continue;

        const QString oldName = (*it)->file().absFilePath();
        map[oldName] = rename(oldName, *(*it)->file().tag());
        itemMap[oldName] = *it;
    }

    if(ConfirmationDialog(map).exec() == QDialog::Accepted) {

        KApplication::setOverrideCursor(Qt::waitCursor);
        int j = 1;
        QMap<QString, QString>::ConstIterator it = map.begin();
        for(; it != map.end(); ++it, ++j) {
            // Don't rename a file if the name didn't change.
            if(QFileInfo(it.key()).absFilePath() == QFileInfo(it.data()).absFilePath())
                continue;

            if(moveFile(it.key(), it.data()))
                itemMap[it.key()]->setFile(FileHandle(it.data()));
            else
                errorFiles << QString("%1 to %2").arg(it.key()).arg(it.data());

            if(j % 5 == 0)
                kapp->processEvents();
        }
        KApplication::restoreOverrideCursor();
    }

    if(!errorFiles.isEmpty())
        KMessageBox::error(0, i18n("The following rename operations failed:\n") + errorFiles.join("\n"));
}

QString FileRenamer::rename(const QString &filename, const Tag &tag) const
{
    QString newFilename = m_cfg.filenameScheme();

    QMap<QChar, QString> substitutions;
    substitutions[ 't' ] = expandToken(Title, tag.title());
    substitutions[ 'a' ] = expandToken(Artist, tag.artist());
    substitutions[ 'A' ] = expandToken(Album, tag.album());
    substitutions[ 'T' ] = expandToken(Track, QString::number(tag.track()));
    substitutions[ 'c' ] = expandToken(Comment, tag.comment());

    newFilename = KMacroExpander::expandMacros(newFilename, substitutions);
    newFilename = newFilename.stripWhiteSpace();

    if(QFileInfo(newFilename).isRelative())
        newFilename = filename.left( filename.findRev( "/" ) )
            + "/" + newFilename;
    newFilename += "." + QFileInfo(filename).extension(false);

    return newFilename;
}

bool FileRenamer::moveFile(const QString &src, const QString &dest)
{
    kdDebug(65432) << "Moving file " << src << " to " << dest << endl;

    if(src == dest)
        return false;

    // Escape URL.
    KURL srcURL = KURL::fromPathOrURL(src);
    KURL dstURL = KURL::fromPathOrURL(dest);

    // Clean it.
    srcURL.cleanPath();
    dstURL.cleanPath();

    // Make sure it is valid.
    if(!srcURL.isValid() || !dstURL.isValid())
        return false;

    // Get just the directory.
    KURL dir = dstURL;
    dir.setFileName(QString::null);

    // Create the directory.
    if(!KStandardDirs::exists(dir.path()))
        if(!KStandardDirs::makeDir(dir.path()))
            return false;

    // Move the file.
    return KIO::NetAccess::file_move(srcURL, dstURL);
}

// vim:ts=4:sw=4:et
