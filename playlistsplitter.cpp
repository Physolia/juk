/***************************************************************************
                          playlistsplitter.cpp  -  description
                             -------------------
    begin                : Fri Sep 13 2002
    copyright            : (C) 2002 by Scott Wheeler
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

#include <klocale.h>
#include <kiconloader.h>
#include <kapplication.h>
#include <kdebug.h>

#include <qinputdialog.h>

#include "playlistsplitter.h"
#include "collectionlist.h"

////////////////////////////////////////////////////////////////////////////////
// helper functions
////////////////////////////////////////////////////////////////////////////////

void processEvents()
{
    static int processed = 0;
    if(processed == 0)
        kapp->processEvents();
    processed = (processed + 1) % 5;
}

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

PlaylistSplitter::PlaylistSplitter(QWidget *parent, bool restoreOnLoad, const char *name) : QSplitter(Qt::Horizontal, parent, name), 
											    playingItem(0), restore(restoreOnLoad)
{
    setupLayout();
    readConfig();
    mediaExtensions.append("mp3");
    mediaExtensions.append("ogg");
    listExtensions.append("m3u");
}

PlaylistSplitter::~PlaylistSplitter()
{
    saveConfig();
}

QString PlaylistSplitter::uniquePlaylistName(const QString &startingWith, bool useParenthesis)
{
    if(!playlistBox)

	return(QString::null);

    QStringList names = playlistBox->names();

    int playlistNumber = 1;

    // while the list contains more than zero instances of the generated 
    // string...

    if(useParenthesis) {
	while(names.contains(startingWith + " (" + QString::number(playlistNumber) + ")") != 0)
	    playlistNumber++;
	
	return(startingWith + " (" + QString::number(playlistNumber) + ")");	
    }
    else
    {
	while(names.contains(startingWith + ' ' + QString::number(playlistNumber)) != 0)
	    playlistNumber++;
	
	return(startingWith + " " + QString::number(playlistNumber));
    }
}

QString PlaylistSplitter::playNextFile(bool random)
{
    Playlist *p;
    PlaylistItem *i;

    if(playingItem) {
	playingItem->setPixmap(0, 0);

	p = static_cast<Playlist *>(playingItem->listView());
	i = p->nextItem(playingItem, random);
    }
    else {
	PlaylistItemList items = playlistSelection();
	if(!items.isEmpty())
	    i = items.first();
	else {
	    p = visiblePlaylist();
	    i = static_cast<PlaylistItem *>(p->firstChild());
	}
    }

    if(i) {
	i->setPixmap(0, QPixmap(UserIcon("playing")));
	playingItem = i;
	return(i->absFilePath());
    }
    else
	return(QString::null);
}

QString PlaylistSplitter::playPreviousFile(bool random)
{
    if(playingItem) {
	Playlist *p = static_cast<Playlist *>(playingItem->listView());
	PlaylistItem *i = p->previousItem(playingItem, random);

	playingItem->setPixmap(0, 0);
	i->setPixmap(0, QPixmap(UserIcon("playing")));
	
	playingItem = i;
	return(i->absFilePath());
    }
    else
	return(QString::null);
}

QString PlaylistSplitter::playSelectedFile()
{
    stop();

    PlaylistItemList items = playlistSelection();

    if(!items.isEmpty()) {
	PlaylistItem *i = items.first();
	i->setPixmap(0, QPixmap(UserIcon("playing")));
	
	playingItem = i;
	return(i->absFilePath());
    }
    else
	return(QString::null);
}

QString PlaylistSplitter::playFirstFile()
{
    stop();

    Playlist *p = visiblePlaylist();
    PlaylistItem *i = static_cast<PlaylistItem *>(p->firstChild());

    if(i) {
	i->setPixmap(0, QPixmap(UserIcon("playing")));
	i->setPixmap(0, QPixmap(UserIcon("playing")));
	playingItem = i;

	return(i->absFilePath());
    }
    else
	return(QString::null);
}

void PlaylistSplitter::add(const QString &file, Playlist *list)
{
    KApplication::setOverrideCursor(Qt::waitCursor);
    addImpl(file, list);
    KApplication::restoreOverrideCursor();
    
    if(editor)
	editor->updateCollection();
}

void PlaylistSplitter::add(const QStringList &files, Playlist *list)
{
    KApplication::setOverrideCursor(Qt::waitCursor);
    for(QStringList::ConstIterator it = files.begin(); it != files.end(); ++it)
        addImpl(*it, list);
    KApplication::restoreOverrideCursor();

    if(editor)
	editor->updateCollection();
}

QString PlaylistSplitter::extensionsString(const QStringList &extensions, const QString &type) // static
{
    QStringList l;

    for(QStringList::ConstIterator it = extensions.begin(); it != extensions.end(); ++it)
	l.append(QString("*." + (*it)));

    // i.e. "*.m3u, *.mp3|Media Files"

    QString s = l.join(" ");

    if(type != QString::null)
	s += "|" + type + " (" + l.join(", ") + ")";

    return(s);
}

/*
void PlaylistSplitter::setSelected(PlaylistItem *i) // static
{
    // Hu hu!  See how much crap I can pack into just one pointer to a 
    // PlaylistItem!  Sensitive viewers may want to close their eyes for the
    // next few lines.
    
    if(i) {
	
	// Get the playlist associated with the playing item and make set the
	// playing item to be both selected and visible.
	
	Playlist *l = dynamic_cast<Playlist *>(i->listView());
	
	if(l) {
	    l->clearSelection();
	    l->setSelected(i, true);
	    l->ensureItemVisible(i);
	    
	    // Now move on to the PlaylistBox.  The Playlist knows which
	    // PlaylistBoxItem that it is associated with, so we'll just get
	    // that and then figure out the PlaylistBox from there.
	    // 
	    // Once we have that we can set the appropriate Playlist to be
	    // visible.
	    
	    if(l->playlistBoxItem() && l->playlistBoxItem()->listBox()) {
		QListBox *b = l->playlistBoxItem()->listBox();
		
		b->clearSelection();
		b->setSelected(l->playlistBoxItem(), true);

		b->setCurrentItem(l->playlistBoxItem());
		b->ensureCurrentVisible();
	    }
	}
    }    
}
*/

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void PlaylistSplitter::open()
{
//    QStringList files = KFileDialog::getOpenFileNames(QString::null, 
//						      extensionsString((mediaExtensions + listExtensions), i18n("Media Files")));
    QStringList files = KFileDialog::getOpenFileNames();
    open(files);
}

void PlaylistSplitter::setEditorVisible(bool visible)
{
    if(visible)
	editor->show();
    else
	editor->hide();
}

Playlist *PlaylistSplitter::createPlaylist()
{
    bool ok;

    // If this text is changed, please also change it in PlaylistBox::duplicate().

    QString name = QInputDialog::getText(i18n("New Playlist..."), i18n("Please enter a name for the new playlist:"),
					 QLineEdit::Normal, uniquePlaylistName(), &ok);
    if(ok)
	return(createPlaylist(name));
    else
	return(0);
}

Playlist *PlaylistSplitter::createPlaylist(const QString &name)
{
    Playlist *p = new Playlist(this, playlistStack, name.latin1());
    setupPlaylist(p, true);
    return(p);
}

void PlaylistSplitter::openPlaylist()
{
    QStringList files = KFileDialog::getOpenFileNames(QString::null, "*.m3u");

    for(QStringList::ConstIterator it = files.begin(); it != files.end(); ++it)
	openPlaylist(*it);
}

Playlist *PlaylistSplitter::openPlaylist(const QString &playlistFile)
{
    QFileInfo file(playlistFile);
    if(!file.exists() || !file.isFile() || !file.isReadable() || playlistFiles.contains(file.absFilePath()))
	return(0);

    playlistFiles.append(file.absFilePath());
    
    Playlist *p = new Playlist(this, playlistFile, playlistStack, file.baseName(true).latin1());
    setupPlaylist(p);
    return(p);
}

void PlaylistSplitter::selectPlaying()
{
    if(!playingItem)
	return;

    Playlist *l = static_cast<Playlist *>(playingItem->listView());
	
    if(l) {

	l->clearSelection();
	l->setSelected(playingItem, true);
	l->ensureItemVisible(playingItem);
	
	// Now move on to the PlaylistBox.  The Playlist knows which
	// PlaylistBoxItem that it is associated with, so we'll just get
	// that and then figure out the PlaylistBox from there.
	// 
	// Once we have that we can set the appropriate Playlist to be
	// visible.
	
	if(l->playlistBoxItem() && l->playlistBoxItem()->listBox()) {
	    QListBox *b = l->playlistBoxItem()->listBox();
	    
	    b->clearSelection();
	    b->setSelected(l->playlistBoxItem(), true);
	    
	    b->setCurrentItem(l->playlistBoxItem());
	    b->ensureCurrentVisible();
	}    
    }
}

QString PlaylistSplitter::playingArtist() const
{
    if(playingItem)
	return(playingItem->text(PlaylistItem::ArtistColumn));
    else
	return(QString::null);
}

QString PlaylistSplitter::playingTrack() const
{
    if(playingItem)
	return(playingItem->text(PlaylistItem::TrackColumn));
    else
	return(QString::null);
}

QString PlaylistSplitter::playingList() const
{
    if(playingItem)
	return(static_cast<Playlist *>(playingItem->listView())->name());
    else
	return(QString::null);
}

void PlaylistSplitter::stop()
{
    if(playingItem) {
	playingItem->setPixmap(0, 0);
	playingItem = 0;
    }
}


void PlaylistSplitter::removeSelectedItems()
{
    PlaylistItemList items = playlistSelection();

    checkPlayingItemBeforeRemove(items);

    Playlist *p = visiblePlaylist();
    if(p)
	p->remove(items);
}

void PlaylistSplitter::clearSelectedItems()
{
    PlaylistItemList items = playlistSelection();
    checkPlayingItemBeforeRemove(items);
 
    Playlist *p = visiblePlaylist();
    if(p)
	p->clearItems(items); 
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void PlaylistSplitter::setupLayout()
{
    playlistBox = new PlaylistBox(this, "playlistBox");

    // Create a splitter to go between the playlists and the editor.

    QSplitter *editorSplitter = new QSplitter(Qt::Vertical, this, "editorSplitter");

    // Create the playlist and the editor.

    playlistStack = new QWidgetStack(editorSplitter, "playlistStack");
    editor = new TagEditor(editorSplitter, "tagEditor");

    // Make the editor as small as possible (or at least as small as recommended)

    editorSplitter->setResizeMode(editor, QSplitter::FollowSizeHint);

    // Make the connection that will update the selected playlist when a 
    // selection is made in the playlist box.

    connect(playlistBox, SIGNAL(currentChanged(PlaylistBoxItem *)), 
	    this, SLOT(changePlaylist(PlaylistBoxItem *)));

    connect(playlistBox, SIGNAL(doubleClicked()), this, SIGNAL(listBoxDoubleClicked()));

    // Create the collection list; this should always exist.  This has a 
    // slightly different creation process than normal playlists (since it in
    // fact is a subclass) so it is created here rather than by using 
    // createPlaylist().

    CollectionList::initialize(this, playlistStack, restore);
    collection = CollectionList::instance();
    setupPlaylist(collection, true, "folder_sound");

    // Show the collection on startup.
    playlistBox->setSelected(0, true);
}

void PlaylistSplitter::readConfig()
{
    KConfig *config = KGlobal::config();
    { // block for Playlists group
	KConfigGroupSaver saver(config, "Playlists");

	if(restore) {
	    QStringList external = config->readListEntry("ExternalPlaylists");
	    for(QStringList::Iterator it = external.begin(); it != external.end(); ++it)
		openPlaylist(*it);
	    
	    QStringList internal = config->readListEntry("InternalPlaylists");
	    for(QStringList::Iterator it = internal.begin(); it != internal.end(); ++it) {
		Playlist *p = openPlaylist(*it);
		if(p)
		    p->setInternal(true);
	    }
	}
    }
}	


void PlaylistSplitter::saveConfig()
{
    KConfig *config = KGlobal::config();

    // Save the list of open playlists.
    
    if(restore && playlistBox) {
	QStringList internalPlaylists;
	QStringList externalPlaylists;

	// Start at item 1.  We want to skip the collection list.

	for(uint i = 1; i < playlistBox->count(); i++) {
	    PlaylistBoxItem *item = static_cast<PlaylistBoxItem *>(playlistBox->item(i));
	    if(item && item->playlist()) {
		Playlist *p = item->playlist();
		if(p->isInternalFile()) {
		    p->save(true);
		    internalPlaylists.append(p->fileName());
		}
		else
		    externalPlaylists.append(p->fileName());
	    }		
	}
	{ // block for Playlists group
	    KConfigGroupSaver saver(config, "Playlists");
	    config->writeEntry("InternalPlaylists", internalPlaylists);
	    config->writeEntry("ExternalPlaylists", externalPlaylists);
	}
    }
}

void PlaylistSplitter::addImpl(const QString &file, Playlist *list)
{
    processEvents();
    QFileInfo fileInfo(QDir::cleanDirPath(file));
    if(fileInfo.exists()) {
        if(fileInfo.isDir()) {
            QDir dir(fileInfo.filePath());
            QStringList dirContents=dir.entryList();
            for(QStringList::Iterator it = dirContents.begin(); it != dirContents.end(); ++it)
                if(*it != "." && *it != "..")
                    addImpl(fileInfo.filePath() + QDir::separator() + *it, list);
        }
        else {
            QString extension = fileInfo.extension(false);
            if(mediaExtensions.contains(extension) > 0)
		list->createItem(fileInfo);
	    else if(listExtensions.contains(extension) > 0)
		openPlaylist(fileInfo.absFilePath());
        }
    }    
}

void PlaylistSplitter::setupPlaylist(Playlist *p, bool raise, const char *icon)
{
    PlaylistBoxItem *i = new PlaylistBoxItem(playlistBox, SmallIcon(icon, 32), p->name(), p);
    p->setPlaylistBoxItem(i);
    playlistBox->sort();

    connect(p, SIGNAL(selectionChanged(const PlaylistItemList &)), editor, SLOT(setItems(const PlaylistItemList &)));
    connect(p, SIGNAL(doubleClicked()), this, SIGNAL(doubleClicked()));
    connect(p, SIGNAL(collectionChanged()), editor, SLOT(updateCollection()));
    connect(p, SIGNAL(numberOfItemsChanged(Playlist *)), this, SLOT(playlistCountChanged(Playlist *)));

    if(raise) {
	playlistStack->raiseWidget(p);
	playlistBox->setCurrentItem(i);
	playlistBox->ensureCurrentVisible();
    }
}

void PlaylistSplitter::checkPlayingItemBeforeRemove(PlaylistItemList &items)
{
    PlaylistItem *item = items.first();
    while(item) {
	if(item == playingItem)
	    playingItem = 0;
	item = items.next();
    }
}

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void PlaylistSplitter::changePlaylist(PlaylistBoxItem *item)
{
    if(item && item->playlist()) {
	playlistStack->raiseWidget(item->playlist());
	editor->setItems(playlistSelection());
	emit(playlistChanged());
    }
}

void PlaylistSplitter::playlistCountChanged(Playlist *p)
{
    if(p && p == playlistStack->visibleWidget())
	emit(selectedPlaylistCountChanged(p->childCount()));
}

#include "playlistsplitter.moc"
