# MultiThreadingOSHwk

## Overview
This is a simple, multi-threaded HTTP server written in C++. The server listens for incoming HTTP requests and responds with the appropriate file or an error message. It uses POSIX threads to handle multiple concurrent requests, with each request being handled by a separate thread. This allows the server to scale with the number of incoming requests.

Additionally, the server features a logger functionality running on a separate thread, logging information such as the HTTP method, URI, status code, content type, and length.

## Features

- **HTTP Request Handling**: Parses HTTP requests and sends back requested files or an error message if the file doesn't exist.
- **Multi-threaded**: Creates a new thread for each incoming request, allowing for handling of multiple concurrent requests.
- **Logging**: Logs each request's information, including HTTP verb, URI, status code, and response details.
- **Shutting Down**: Ability to shut down via a special "/quit" URI request.
- **Error Handling**: Manages errors during socket creation, binding, or listening.
- **File Not Found**: Responds with a 404 Not Found status code and message for non-existent files.

## How to Use

1. Compile the server code using a C++ compiler.
2. Execute the compiled binary to run the server. It will listen for HTTP requests on port 8080.
3. Send requests using an HTTP client. For example, GET `http://localhost:8080/example.html` to request a file.
4. To stop the server, send a GET request to `http://localhost:8080/quit`. The server will then shut down.
