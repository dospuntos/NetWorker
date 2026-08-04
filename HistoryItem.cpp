/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "HistoryItem.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MainView"

BString
HistoryItem::_BuildLabel(const BString& method, const BString& url)
{
    BString label;
    label << method << " " << url;
    return label;
}


HistoryItem::HistoryItem(const BString& method, const BString& url, const BString& body,
	const BMessage& params, const BString& authType, const BMessage& authValues)
	:
	BStringItem(_BuildLabel(method, url)),
	fMethod(method),
	fUrl(url),
	fBody(body),
	fParams(params),
	fAuthType(authType),
	fAuthValues(authValues)
{
}

HistoryItem::HistoryItem(const BMessage& archive)
    :
    BStringItem("")
{
    archive.FindString("method", &fMethod);
    archive.FindString("url", &fUrl);
    archive.FindString("body", &fBody);

    BMessage params;
    if (archive.FindMessage("params", &params) == B_OK)
        fParams = params;

	if (archive.FindString("authType", &fAuthType) != B_OK)
		fAuthType = "none";

	BMessage authValues;
	if (archive.FindMessage("authValues", &authValues) == B_OK)
		fAuthValues = authValues;

	SetText(_BuildLabel(fMethod, fUrl));
}

status_t
HistoryItem::Archive(BMessage& archive) const
{
    archive.AddString("method", fMethod);
    archive.AddString("url", fUrl);
    archive.AddString("body", fBody);
    archive.AddMessage("params", &fParams);
	archive.AddString("authType", fAuthType);
	archive.AddMessage("authValues", &fAuthValues);
	return B_OK;
}
