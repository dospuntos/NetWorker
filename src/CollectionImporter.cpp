/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "CollectionImporter.h"

#include <File.h>

#include <nlohmann/json.hpp>

#include "Collection.h"
#include "CollectionItem.h"

using json = nlohmann::json;

namespace {

BMessage
JsonArrayToParamMessage(const json& arr, const char* subMessageName)
{
    BMessage result;
    if (!arr.is_array())
        return result;

    for (const auto& entry : arr) {
        BMessage param;
        param.AddString("key", entry.value("key", "").c_str());
        param.AddString("value", entry.value("value", "").c_str());
        result.AddMessage(subMessageName, &param);
    }

    return result;
}

} // namespace

status_t
CollectionImporter::Import(const BString& path, Collection*& outCollection, BString& outError)
{
    BFile file(path.String(), B_READ_ONLY);
    status_t status = file.InitCheck();
    if (status != B_OK) {
        outError = "Could not open file.";
        return status;
    }

    off_t size = 0;
    file.GetSize(&size);

    std::string content;
    content.resize((size_t)size);
    ssize_t bytesRead = file.Read(&content[0], size);
    if (bytesRead < 0 || (off_t)bytesRead != size) {
        outError = "Could not read file.";
        return B_IO_ERROR;
    }

    json root;
    try {
        root = json::parse(content);
    } catch (const json::parse_error& e) {
        outError = "File is not valid JSON.";
        return B_BAD_DATA;
    }

    std::string format = root.value("/meta/format"_json_pointer, "");
    if (format != "networker") {
        outError = "Not a NetWorker collection file "
            "(only native NetWorker JSON is supported for import right now).";
        return B_BAD_DATA;
    }

    try {
        std::string name = root.at("/collection/name"_json_pointer).get<std::string>();
        Collection* collection = new Collection(BString(name.c_str()));

        const json& requests = root.at("/collection/requests"_json_pointer);
		if (requests.is_array()) {
			for (const auto& req : requests) {
				BString method(req.value("method", "").c_str());
				BString url(req.value("url", "").c_str());
				BString body(req.value("body", "").c_str());
				BString bodyMode(req.value("bodyMode", "none").c_str());
				BString filePath(req.value("filePath", "").c_str());
				BString authType(req.value("authType", "none").c_str());

				BMessage params = JsonArrayToParamMessage(req.value("params", json::array()), "param");
				BMessage queryParams = JsonArrayToParamMessage(
					req.value("queryParams", json::array()), "param");
				BMessage customHeaders = JsonArrayToParamMessage(
					req.value("customHeaders", json::array()), "header");

				BMessage authValues;
				if (req.contains("authValues") && req["authValues"].is_object()) {
					const json& av = req["authValues"];
					for (auto& [key, value] : av.items())
						authValues.AddString(key.c_str(), value.get<std::string>().c_str());
				}

				RequestData data(method, url, customHeaders, body, params, queryParams,
					bodyMode, filePath, authType, authValues);

				BString label(req.value("label", "").c_str());
				collection->AddItem(new CollectionItem(data, label));
			}
		}

        outCollection = collection;
        return B_OK;

    } catch (const json::exception& e) {
        outError = "Collection file is missing required fields.";
        return B_BAD_DATA;
    }
}

