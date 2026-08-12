/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef COLLECTION_H
#define COLLECTION_H

#include <ObjectList.h>
#include <String.h>

#include "CollectionItem.h"

class Collection {
public:
    explicit Collection(const BString& name);
    explicit Collection(const BMessage& archive);
    ~Collection();

    status_t Archive(BMessage& archive) const;

    const BString& Name() const { return fName; }
    void SetName(const BString& name) { fName = name; }

	const BString& FileName() const { return fFileName; }
    void SetFileName(const BString& fileName) { fFileName = fileName; }

    int32 CountItems() const { return fItems.CountItems(); }
    CollectionItem* ItemAt(int32 index) const { return fItems.ItemAt(index); }
    void AddItem(CollectionItem* item) { fItems.AddItem(item); }
    CollectionItem* RemoveItem(int32 index); // detaches item, does not delete it
    void MoveItem(int32 from, int32 to);

private:
    BString fName;
	BString fFileName;
    BObjectList<CollectionItem, true> fItems;
};

#endif // COLLECTION_H