/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "CollectionItem.h"


BString
CollectionItem::_BuildLabel(const BString& method, const BString& url)
{
	BString label;
	label << method << " " << url;
	return label;
}


CollectionItem::CollectionItem(const RequestData& data, const BString& customLabel)
	:
	BStringItem(""),
	fData(data),
	fCustomLabel(customLabel)
{
	_RefreshText();
}

CollectionItem::CollectionItem(const BMessage& archive)
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
CollectionItem::Archive(BMessage& archive) const
{
	BMessage dataArchive;
    fData.Archive(dataArchive);
    archive.AddMessage("data", &dataArchive);
	archive.AddString("customLabel", fCustomLabel);
	return B_OK;
}


void
CollectionItem::SetCustomLabel(const BString& label)
{
	fCustomLabel = label;
	_RefreshText();
}


void
CollectionItem::_RefreshText()
{
	if (fCustomLabel.Length() > 0)
		SetText(fCustomLabel.String());
	else
		SetText(_BuildLabel(fData.fMethod, fData.fUrl).String());
}