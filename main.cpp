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


bool doGet(int socket, std::string request)
{
	if (std::string::npos != request.find("?"))
		request = request.substr(0, request.find("?"));
	if (std::string::npos != request.find("HTTP"))
		request = request.substr(0, request.find("HTTP"));

	std::string fullpath = request.substr(request.find("/"), request.size() - request.find("/"));
	fullpath = "." + fullpath;
	fullpath.erase(std::remove(fullpath.begin(), fullpath.end(), ' '), fullpath.end());
	int fd = open(fullpath.c_str(), O_RDONLY);
	if (-1 == fd) //if cannot open file
	{
		std::string responce = "HTTP/1.0 404 NOT FOUND\r\nContent-Length: 0\r\nContent-Type: text/html\r\n\r\n";
		send(socket, responce.c_str(), responce.size(), 0);
		return false;
	}

	std::string responce = "HTTP/1.0 200 OK\r\n\r\n";
	send(socket, responce.c_str(), responce.size(), 0);
	char readBuf[1024];
	while (int cntRead = read(fd, readBuf, 1024))
	{
		send(socket, readBuf, cntRead, 0);
	}
	close(fd);
	return true;
}

bool checkGet(std::string request)
{ 
    return ( (request.size() > 3) && ( "GET" == request.substr(0, 3) ) );
}

bool worker(int socket)
{
	char buf[2048];
	int cntRead = read(socket, buf, 2048);
	if (-1 == cntRead) //if error
	{
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
	std::string requestStr(buf, (unsigned long)cntRead);
	if (checkGet(requestStr))
	{
		doGet(socket, requestStr);
	}
	else 		//400 Bad Request
	{
		std::string responce = "HTTP/1.0 400 Bad Request\r\nContent-Length: 0\r\nContent-Type: text/html\r\n\r\n";
		send(socket, responce.c_str(), responce.size(), 0);
	}

	shutdown(socket, SHUT_RDWR);
	close(socket);
	return true;
}



int main(int argc, char** argv) {

	char* port = "54321";
	char* ip = "127.0.0.1";
	char* directory = "tmp";
	int opchar = 0;
	while (-1 != (opchar = getopt(argc, argv, "h:p:d:")))
	{
		switch (opchar)
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

    //create daemon due to task
	if (!fork()) //if child
	{
		//daemon
		setsid();
		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
		chdir(directory);

		//now start listenning
		if (listen(masterSocket, SOMAXCONN)) //returns 0 if success
		{
			return 1;
		}
		while (1) {
			int slaveSocket = accept(masterSocket, 0, 0);
			std::thread thr(worker, slaveSocket);
			thr.detach();
		}
	}
	return 0;

}
