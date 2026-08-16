/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef REQUEST_DATA_H
#define REQUEST_DATA_H

#include <Message.h>
#include <String.h>

class RequestData {
public:
    RequestData();
    RequestData(const BString& method, const BString& url, const BMessage& customHeaders, const BString& body,
        const BMessage& params, const BMessage& queryParams, const BString& bodyMode, const BString& filePath, const BString& authType, const BMessage& authValues);
    explicit RequestData(const BMessage& archive);

    status_t Archive(BMessage& archive) const;
    bool Equals(const RequestData& other) const;

    BString  fMethod;
    BString  fUrl;
	BMessage fCustomHeaders;
    BString  fBody;
    BMessage fParams;
	BMessage fQueryParams;
	BString  fBodyMode; // none, raw, form, file
	BString  fFilePath;
    BString  fAuthType; // none, basic, bearer, apikey
    BMessage fAuthValues;
};

#endif // REQUEST_DATA_H
