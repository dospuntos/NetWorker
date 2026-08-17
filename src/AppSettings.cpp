/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "AppSettings.h"
#include "Constants.h"

#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <Message.h>


AppSettings::AppSettings()
	:
	fSaveFieldsOnExit(false),
	fTimeoutSeconds(30),
	fMaxHistoryItems(100),
	fVerifySSL(true),
	fFollowRedirects(true),
	fMaxResponseSize(10 * 1024 * 1024), // 10 MB
	fWordWrap(true)
{
}


status_t
AppSettings::_FilePath(BPath& path) const
{
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status != B_OK)
		return status;

	status = path.Append(kSettingsDirName);
	if (status != B_OK)
		return status;

	return path.Append(kPreferencesFileName);
}


status_t
AppSettings::Load()
{
	BPath path;
	status_t status = _FilePath(path);
	if (status != B_OK)
		return status;

	BFile file;
	status = file.SetTo(path.Path(), B_READ_ONLY);
	if (status != B_OK)
		return status;

	BMessage archive;
	status = archive.Unflatten(&file);
	if (status != B_OK)
		return status;

	archive.FindBool("saveFieldsOnExit", &fSaveFieldsOnExit);
	archive.FindString("defaultUserAgent", &fDefaultUserAgent);
	archive.FindInt32("timeoutSeconds", &fTimeoutSeconds);
	archive.FindInt32("maxHistoryItems", &fMaxHistoryItems);
	archive.FindBool("verifySSL", &fVerifySSL);
	archive.FindBool("followRedirects", &fFollowRedirects);
	archive.FindInt64("maxResponseSize", &fMaxResponseSize);
	archive.FindBool("wordWrap", &fWordWrap);

	return B_OK;
}


status_t
AppSettings::Save()
{
	BPath path;
	status_t status = _FilePath(path);
	if (status != B_OK)
		return status;

	BPath dirPath(path);
	dirPath.GetParent(&dirPath);
	create_directory(dirPath.Path(), 0755);

	BFile file;
	status = file.SetTo(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (status != B_OK)
		return status;

	BMessage archive;
	archive.AddBool("saveFieldsOnExit", fSaveFieldsOnExit);
	archive.AddString("defaultUserAgent", fDefaultUserAgent);
	archive.AddInt32("timeoutSeconds", fTimeoutSeconds);
	archive.AddInt32("maxHistoryItems", fMaxHistoryItems);
	archive.AddBool("verifySSL", fVerifySSL);
	archive.AddBool("followRedirects", fFollowRedirects);
	archive.AddInt64("maxResponseSize", fMaxResponseSize);
	archive.AddBool("wordWrap", fWordWrap);

	return archive.Flatten(&file);
}
