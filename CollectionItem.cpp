/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "CollectionItem.h"

CollectionItem::CollectionItem(const BString& label, const RequestData& data)
    :
    BStringItem(label.String()),
    fData(data),
    fLabel(label)
{
}

CollectionItem::CollectionItem(const BMessage& archive)
    :
    BStringItem("")
{
    archive.FindString("label", &fLabel);

    BMessage dataArchive;
    if (archive.FindMessage("data", &dataArchive) == B_OK)
        fData = RequestData(dataArchive);

    SetText(fLabel.String());
}

status_t
CollectionItem::Archive(BMessage& archive) const
{
    archive.AddString("label", fLabel);

    BMessage dataArchive;
    fData.Archive(dataArchive);
    archive.AddMessage("data", &dataArchive);
    return B_OK;
}

void
CollectionItem::SetLabel(const BString& label)
{
    fLabel = label;
    SetText(label.String());
}