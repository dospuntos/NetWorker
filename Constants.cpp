/*
 * Copyright 2026, Johan Wagenheim <johan@dospuntos.no>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "Constants.h"
#include <Catalog.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "App"

const char* kApplicationSignature = "application/x-vnd.jpw-NetWorker";
const char* kApplicationName = B_TRANSLATE_SYSTEM_NAME("NetWorker");
const char* kSettingsFile = "NetWorker_settings";
