/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef COLLECTION_ITEM_H
#define COLLECTION_ITEM_H

#include <ListItem.h>
#include <String.h>

#include "RequestData.h"

class CollectionItem : public BStringItem {
public:
    CollectionItem(const BString& label, const RequestData& data);
    explicit CollectionItem(const BMessage& archive);

    status_t Archive(BMessage& archive) const;

    void SetLabel(const BString& label);

    RequestData fData;
    BString     fLabel;
};

#endif // COLLECTION_ITEM_H
