/***************************************************************************
                          systray.cpp  -  description
                             -------------------
    copyright            : (C) 2002 by Daniel Molkentin 
    email                : molkentin@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <kapplication.h>
#include <kiconloader.h>
#include <kpopupmenu.h>
#include <klocale.h>
#include <kpassivepopup.h>
#include <kaction.h>
#include <kmainwindow.h>
#include <qhbox.h>
#include <qpushbutton.h>
#include <qlabel.h>

#include <qtimer.h>
#include <qtooltip.h>

#include "systemtray.h"

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

SystemTray::SystemTray(KMainWindow *parent, const char *name) : KSystemTray(parent, name),
								m_popup(0)

{
    m_appPix     = SmallIcon("juk");
    m_playPix    = SmallIcon("player_play");
    m_pausePix   = SmallIcon("player_pause");
    m_backPix    = SmallIcon("player_start");
    m_forwardPix = SmallIcon("player_end");    

    setPixmap(m_appPix);

    KPopupMenu *cm = contextMenu();

    m_actionCollection = parent->actionCollection();

    m_playAction    = m_actionCollection->action("play");
    m_pauseAction   = m_actionCollection->action("pause");
    m_stopAction    = m_actionCollection->action("stop");
    m_backAction    = m_actionCollection->action("back");
    m_forwardAction = m_actionCollection->action("forward");

    m_togglePopupsAction = static_cast<KToggleAction *>(m_actionCollection->action("togglePopups"));

    m_playAction->plug(cm);
    m_pauseAction->plug(cm);
    m_stopAction->plug(cm);
    m_backAction->plug(cm);
    m_forwardAction->plug(cm);

    cm->insertSeparator();

    m_togglePopupsAction->plug(cm);
}

SystemTray::~SystemTray()
{

}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void SystemTray::slotNewSong(const QString& songName)
{
    QToolTip::remove(this);
    QToolTip::add(this, songName);

    createPopup(songName, true);
}

void SystemTray::slotStop()
{
    setPixmap(m_appPix);
    QToolTip::remove(this);

    delete m_popup;
    m_popup = 0;
}

////////////////////////////////////////////////////////////////////////////////
// private methods
////////////////////////////////////////////////////////////////////////////////

void SystemTray::createPopup(const QString &songName, bool addButtons) 
{
    // If the action exists and it's checked, do our stuff
    if(m_togglePopupsAction && m_togglePopupsAction->isChecked()) {

	delete m_popup;
        m_popup = new KPassivePopup(this);

        QHBox *box = new QHBox(m_popup);

        // The buttons are now optional - no real reason, but it might be useful later
        if(addButtons) {
            QPushButton *backButton = new QPushButton(m_backPix, 0, box, "popup_back");
            backButton->setFlat(true);
            connect(backButton, SIGNAL(clicked()), m_backAction, SLOT(activate()));
        }

        QLabel *l = new QLabel(songName, box);
	l->setMargin(3);

        if(addButtons) {
            QPushButton *forwardButton = new QPushButton (m_forwardPix, 0, box, "popup_forward");
            forwardButton->setFlat(true);
            connect(forwardButton, SIGNAL(clicked()), m_forwardAction, SLOT(activate()));
        }

        m_popup->setView(box);

        // We don't want an autodelete popup.  There are times when it will need to be hidden before the timeout.
        m_popup->setAutoDelete(false);
        m_popup->show();
    }
}

#include "systemtray.moc"
