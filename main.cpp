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
#include <algorithm>
#include <thread>    


bool DoGet(int socket, std::string request)
{
    
    if (std::string::npos != request.find("?"))
        request = request.substr(0, request.find("?"));
    if (std::string::npos != request.find("HTTP"))
        request = request.substr(0, request.find("HTTP"));

    std::string path = request.substr(request.find("/"), request.size() - request.find("/"));
    std::string fullpath = "." + path;
//    std::cerr << "*_* path document: " << fullpath; //aux
    fullpath.erase(std::remove(fullpath.begin(), fullpath.end(), ' '), fullpath.end());
int fd = open(fullpath.c_str(), O_RDONLY);
    if (-1 == fd ) //if error ENOENT
    {
//        std::cerr << "*_* cannot open file " <<std::endl;
 //       perror("open file");
        std::string responce = "HTTP/1.0 404 NOT FOUND\r\nContent-Length: 0\r\nContent-Type: text/html\r\n\r\n";
        send(socket, responce.c_str(), responce.size(), 0);
        return false;
    }

//    std::string responce = "HTTP/1.0 200 OK \r\n Server: Myserver(v 1.0) Stepic/cpp \r\n Content-Type: text/html; charset = utf-8 \r\n \r\n";
  
  std::string responce = "HTTP/1.0 200 OK\r\n\r\n";
    send(socket, responce.c_str(), responce.size(), 0);
    char readBuf[1024];
    while ( int cntRead = read(fd, readBuf, 1024))
    {
        send(socket, readBuf, cntRead, 0);
    }
    close(fd);
//    std::cerr << "*_* transfer complite" << std::endl; //aux
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
        perror("*_* read trouble: ");
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
//    std::cerr << "*_* request: " << requestStr; //aux
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
    char* port = "54321";
    char* ip = "127.0.0.1";
    char* directory = "tmp";
//here we use getopt
    int opchar = 0;
    while (-1 != (opchar = getopt(argc, argv, "h:p:d:")))
    {
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
 //   std::cerr << "my pid is " << getpid() <<std::endl;

    //here we will demonize ]:->
    int a = 0;
    if ( 0 == (a =fork()) ) //if child
    {
        setsid();
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        if (chdir(directory))
        {
   //         std::cerr << "*_* error in chdir" << std::endl;
        }
        //now start listenning
        if (listen(masterSocket, SOMAXCONN) ) //returns 0 if success
        {
 //           perror("trouble with listenning: ");
            return 1;
        }
        int ss = 1;
        while ( 1 ) {
            ss = accept(masterSocket, 0, 0);
 //           std::cerr << "*_* accepted " << ss << std::endl;
//#pragma omp task
//            {
std::thread t(worker, ss);
t.detach();
//                worker(ss);
//            }
        }
//#pragma omp taskwait

        return 0;
    }
   else
    {
//    std::cerr << "child pid = " << a << std::endl;
    return 0;
    }
    return 0;

}
// curl -I -0 -X GET "http://127.0.0.1:54321/index.html"
