#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <vector>
#include <sstream>
#include <unistd.h>
#include <pthread.h>
#include <mutex>


using namespace std;

#define PORT 8080


//create a string path to the test files folder
const string PATH_TO_TEST_FILES = "testsuite";

//create a struct to hold the header information
typedef struct {
    string key;
    string value;
} http_header_t;

//create a struct to hold the request information
typedef struct {
    string verb;
    string version;
    string uri;
    vector<http_header_t> headers;
} http_request_t;

//create a pipe to send information to the logger
int fd[2];

//create the mutex
pthread_mutex_t mutexLock;

//create a mutex for handling the requests
pthread_mutex_t requestMutex;

//Using this function to get the thread id for Mac users
long get_tid_xplat() {
#ifdef __APPLE__
    long   tid;
    pthread_t self = pthread_self();
    int res = pthread_threadid_np(self, reinterpret_cast<__uint64_t *>(&tid));
    return tid;
#else
    pid_t tid = gettid();
    return (long) tid;
#endif
}


vector<string> split(string &src, char delim);

http_request_t parseRequest(char *buffer);

//method header to handle the request via a thread
void *handleRequest(void *arg);

//method header to handle the logger
void *loggerRunner(void *arg);

//method header to print buffer
void printBuffer();


int main() {

    //create a logger thread
    pthread_t loggerThread;
    if(pthread_create(&loggerThread, nullptr, loggerRunner, nullptr) != 0) {
        perror("Error creating logger thread");
        return -1;
    }

    //create a pipe
    if (pipe(fd) < 0) {
        perror("Error creating pipe");
        return -1;
    }

    //create a mutex
    if (pthread_mutex_init(&mutexLock, nullptr) != 0) {
        perror("Error creating mutex");
        return -1;
    }

    //create a mutex for handling the requests
    if (pthread_mutex_init(&requestMutex, nullptr) != 0) {
        perror("Error creating mutex");
        return -1;
    }

    //create a socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Error creating socket");
        return -1;
    }

    //create a sockaddr_in struct
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    //bind the socket to the address
    if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) <
        0) {
        perror("Error binding socket");
        return -1;
    }

    //listen for connections
    if (listen(server_fd, 10) < 0) {
        perror("Error listening for connections");
        return -1;
    }

    //accept connections and create a thread to handle the request
    int client_fd;
    struct sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);

    //the server will run until the quit command is sent from the client
    // which is handled in the child thread
    while (true) {
        client_fd = accept(server_fd, (struct sockaddr *) &client_addr,
                           &client_len);
        if (client_fd < 0) {
            perror("Error accepting connection");
            return -1;
        }

        //create a thread to handle the request
        pthread_mutex_lock(&requestMutex);
        pthread_t thread;
        pthread_create(&thread, nullptr, handleRequest, (void *) &client_fd);
        pthread_detach(thread);
        pthread_mutex_unlock(&requestMutex);

    }
    return 0;
}

//function to handle request via a thread
void *handleRequest(void *arg) {

    //get the client socket from the argument
    int client_fd = *((int *) arg);

    //create a buffer to hold the request
    char buffer[10000];

    //read the request into the buffer
    read(client_fd, buffer, 10000);

    //parse the request and store it in the request struct
    http_request_t request = parseRequest(buffer);

    //check if the file exists
    string path = PATH_TO_TEST_FILES + request.uri;

    //open the file
    FILE *file = fopen(path.c_str(), "r");
    if (file == nullptr) {

        //if the file does not exist, send a 404
        string response = "HTTP/1.1 404 Not Found\r\n";

        //for quit uri send a different message saying the server is shutting
        // down
        if (request.uri == "/quit") {
            string quitHTMLMessage = "<html><body><h1>Quit command has "
                                     "been issued.</h1>"
                                     "<p>The server is shutting down.</p>"
                                     "</body></html>";
            response += "Content-Type: text/html\r\n";
            response += "Content-Length: " + to_string(quitHTMLMessage.length()) + "\r\n\r\n";
            response += quitHTMLMessage;
            pthread_mutex_lock(&mutexLock);
            send(client_fd, response.c_str(), response.length(), 0);
            pthread_mutex_unlock(&mutexLock);

        } else {
            //html message
            string html = "<html><body><h1>404 Not Found</h1>"
                          "<p>The file that you requested (" + request.uri +
                          ") does not exist.</p>"
                          "</body></html>";

            //add the content type and content length to the response
            response += "Content-Type: text/html\r\n";
            response +=
                    "Content-Length: " + to_string(html.length()) + "\r\n\r\n";

            //add the html to the response
            response += html;

            pthread_mutex_lock(&mutexLock);
            send(client_fd, response.c_str(), response.length(), 0);
            pthread_mutex_unlock(&mutexLock);
        }

        //send message to logger that includes the verb, uri, and 404 File not found
        string message = request.verb + " " + request.uri + " 404 File not found\n";

        //write to the pipe with mutex
        pthread_mutex_lock(&mutexLock);
        write(fd[1], message.c_str(), message.length());
        pthread_mutex_unlock(&mutexLock);

        //if it is a quit command, close the server
        if (request.uri == "/quit") {

            //send a message to the logger
            string message = "Shutting down.";


            //write to the pipe
            pthread_mutex_lock(&mutexLock);
            write(fd[1], message.c_str(), message.length());
            pthread_mutex_unlock(&mutexLock);

            //sleep for 1 second
            sleep(1);

            close(client_fd);
            exit(0);
        }
    } else {

        //if the file exists, send a 200
        string response = "HTTP/1.1 200 OK\r\n";

        //determine content type whether it is, html, js, css, png, plain text
        string contentType = "text/plain";
        if (path.find(".html") != string::npos) {
            contentType = "text/html";
        } else if (path.find(".js") != string::npos) {
            contentType = "text/javascript";
        } else if (path.find(".css") != string::npos) {
            contentType = "text/css";
        } else if (path.find(".png") != string::npos) {
            contentType = "image/png";
        }

        //add the content type to the response
        response += "Content-Type: " + contentType + "\r\n";

        //determine the content length
        fseek(file, 0, SEEK_END);
        int contentLength = ftell(file);
        fseek(file, 0, SEEK_SET);

        //add the content length to the response
        response += "Content-Length: " + to_string(contentLength) + "\r\n";

        //add the connection close to the response
        response += "Connection: Closed\r\n\r\n";


        //create a string to send to logger containing the verb, uri, 200 ok,
        // content type and content length
        string loggerString = request.verb + " " + request.uri + " 200 Ok " +
                              "content-type: " + contentType + " "
                                                               "content-length: " + to_string
                                      (contentLength) + "\n";

        //write to the pipe with mutex
        pthread_mutex_lock(&mutexLock);
        write(fd[1], loggerString.c_str(), loggerString.length());
        pthread_mutex_unlock(&mutexLock);


        //add the file contents to the response
        char fileBuffer[contentLength];
        fread(fileBuffer, 1, contentLength, file);
        response += string(fileBuffer, contentLength);


        //send the response
        pthread_mutex_lock(&mutexLock);
        send(client_fd, response.c_str(), response.length(), 0);
        pthread_mutex_unlock(&mutexLock);

        //close the file
        fclose(file);
    }

    //close the client socket
    close(client_fd);

    //return null
    return nullptr;

}


void *loggerRunner(void *arg) {

    //print out the logger pid, and tid using mutex
    pthread_mutex_lock(&mutexLock);
    printf("Logger pid: %d, tid: %lu\n", getpid(), get_tid_xplat());
    printf("Listening on port %d\n", PORT);
    pthread_mutex_unlock(&mutexLock);

    //print the buffer
    printBuffer();

    //pthread_exit
    pthread_exit(nullptr);


}

void printBuffer() {

    //create a buffer of a good size that can hold a png file
    char buffer[1000000];

    //This is the file descriptor set that will be used by select
    fd_set set;

    //This is the return value of select
    int ret;

    while(true) {
        //set the file descriptor.
        FD_ZERO(&set);
        //add the read end of the pipe to the set
        FD_SET(fd[0], &set);

        //wait for the read end of the pipe to become readable
        ret = select(fd[0] + 1, &set, nullptr, nullptr, nullptr);

        //check if the select was successful
        if (ret == -1) {
            perror("select");
            exit(1);
        } else if (ret == 0) {
            continue;
        }

        //read the pipe into the buffer
        ssize_t bytes_read = read(fd[0], buffer, sizeof(buffer));

        //check if the read was successful
        if (bytes_read == -1) {
            perror("read");
            exit(1);

            //else if the read was 0, break out of the loop
        } else if (bytes_read == 0) {
            break;
        }

        //print the buffer
        pthread_mutex_lock(&mutexLock);
        for(int i = 0; i < bytes_read; i++) {
            printf("%c", buffer[i]);
        }
        pthread_mutex_unlock(&mutexLock);

        //clear the buffer
        memset(buffer, 0, sizeof(buffer));
    }

    //close the read end of the pipe
    close(fd[0]);
}



//Function to separate the request string return a struct http_request_t
http_request_t parseRequest(char *buffer) {
    http_request_t request;
    string requestString(buffer);

    //split the request string using the split function by new line
    vector<string> lines = split(requestString, '\n');

    //parse the request line
    vector<string> requestLine = split(lines[0], ' ');
    if (requestLine.size() != 3 || requestLine[0] != "GET") {
        perror("Invalid request");
        return request;
    }


    request.verb = requestLine[0];
    request.uri = requestLine[1];
    request.version = requestLine[2];

    //parse the request headers
    for (int i = 1; i < lines.size(); i++) {
        //if the line is a carriage return, break
        if (lines[i] == "\r") {
            break;
        }

        vector<string> header = split(lines[i], ':');

        //handle the case where there is two colons in the header, take the
        // first one as the key and the rest as the value
        if (header.size() > 2) {
            string value = header[1];
            for (int j = 2; j < header.size(); j++) {
                value += ":" + header[j];
            }
            header[1] = value;
        }

        //store the headers in the header struct
        http_header_t headerStruct;
        headerStruct.key = header[0];
        headerStruct.value = header[1];

        //push the header struct into the request headers vector
        request.headers.push_back(headerStruct);


    }
    return request;

}

vector<string> split(string &src, char delim) {
    istringstream ss(src);
    vector<string> res;

    string piece;
    while (getline(ss, piece, delim)) {
        res.push_back(piece);
    }

    return res;
}
