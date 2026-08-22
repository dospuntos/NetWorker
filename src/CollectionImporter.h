/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef COLLECTION_IMPORTER_H
#define COLLECTION_IMPORTER_H

#include <String.h>
#include <SupportDefs.h>

class Collection;

class CollectionImporter {
public:
    static status_t Import(const BString& path, Collection*& outCollection, BString& outError);
};

#endif // COLLECTION_IMPORTER_H
