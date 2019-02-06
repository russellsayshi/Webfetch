#ifndef WF_CONNECTION_H
#define WF_CONNECTION_H

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <string>

class connection {
private:
	int sockfd;
	bool tcp;
	unsigned int buffsize;
	char* buffer = nullptr;
	struct sockaddr_in servaddr;

public:
	connection(std::string ip, uint16_t port, bool tcp, unsigned int buffsize = 4096);
	connection(const connection&) = delete;
	connection& operator=(const connection&) = delete;
	connection(connection&&) = default;
	int send(const char* data, unsigned int len);
	int recv(char* usr_buffer, unsigned int len);
	int send(std::string data);
	std::string recv();
	~connection();
};

#endif
