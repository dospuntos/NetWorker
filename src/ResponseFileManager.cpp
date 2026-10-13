/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "ResponseFileManager.h"

#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <NodeInfo.h>
#include <Path.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>


BString
ResponseFileManager::_NormalizeMimeType(const BString& contentType)
{
    BString mimeType(contentType);

    // Remove optional parameters, e.g. "; charset=utf-8"
    int32 separator = mimeType.FindFirst(';');
    if (separator >= 0)
        mimeType.Truncate(separator);

    mimeType.Trim();
    mimeType.ToLower();

    if (mimeType.IsEmpty())
        mimeType = "application/octet-stream";

    return mimeType;
}


const char*
ResponseFileManager::_ExtensionForMimeType(const BString& mimeType)
{
    if (mimeType == "text/html")
        return ".html";

    if (mimeType == "application/json"
        || mimeType == "text/json"
        || mimeType.EndsWith("+json"))
        return ".json";

    if (mimeType == "text/plain")
        return ".txt";

    if (mimeType == "text/css")
        return ".css";

    if (mimeType == "application/javascript"
        || mimeType == "text/javascript")
        return ".js";

    if (mimeType == "application/xml"
        || mimeType == "text/xml"
        || mimeType.EndsWith("+xml"))
        return ".xml";

    if (mimeType == "image/png")
        return ".png";

    if (mimeType == "image/jpeg")
        return ".jpg";

    if (mimeType == "image/gif")
        return ".gif";

    if (mimeType == "image/webp")
        return ".webp";

    if (mimeType == "image/svg+xml")
        return ".svg";

    if (mimeType == "application/pdf")
        return ".pdf";

    if (mimeType == "application/zip")
        return ".zip";

    return ".bin";
}


status_t
ResponseFileManager::_CreateFile(const void* data, size_t size,
    const BString& mimeType, const char* extension, entry_ref& ref)
{
    if (data == nullptr && size > 0)
        return B_BAD_VALUE;

    BString directory;
    status_t status = _GetTempDirectory(directory);

    if (status != B_OK)
        return status;

    BString filename(directory);
    filename << "/response-XXXXXX";

    char* tempName = strdup(filename.String());
    if (tempName == nullptr)
        return B_NO_MEMORY;

    int fd = mkstemp(tempName);
    if (fd < 0) {
        free(tempName);
        return B_ERROR;
    }

    // Append the extension
    BString finalPath(tempName);
    finalPath << extension;

    if (rename(tempName, finalPath.String()) != 0) {
        close(fd);
        unlink(tempName);
        free(tempName);
        return B_ERROR;
    }

    free(tempName);

    // Write the complete response, including binary data
    const uint8* bytes = static_cast<const uint8*>(data);
    size_t remaining = size;

    while (remaining > 0) {
        ssize_t written = write(fd, bytes, remaining);

        if (written < 0 && errno == EINTR)
            continue;

        if (written <= 0) {
            close(fd);
            unlink(finalPath.String());
            return B_ERROR;
        }

        bytes += written;
        remaining -= written;
    }

    if (close(fd) != 0) {
        unlink(finalPath.String());
        return B_ERROR;
    }

    // Set MIME type
    BFile file(finalPath.String(), B_READ_WRITE);
    status = file.InitCheck();

    if (status != B_OK) {
        unlink(finalPath.String());
        return status;
    }

    BNodeInfo nodeInfo(&file);
    status = nodeInfo.SetType(mimeType.String());

    if (status != B_OK) {
        unlink(finalPath.String());
        return status;
    }

    BEntry entry(finalPath.String());
    status = entry.GetRef(&ref);

    if (status != B_OK)
        unlink(finalPath.String());

    return status;
}


status_t
ResponseFileManager::_GetTempDirectory(BString& path)
{
    BPath tempPath;

    status_t status = find_directory(B_SYSTEM_TEMP_DIRECTORY, &tempPath);
    if (status != B_OK)
        return status;

    status = tempPath.Append("NetWorker");
    if (status != B_OK)
        return status;

    path = tempPath.Path();

    if (mkdir(path.String(), 0700) != 0 && errno != EEXIST)
        return errno;

    struct stat info;

    if (lstat(path.String(), &info) != 0)
        return errno;

    if (!S_ISDIR(info.st_mode))
        return B_NOT_A_DIRECTORY;

    return B_OK;
}


status_t
ResponseFileManager::CreateFile(const void* data, size_t size,
    const BString& contentType, entry_ref& ref)
{
    BString mimeType = _NormalizeMimeType(contentType);

    return _CreateFile(data, size, mimeType,
        _ExtensionForMimeType(mimeType), ref);
}


status_t
ResponseFileManager::CreateTextFile(const void* data, size_t size,
    entry_ref& ref)
{
    return _CreateFile(data, size, "text/plain", ".txt", ref);
}
