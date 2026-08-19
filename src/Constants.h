/*
 * Copyright 2026, Johan Wagenheim <johan@dospuntos.no>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <SupportDefs.h>

extern const char* kApplicationSignature;
extern const char* kApplicationName;
extern const char* kSettingsFile;
extern const char* kSettingsDirName;
extern const char* kCollectionsDirName;
extern const char* kCollectionsIndexFileName;
extern const char* kPreferencesFileName;

enum {
	M_NEW_FILE = 'fnew',
	M_SHOW_HELP = 'help',
	M_REPORT_A_BUG = 'bugs',
	M_SHOW_SETTINGS = 'stng',
	M_SETTINGS_CHANGED = 'stch',
	M_SETTINGS_APPLIED = 'stap',
	M_SETTINGS_WINDOW_CLOSED = 'stcl',
	M_SETTINGS_CATEGORY_SELECTED = 'scat',
	M_NOT_IMPLEMENTED = 'nimp',
	M_TOGGLE_PREVIEW = 'tprv',
	M_TOGGLE_SIDEBAR = 'tsbr',

	M_NEW_REQUEST = 'nrqs',
	M_SEND_REQUEST = 'send',
	M_CLEAR_RESPONSE = 'clrr',
	M_REQUEST_TIMEOUT = 'rqto',
	M_HEADER_ADD = 'hdad',
	M_HEADER_REMOVE = 'hdrm',
	M_HEADER_SELECT = 'hdsl',
	M_FORM_PARAM_ADD = 'fpad',
	M_FORM_PARAM_REMOVE = 'fprm',
	M_FORM_PARAM_SELECT = 'fpsl',
	M_QUERY_PARAM_ADD = 'fqad',
	M_QUERY_PARAM_REMOVE = 'fqrm',
	M_QUERY_PARAM_SELECT = 'fqsl',
	M_BODY_MODE_CHANGED = 'bmch',
	M_BODY_FILE_SELECTED = 'bfsl',
	M_SHOW_BODY_FILE_PANEL = 'sfpn',
	M_AUTH_TYPE_CHANGED = 'auth',
	M_SELECT_HISTORY = 'hsel',
	M_DELETE_HISTORY_ITEM = 'dhis',
	M_CLEAR_HISTORY = 'clrh',
	M_HISTORY_SELECTION_CHANGED = 'chis',
	M_COPY_HISTORY_URL = 'cphu',
	M_SHOW_RENAME_DIALOG = 'shrn',
	M_RENAME_HISTORY_ITEM = 'rnhi',
	M_UPDATE_PREVIEW = 'upvw',

	M_SELECT_COLLECTION = 'scol',
	M_NEW_COLLECTION = 'ncol',
	M_CREATE_COLLECTION = 'crcl',
	M_DELETE_COLLECTION = 'dcol',
	M_CONFIRM_DELETE_COLLECTION = 'cdcl',
	M_COPY_COLLECTION_URL = 'cpcu',

	M_SAVE_TO_COLLECTION = 'stco',
	M_CONFIRM_SAVE_TO_COLLECTION = 'cstc',

	M_LOAD_COLLECTION_ITEM = 'lcit',
	M_SHOW_RENAME_COLLECTION_ITEM = 'srci',
	M_RENAME_COLLECTION_ITEM = 'rnci',
	M_DELETE_COLLECTION_ITEM = 'dcit',

};


#endif // CONSTANTS_H
