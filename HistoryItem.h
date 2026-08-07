/*
 * Copyright 2026, Johan Wagenheim <johan@dospuntos.no>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef HISTORY_ITEM_H
#define HISTORY_ITEM_H

#include <ListItem.h>
#include <String.h>

#include "RequestData.h"

class HistoryItem : public BStringItem {
public:
    explicit HistoryItem(const RequestData& data);
    explicit HistoryItem(const BMessage& archive);

    status_t Archive(BMessage& archive) const;
    bool Equals(const HistoryItem& other) const;

    void SetCustomLabel(const BString& label);

    RequestData fData;
    BString     fCustomLabel;

private:
    void _RefreshText();
    static BString _BuildLabel(const BString& method, const BString& url);
};

#endif // HISTORY_ITEM_H