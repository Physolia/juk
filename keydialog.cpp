/***************************************************************************
    begin                : Tue Mar 11 19:00:00 CET 2003
    copyright            : (C) 2003 by Stefan Asserhall
    email                : stefan.asserhall@telia.com
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "keydialog.h"
#include "actioncollection.h"

#include <kconfig.h>
#include <klocale.h>
#include <kaction.h>

#include <QRadioButton>
#include <QGroupBox>
#include <q3vbox.h>



// Table of shortcut keys for each action, key group and three or four button modifier

const KeyDialog::KeyInfo KeyDialog::keyInfo[] = {
    { "playPause",
      { KShortcut::null(),
        Qt::CTRL+Qt::ALT+Qt::Key_P,
        Qt::Key_MediaPlay } },
    { "stop",
      { KShortcut::null(),
        Qt::CTRL+Qt::ALT+Qt::Key_S,
        Qt::Key_MediaStop } },
    { "back",
      { KShortcut::null(),
        Qt::CTRL+Qt::ALT+Qt::Key_Left,
        Qt::Key_MediaPrevious } },
    { "forward",
      { KShortcut::null(),
        Qt::CTRL+Qt::ALT+Qt::Key_Right,
        Qt::Key_MediaNext } },
    { "forwardAlbum",
      { KShortcut::null(),
        Qt::CTRL+Qt::ALT+Qt::Key_Up,
        Qt::CTRL+Qt::Key_MediaNext } },
    { "seekBack",
      { KShortcut::null(),
        Qt::CTRL+Qt::SHIFT+Qt::ALT+Qt::Key_Left,
        Qt::SHIFT+Qt::Key_MediaPrevious } },
    { "seekForward",
      { KShortcut::null(),
        Qt::CTRL+Qt::SHIFT+Qt::ALT+Qt::Key_Right,
        Qt::SHIFT+Qt::Key_MediaNext } },
    { "volumeUp",
      { KShortcut::null(),
        Qt::CTRL+Qt::ALT+Qt::SHIFT+Qt::Key_Up,
        Qt::Key_VolumeUp } },
    { "volumeDown",
      { KShortcut::null(),
        Qt::CTRL+Qt::ALT+Qt::SHIFT+Qt::Key_Down,
        Qt::Key_VolumeDown } },
    { "mute",
      { KShortcut::null(),
        Qt::CTRL+Qt::ALT+Qt::Key_M,
        Qt::Key_VolumeMute } },
    { "showHide",
      { KShortcut::null(),
        KShortcut::null(),
        KShortcut::null() } }
};

const uint KeyDialog::keyInfoCount = sizeof(KeyDialog::keyInfo) / sizeof(KeyDialog::keyInfo[0]);

KeyDialog::KeyDialog(KActionCollection *actionCollection, QWidget *parent)
    : KDialog(parent)
{
    setCaption(i18n("Configure Shortcuts"));
    setButtons(Default | Ok | Cancel);

    // Read key group from configuration

    KConfigGroup config(KGlobal::config(), "Shortcuts");
    int selectedButton = config.readEntry("GlobalKeys", int(StandardKeys));

    // Create widgets for key chooser - widget stack used to replace key chooser

    Q3VBox *vbox = new Q3VBox(this);
    vbox->setSpacing(KDialog::spacingHint());
    m_widgetStack = new Q3WidgetStack(vbox);

    vbox->setStretchFactor(m_widgetStack, 1);

    // Create buttons to select key group

    m_group = new QButtonGroup(vbox);
    QGroupBox *buttonBox = new QGroupBox(i18n("Global Shortcuts"), vbox);

    m_group->addButton(new QRadioButton(i18n("&No keys"), buttonBox), NoKeys);
    m_group->addButton(new QRadioButton(i18n("&Standard keys"), buttonBox), StandardKeys);
    m_group->addButton(new QRadioButton(i18n("&Multimedia keys"), buttonBox), MultimediaKeys);

    connect(m_group, SIGNAL(buttonClicked(int)), this, SLOT(slotKeys(int)));
    buttonBox->setWhatsThis(
	i18n("Here you can select the keys used as global shortcuts to control the player"));

    // Create the key chooser

    setMainWidget(vbox);
    newDialog(actionCollection, selectedButton);
}

KeyDialog::~KeyDialog()
{

}

void KeyDialog::newDialog(KActionCollection *actionCollection, int selectedButton)
{
    m_actionCollection = actionCollection;

    // Create key chooser and show it in the widget stack
    m_pKeyChooser = new KKeyChooser(actionCollection, this);
    m_widgetStack->addWidget(m_pKeyChooser);
    m_widgetStack->raiseWidget(m_pKeyChooser);

    m_group->button(selectedButton)->setChecked(true);

    connect(this, SIGNAL(defaultClicked()), this, SLOT(slotDefault()));
}

int KeyDialog::configure()
{
    // Show the dialog and save configuration if accepted

    int retcode = exec();
    if(retcode == Accepted) {
        KConfigGroup config(KGlobal::config(), "Shortcuts");

        config.writeEntry("GlobalKeys", m_group->checkedId());
        KGlobal::config()->sync();

        m_pKeyChooser->save();
    }
    return retcode;
}

void KeyDialog::slotKeys(int group)
{
    // Set modifier keys according to key group and modifier keys

    for(uint i = 0; i < keyInfoCount; i++) {
        KAction *a = ActionCollection::action(keyInfo[i].action);
        if(a)
            a->setGlobalShortcut(keyInfo[i].shortcut[group]);
    }

    // Create a new key chooser to show the keys, and delete the old one

    QWidget *w = m_widgetStack->visibleWidget();
    newDialog(m_actionCollection, group);
    m_widgetStack->removeWidget(w);
    delete w;
}

void KeyDialog::slotDefault()
{
    // Select default keys - standard key group

    m_group->button(StandardKeys)->setChecked(true);
    m_pKeyChooser->allDefault();
}

int KeyDialog::configure(KActionCollection *actionCollection, QWidget *parent)
{
    // Create and show dialog - update connections if accepted

    return KeyDialog(actionCollection, parent).configure();
}

void KeyDialog::setupActionShortcut(const QString &actionName)
{
    // Find and insert a standard key
    KShortcut shortcut = KShortcut::null();

    for(uint i = 0; i < keyInfoCount; i++) {
        if(keyInfo[i].action == actionName) {
            shortcut = keyInfo[i].shortcut[StandardKeys];
            break;
        }
    }

    if(shortcut.isNull())
        return; // We have no shortcut to set.

    KAction *a = ActionCollection::action(actionName);
    if(a)
        a->setGlobalShortcut(shortcut);
}

#include "keydialog.moc"

// vim: set et sw=4 tw=0 sta:
