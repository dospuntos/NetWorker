/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "CollectionExporter.h"

#include <File.h>
#include <Message.h>

#include <nlohmann/json.hpp>

#include "Collection.h"
#include "CollectionItem.h"

using json = nlohmann::json;

namespace {

json
ParamArrayToJson(const BMessage& params, const char* subMessageName)
{
    json arr = json::array();

    BMessage param;
    for (int32 i = 0; params.FindMessage(subMessageName, i, &param) == B_OK; i++) {
        BString key, value;
        param.FindString("key", &key);
        param.FindString("value", &value);

        json entry;
        entry["key"] = key.String();
        entry["value"] = value.String();
        arr.push_back(entry);

        param.MakeEmpty();
    }

    return arr;
}

} // namespace

status_t
CollectionExporter::Export(Collection* collection, const BString& path)
{
    json root;

    try {
        root["meta"]["format"] = "networker";
        root["meta"]["version"] = "1.0";

        json requests = json::array();

        for (int32 i = 0; i < collection->CountItems(); i++) {
            CollectionItem* item = collection->ItemAt(i);
            const RequestData& data = item->fData;

            json req;
            req["label"] = item->Text();
            req["method"] = data.fMethod.String();
            req["url"] = data.fUrl.String();
            req["bodyMode"] = data.fBodyMode.String();
            req["body"] = data.fBody.String();
            req["filePath"] = data.fFilePath.String();
            req["params"] = ParamArrayToJson(data.fParams, "param");
            req["queryParams"] = ParamArrayToJson(data.fQueryParams, "param");
            req["customHeaders"] = ParamArrayToJson(data.fCustomHeaders, "header");
            req["authType"] = data.fAuthType.String();

            json authValues = json::object();
            if (data.fAuthType == "basic") {
                BString username, password;
                data.fAuthValues.FindString("username", &username);
                data.fAuthValues.FindString("password", &password);
                authValues["username"] = username.String();
                authValues["password"] = password.String();
            } else if (data.fAuthType == "bearer") {
                BString token;
                data.fAuthValues.FindString("token", &token);
                authValues["token"] = token.String();
            } else if (data.fAuthType == "apikey") {
                BString headerName, headerValue;
                data.fAuthValues.FindString("headerName", &headerName);
                data.fAuthValues.FindString("headerValue", &headerValue);
                authValues["headerName"] = headerName.String();
                authValues["headerValue"] = headerValue.String();
            }
            req["authValues"] = authValues;

            requests.push_back(req);
        }

        root["collection"]["name"] = collection->Name().String();
        root["collection"]["requests"] = requests;
    } catch (const json::exception&) {
        return B_ERROR;
    }

    std::string dumped = root.dump(2);   // pretty-printed, 2-space indent — readable, diffable

    BFile file(path.String(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
    status_t status = file.InitCheck();
    if (status != B_OK)
        return status;

    ssize_t written = file.Write(dumped.data(), dumped.size());
    if (written < 0)
        return (status_t)written;
    if ((size_t)written != dumped.size())
        return B_IO_ERROR;

    return B_OK;
}