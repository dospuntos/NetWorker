/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "Collection.h"

#include <OS.h>


Collection::Collection(const BString& name)
    :
    fName(name),
    fItems(20)
{
	fFileName << "collection_" << system_time();
}

Collection::Collection(const BMessage& archive)
    :
    fItems(20)
{
    archive.FindString("name", &fName);

    BMessage itemArchive;
    for (int32 i = 0; archive.FindMessage("item", i, &itemArchive) == B_OK; ++i) {
        fItems.AddItem(new CollectionItem(itemArchive));
        itemArchive.MakeEmpty();
    }
}

Collection::~Collection()
{
}

status_t
Collection::Archive(BMessage& archive) const
{
    archive.AddString("name", fName);

    for (int32 i = 0; i < fItems.CountItems(); ++i) {
        BMessage itemArchive;
        fItems.ItemAt(i)->Archive(itemArchive);
        archive.AddMessage("item", &itemArchive);
    }

    return B_OK;
}

CollectionItem*
Collection::RemoveItem(int32 index)
{
    return fItems.RemoveItemAt(index);
}

void
Collection::MoveItem(int32 from, int32 to)
{
    fItems.MoveItem(from, to);
}