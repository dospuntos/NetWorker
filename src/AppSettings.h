/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef APP_SETTINGS_H
#define APP_SETTINGS_H

#include <Path.h>
#include <String.h>
#include <SupportDefs.h>

class AppSettings {
public:
    AppSettings();

    status_t Load();
    status_t Save();

    bool     fSaveFieldsOnExit;
    BString  fDefaultUserAgent;
    int32    fTimeoutSeconds;
    int32    fMaxHistoryItems;
    bool     fVerifySSL;
    bool     fFollowRedirects;
    int64    fMaxResponseSize;
    bool     fWordWrap;

private:
    status_t _FilePath(BPath& path) const;
};

#endif // APP_SETTINGS_H
