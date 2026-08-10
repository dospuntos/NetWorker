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
    explicit CollectionItem(const RequestData& data, const BString& customLabel = "");
    explicit CollectionItem(const BMessage& archive);

    status_t Archive(BMessage& archive) const;

    void SetCustomLabel(const BString& label);

    RequestData fData;
    BString     fCustomLabel;

private:
	void _RefreshText();
	static BString _BuildLabel(const BString& method, const BString& url);
};

#endif // COLLECTION_ITEM_H
