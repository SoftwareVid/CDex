/***************************************************************************
    copyright            : (C) 2002, 2003 by Scott Wheeler
    email                : wheeler@kde.org
 ***************************************************************************/

/***************************************************************************
 *   This library is free software; you can redistribute it and/or modify  *
 *   it  under the terms of the GNU Lesser General Public License version  *
 *   2.1 as published by the Free Software Foundation.                     *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful, but   *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
 *   USA                                                                   *
 ***************************************************************************/

#include <tdebug.h>
#include <tfile.h>

#include "id3v1tag.h"
#include "id3v1genres.h"

using namespace TagLib;
using namespace ID3v1;

class ID3v1::Tag::TagPrivate
{
public:
  TagPrivate() : file(0), tagOffset(-1), track(0), genre(255) {}

  File *file;
  long tagOffset;

  String title;
  String artist;
  String album;
  String year;
  String comment;
  uchar track;
  uchar genre;

  static const StringHandler *stringHandler;
};

const ID3v1::StringHandler *ID3v1::Tag::TagPrivate::stringHandler = new StringHandler;

////////////////////////////////////////////////////////////////////////////////
// StringHandler implementation
////////////////////////////////////////////////////////////////////////////////

String ID3v1::StringHandler::parse(const ByteVector &data) const
{
  return String(data, String::Latin1);
}

ByteVector ID3v1::StringHandler::render(const String &s) const
{
  return s.data(String::Latin1);
}

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

ID3v1::Tag::Tag() : TagLib::Tag()
{
  d = new TagPrivate;
}

ID3v1::Tag::Tag(File *file, long tagOffset) : TagLib::Tag()
{
  d = new TagPrivate;
  d->file = file;
  d->tagOffset = tagOffset;

  read();
}

ID3v1::Tag::~Tag()
{
  delete d;
}

ByteVector ID3v1::Tag::render() const
{
  ByteVector data;

  data.append(fileIdentifier());
  data.append(TagPrivate::stringHandler->render(d->title).resize(30));
  data.append(TagPrivate::stringHandler->render(d->artist).resize(30));
  data.append(TagPrivate::stringHandler->render(d->album).resize(30));
  data.append(TagPrivate::stringHandler->render(d->year).resize(4));
  data.append(TagPrivate::stringHandler->render(d->comment).resize(28));
  data.append(char(0));
  data.append(char(d->track));
  data.append(char(d->genre));

  return data;
}

ByteVector ID3v1::Tag::fileIdentifier()
{
  return ByteVector::fromCString("TAG");
}

String ID3v1::Tag::title() const
{
  return d->title;
}

String ID3v1::Tag::artist() const
{
  return d->artist;
}

String ID3v1::Tag::album() const
{
  return d->album;
}

String ID3v1::Tag::comment() const
{
  return d->comment;
}

String ID3v1::Tag::genre() const
{
  return ID3v1::genre(d->genre);
}

TagLib::uint ID3v1::Tag::year() const
{
  return d->year.toInt();
}

TagLib::uint ID3v1::Tag::track() const
{
  return d->track;
}

void ID3v1::Tag::setTitle(const String &s)
{
  d->title = s;
}

void ID3v1::Tag::setArtist(const String &s)
{
  d->artist = s;
}

void ID3v1::Tag::setAlbum(const String &s)
{
  d->album = s;
}

void ID3v1::Tag::setComment(const String &s)
{
  d->comment = s;
}

void ID3v1::Tag::setGenre(const String &s)
{
  d->genre = ID3v1::genreIndex(s);
}

void ID3v1::Tag::setYear(uint i)
{
  d->year = String::number(i);
}

void ID3v1::Tag::setTrack(uint i)
{
  d->track = i < 256 ? i : 0;
}

void ID3v1::Tag::setStringHandler(const StringHandler *handler)
{
  delete TagPrivate::stringHandler;
  TagPrivate::stringHandler = handler;
}

////////////////////////////////////////////////////////////////////////////////
// protected methods
////////////////////////////////////////////////////////////////////////////////

void ID3v1::Tag::read()
{
  if(d->file && d->file->isValid()) {
    d->file->seek(d->tagOffset);
    // read the tag -- always 128 bytes
    ByteVector data = d->file->readBlock(128);

    // some initial sanity checking
    if(data.size() == 128 && data.mid(0, 3) == "TAG")
      parse(data);
    else
      debug("ID3v1 tag is not valid or could not be read at the specified offset.");
  }
}

void ID3v1::Tag::parse(const ByteVector &data)
{
  int offset = 3;

  d->title = TagPrivate::stringHandler->parse(data.mid(offset, 30));
  offset += 30;

  d->artist = TagPrivate::stringHandler->parse(data.mid(offset, 30));
  offset += 30;

  d->album = TagPrivate::stringHandler->parse(data.mid(offset, 30));
  offset += 30;

  d->year = TagPrivate::stringHandler->parse(data.mid(offset, 4));
  offset += 4;

  // Check for ID3v1.1 -- Note that ID3v1 *does not* support "track zero" -- this
  // is not a bug in TagLib.  Since a zeroed byte is what we would expect to
  // indicate the end of a C-String, specifically the comment string, a value of
  // zero must be assumed to be just that.

  if(data[offset + 28] == 0 && data[offset + 29] != 0) {
    // ID3v1.1 detected

    d->comment = TagPrivate::stringHandler->parse(data.mid(offset, 28));
    d->track = uchar(data[offset + 29]);
  }
  else
    d->comment = data.mid(offset, 30);

  offset += 30;

  d->genre = uchar(data[offset]);
}
