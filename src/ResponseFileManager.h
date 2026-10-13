/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef RESPONSE_FILE_MANAGER_H
#define RESPONSE_FILE_MANAGER_H

#include <Entry.h>
#include <String.h>
#include <SupportDefs.h>

#include <stddef.h>

class ResponseFileManager {
public:
    static status_t CreateFile(const void* data, size_t size,
        const BString& contentType, entry_ref& ref);

    static status_t CreateTextFile(const void* data, size_t size,
        entry_ref& ref);

private:
    static status_t _CreateFile(const void* data, size_t size,
        const BString& mimeType, const char* extension, entry_ref& ref);

    static status_t _GetTempDirectory(BString& path);

    static BString _NormalizeMimeType(const BString& contentType);
    static const char* _ExtensionForMimeType(const BString& mimeType);
};

#endif // RESPONSE_FILE_MANAGER_H
