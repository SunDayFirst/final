#include <iostream>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

bool DoGet(int socket, std::string request)
{
    std::string path = request.substr(3, (request.size() - request.find("HTTP") - 3));
    std::cerr << "path document: " << path; //aux

int fd = open(path.c_str(), O_RDONLY);
    if (-1 == fd ) //if error ENOENT
    {
        char* responce = "HTTP/1.0 404 NOT FOUND";
        send(socket, responce, sizeof(responce), 0);
        return false;
    }

    char* responce = "HTTP/1.0 200 OK \n Server: Myserver(v 1.0) Stepic/cpp \n Content-Type: text/html; charset = utf-8 \n \n";
    send(socket, responce, sizeof(responce), 0);
    char readBuf[1024];
    while ( int cntRead = read(fd, readBuf, 1024))
    {
        send(socket, readBuf, cntRead, 0);
    }
    std::cerr << "transfer complite" << std::endl; //aux
    return true;
}

bool checkGet(std::string request)
{
    if ("GET" == request.substr(0, 3))
        return true;
    return false;
}

bool worker(int socket)
{
    char buf[2048];
    int cntRead = read(socket, buf, 2048);
    if (-1 == cntRead) //if error
    {
        perror("read trouble: ");
        shutdown(socket, SHUT_RDWR);
        close(socket);
        return false;
    }
    if (0 == cntRead) //if EOF
    {
        shutdown(socket, SHUT_RDWR);
        close(socket);
        return false;
    }
    std::string requestStr(buf,(unsigned long)cntRead);
    std::cerr << "request: " << requestStr; //aux
    if (checkGet(requestStr))
    {
        DoGet(socket, requestStr);
    }
    else
    {
        //400 Bad Request
    }

    shutdown(socket, SHUT_RDWR);
    close(socket);
    return true;
}



int main(int argc, char** argv) {

//get port, get ip from argv
    char* port = NULL;
    char* ip = NULL;
    char* directory = NULL;
//here we use getopt
    int opchar = 0;
    while (-1 != (opchar = getopt(argc, argv, "h:p:d:")))
    {
//	printf("opchar = %c (%d) oparg = %c \n", opchar, opchar, optarg);
        switch(opchar)
        {
            case 'h':
            {
                ip = optarg;
                break;
            }
            case 'p':
            {
                port = optarg;
                break;
            }
            case 'd':
            {
                directory = optarg;
                break;
            }
            default:
                break;
        }
    }

    //create Master Socket
    struct sockaddr_in sa_in;
    sa_in.sin_family = AF_INET;
    sa_in.sin_port = htons(atoi(port));
    inet_aton(ip, &sa_in.sin_addr);
    int masterSocket = socket(AF_INET, SOCK_STREAM, 0);
    bind(masterSocket, (struct sockaddr*)&sa_in, sizeof(sa_in));

    //here we will demonize ]:->
    setsid();
    chdir(directory);
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    //now start listenning
    if (listen(masterSocket, SOMAXCONN) ) //returns 0 if success
    {
        perror("trouble with listenning: ");
        return 1;
    }

    while (int ss = accept(masterSocket, 0, 0))
    {
#pragma omp task
        {
            worker(ss);
        }
    }
#pragma omp taskwait

    return 0;
}