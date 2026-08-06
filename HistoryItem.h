/*
 * Copyright 2026, Johan Wagenheim <johan@dospuntos.no>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef HISTORYITEM_H
#define HISTORYITEM_H

#include <Message.h>
#include <String.h>
#include <StringItem.h>
#include <SupportDefs.h>

class HistoryItem : public BStringItem {
public:
    HistoryItem(const BString& method, const BString& url,
                const BString& body, const BMessage& params,
                const BString& authType, const BMessage& authValues);
    explicit HistoryItem(const BMessage& archive);

    status_t Archive(BMessage& archive) const;
	bool Equals(const HistoryItem& other) const;
	void SetCustomLabel(const BString& label);

    BString  fMethod;
    BString  fUrl;
    BString  fBody;
    BMessage fParams;
    BString  fAuthType;
    BMessage fAuthValues;
	BString fCustomLabel;

private:
    static BString _BuildLabel(const BString& method, const BString& url);
	void _RefreshText();
};

#endif // HISTORYITEM_H
