/***************************************************************************
                          oggtag.cpp  -  description
                             -------------------
    begin                : Sat Oct 5 2002
    copyright            : (C) 2002 by Scott Wheeler
    email                : scott@slackorama.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <kdebug.h>

#include <qdatetime.h>
#include <qvariant.h>

#include "oggtag.h"
#include "genrelistlist.h"

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

OggTag::OggTag(const QString &file) : Tag(file)
{
    metaInfo = KFileMetaInfo(file);
    commentGroup = KFileMetaInfoGroup(metaInfo.group("Comment"));
}

OggTag::~OggTag()
{

}

void OggTag::save()
{
    metaInfo.applyChanges();
}

QString OggTag::track() const
{
    return readCommentString("Title");
}

QString OggTag::artist() const
{
    return readCommentString("Artist");
}

QString OggTag::album() const
{
    return readCommentString("Album");
}

Genre OggTag::genre() const
{
    QString genreName = readCommentString("Genre");
    int index = GenreListList::ID3v1List()->findIndex(genreName);
    return Genre(genreName, index);
}

int OggTag::trackNumber() const
{
    return readCommentInt("Tracknumber");
}

QString OggTag::trackNumberString() const
{
    return readCommentString("Tracknumber");
}

int OggTag::year() const
{
    QDateTime d = QDateTime::fromString(readCommentString("Date"), Qt::ISODate);

    if(d.isValid())
	return(d.date().year());
    else
	return(0);
}

QString OggTag::yearString() const
{
    QDateTime d = QDate::fromString(readCommentString("Date"), Qt::ISODate);
    if(d.isValid())
	return(QString::number(d.date().year()));
    else
	return(QString::null);
}

QString OggTag::comment() const
{
    return readCommentString("Description");
}

bool OggTag::hasTag() const
{
    if(metaInfo.isValid() && !metaInfo.isEmpty())
	return(true);
    else
	return(false);
}

void OggTag::setTrack(const QString &value)
{
    writeCommentItem("Title", value);
}

void OggTag::setArtist(const QString &value)
{
    writeCommentItem("Artist", value);
}

void OggTag::setAlbum(const QString &value)
{
    writeCommentItem("Album", value);
}

void OggTag::setGenre(const Genre &value)
{
    writeCommentItem("Genre", value);
}

void OggTag::setTrackNumber(int value)
{
    writeCommentItem("Tracknumber", value);
}

void OggTag::setYear(int value)
{
    QDate d = QDate::fromString(readCommentString("Date"), Qt::ISODate);
    if(d.setYMD(value, d.month(), d.day())) {
	QDateTime dt = d;
	writeCommentItem("Date", dt.toString(Qt::ISODate));
    }
}

void OggTag::setComment(const QString &value)
{
    writeCommentItem("Description", value);
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

QString OggTag::readCommentString(const QString &key) const
{
    if(metaInfo.isValid() && !metaInfo.isEmpty() &&
       commentGroup.isValid() && !commentGroup.isEmpty() &&
       commentGroup.contains(key))
	// I'm throwing in the stripWhiteSpace() here, because the IOSlave/KFMI
	// stuff seems to be padding fields arbitrarily.
	return(commentGroup.item(key).string().stripWhiteSpace());
    else
	return(QString::null);
}

int OggTag::readCommentInt(const QString &key) const
{
    if(metaInfo.isValid() && !metaInfo.isEmpty() &&
       commentGroup.isValid() && !commentGroup.isEmpty() &&
       commentGroup.contains(key)) {
	bool ok;
	int value = commentGroup.item(key).value().toInt(&ok);
	if(ok)
	    return(value);
	else
	    return(-1);
    }
    else
	return(-1);
}

void OggTag::writeCommentItem(const QString &key, const QString &value)
{
    if(metaInfo.isValid() && commentGroup.isValid()) {
	QVariant v(value);
	if(!commentGroup.contains(key))
	    commentGroup.addItem(key);
	commentGroup.item(key).setValue(v);
    }
}

void OggTag::writeCommentItem(const QString &key, int value)
{
    if(metaInfo.isValid() && commentGroup.isValid()) {
	QVariant v(QString::number(value));
	if(!commentGroup.contains(key))
	    commentGroup.addItem(key);
	commentGroup.item(key).setValue(v);
    }
}
