/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "HistoryItem.h"

BString
HistoryItem::_BuildLabel(const BString& method, const BString& url)
{
    BString label;
    label << method << " " << url;
    return label;
}


HistoryItem::HistoryItem(const RequestData& data)
    :
    BStringItem(""),
    fData(data)
{
    _RefreshText();
}


HistoryItem::HistoryItem(const BMessage& archive)
    :
    BStringItem("")
{
    BMessage dataArchive;
    if (archive.FindMessage("data", &dataArchive) == B_OK)
        fData = RequestData(dataArchive);

    archive.FindString("customLabel", &fCustomLabel);
    _RefreshText();
}

status_t
HistoryItem::Archive(BMessage& archive) const
{
    BMessage dataArchive;
    fData.Archive(dataArchive);
    archive.AddMessage("data", &dataArchive);
    archive.AddString("customLabel", fCustomLabel);
    return B_OK;
}


bool
HistoryItem::Equals(const HistoryItem& other) const
{
    return fData.Equals(other.fData);
}


void
HistoryItem::_RefreshText()
{
    if (fCustomLabel.Length() > 0)
        SetText(fCustomLabel.String());
    else
        SetText(_BuildLabel(fData.fMethod, fData.fUrl).String());
}


void
HistoryItem::SetCustomLabel(const BString& label)
{
	fCustomLabel = label;
	_RefreshText();
}
