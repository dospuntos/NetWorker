/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "RequestData.h"

#include <DataIO.h>
#include <cstring>

namespace {

bool
MessagesEqual(const BMessage& a, const BMessage& b)
{
    BMallocIO bufA, bufB;
    if (a.Flatten(&bufA) != B_OK || b.Flatten(&bufB) != B_OK)
        return false;

    if (bufA.BufferLength() != bufB.BufferLength())
        return false;

    return memcmp(bufA.Buffer(), bufB.Buffer(), bufA.BufferLength()) == 0;
}

} // namespace

RequestData::RequestData()
    :
    fAuthType("none")
{
}


RequestData::RequestData(const BString& method, const BString& url, const BString& body,
	const BMessage& params, const BString& bodyMode, const BString& filePath,
	const BString& authType, const BMessage& authValues)
	:
	fMethod(method),
	fUrl(url),
	fBody(body),
	fParams(params),
	fBodyMode(bodyMode),
	fFilePath(filePath),
	fAuthType(authType),
	fAuthValues(authValues)
{
}

RequestData::RequestData(const BMessage& archive)
    :
    fAuthType("none")
{
    archive.FindString("method", &fMethod);
    archive.FindString("url", &fUrl);
    archive.FindString("body", &fBody);

    BMessage params;
    if (archive.FindMessage("params", &params) == B_OK)
        fParams = params;

	BString bodyMode;
	if (archive.FindString("bodyMode", &bodyMode) == B_OK)
		fBodyMode = bodyMode;

	BString filePath;
	if (archive.FindString("filePath", &filePath) == B_OK)
		fFilePath = filePath;

	archive.FindString("authType", &fAuthType);

	BMessage authValues;
    if (archive.FindMessage("authValues", &authValues) == B_OK)
        fAuthValues = authValues;
}

status_t
RequestData::Archive(BMessage& archive) const
{
    archive.AddString("method", fMethod);
    archive.AddString("url", fUrl);
    archive.AddString("body", fBody);
    archive.AddMessage("params", &fParams);
	archive.AddString("bodyMode", fBodyMode);
	archive.AddString("filePath", fFilePath);
	archive.AddString("authType", fAuthType);
	archive.AddMessage("authValues", &fAuthValues);
    return B_OK;
}

bool
RequestData::Equals(const RequestData& other) const
{
	return fMethod == other.fMethod && fUrl == other.fUrl && fBody == other.fBody
		&& fFilePath == other.fFilePath && fAuthType == other.fAuthType
		&& MessagesEqual(fParams, other.fParams) && MessagesEqual(fAuthValues, other.fAuthValues);
}

