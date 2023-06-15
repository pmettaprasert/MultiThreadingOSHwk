# MultiThreadingOSHwk
Overview:
This is a simple, multi-threaded HTTP server written in C++. The server listens for incoming HTTP requests and responds with the appropriate file or an error message. It uses POSIX threads to handle multiple concurrent requests. Each request is handled by a separate thread, allowing the server to scale with the number of incoming requests.

This server also has a logger functionality, which logs information about the HTTP requests it serves. It runs on a separate thread and communicates with the main thread via a pipe. The logger logs the method (GET), the URI, the status code (200 for OK, 404 for not found), and additional information about the response such as the content type and length.

Features:
HTTP request handling: The server parses HTTP requests and sends back the requested files if they exist, or an error message if they don't.

Multi-threaded: The server creates a new thread for each incoming request. This ensures that it can handle multiple concurrent requests without any delay.

Logging: The server logs each request's information. For each HTTP request it serves, it logs the HTTP verb, the URI, the HTTP status code, and additional information about the response.

Shutting down the server: If a client sends a request with "/quit" URI, the server sends a message that the server is shutting down and then terminates itself.

Error handling: Errors during socket creation, binding, or listening are handled and appropriate error messages are logged.

File not found: If a client requests a file that does not exist, the server responds with a 404 Not Found status code and a HTML message describing the error.

How to Use:

Compile the server code using a C++ compiler. To run the server, execute the compiled binary. The server will start listening for HTTP requests on port 8080.

You can send requests to the server using any HTTP client. For example, to request a file named "example.html", you would send a GET request to "http://localhost:8080/example.html".

To stop the server, send a GET request to "http://localhost:8080/quit". The server will send a final response and then shut itself down.
