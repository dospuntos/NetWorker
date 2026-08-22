/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef COLLECTION_EXPORTER_H
#define COLLECTION_EXPORTER_H

#include <String.h>
#include <SupportDefs.h>

class Collection;

class CollectionExporter {
public:
    static status_t Export(Collection* collection, const BString& path);
};

#endif // COLLECTION_EXPORTER_H
