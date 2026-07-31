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

enum {
	M_NEW_FILE = 'fnew',
	M_SHOW_HELP = 'help',
	M_REPORT_A_BUG = 'bugs',

	M_SEND_REQUEST = 'send',
	M_CLEAR_RESPONSE = 'clrr',
	M_ADD_PARAMETER = 'padd',
	M_REMOVE_PARAMETER = 'prem',
	M_SELECT_PARAMETER = 'psel',
	M_SELECT_HISTORY = 'hsel'
};


#endif // CONSTANTS_H
