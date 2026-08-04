# NetWorker

![Screenshot](screenshot.png)

**NetWorker** is a lightweight, Haiku-native REST API client, inspired by tools like Postman and HttpShout. Create, send, and inspect HTTP requests.

Built using the `netservices2` library.

Compose a request with method, URL, headers, form parameters, or a raw body, authenticate with Basic, Bearer, or API key auth. Each request is saved in a history list, and can be restored.

---

## Features

- **Request builder**
  - Method selector: GET, QUERY, POST, PUT, PATCH, DELETE
  - URL input with validation
  - Live request preview panel

- **Request body**
  - Raw body editor
  - Form-encoded parameters (`application/x-www-form-urlencoded`) with an add/remove key-value editor

- **Authorization**
  - None, Basic (username/password), Bearer token, and API key (custom header name/value)

- **Response viewer**
  - Status line
  - Response headers in a sortable column list
  - Response body view

- **History**
  - Auto-populated on every send (method, URL, body, params, and auth recorded)
  - Click an item to restore it into the request builder
  - Multi-select delete, clear-all with confirmation
  - Persists between sessions

- **Layout**
  - Resizable panel layout: history sidebar, request/preview split, response panel

---

## Build Instructions

To build the app:

```bash
make
```

---


## License

[MIT License](LICENSE)

---

## Contributions

Pull requests and suggestions are welcome.