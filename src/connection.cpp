#include "connection.h"

#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sstream>
#include <stdexcept>
#include <sys/ioctl.h>
#include <cstring>
#include <iostream>

connection::connection(std::string ip, uint16_t port, bool tcp, unsigned int buffsize) {
	//set member variables
	this->buffsize = buffsize;
	this->tcp = tcp;

	//make our socket
	if((sockfd = socket(AF_INET, tcp ? SOCK_STREAM : SOCK_DGRAM, 0)) < 0) {
		throw std::runtime_error("could not connect to socket.");
	}

	//zero out server address struct until we can fill it ourselves
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);

	//parse IP address and convert to binary
	if(inet_pton(AF_INET, ip.c_str(), &servaddr.sin_addr) <= 0) {
		close(sockfd);
		throw std::runtime_error("invalid ip given.");
	}

	//create our buffer and fill with zeros
	buffer = new char[buffsize];
	if(buffer == nullptr) {
		close(sockfd);
		throw std::runtime_error("could not allocate buffer of given size.");
	}
	memset(buffer, 0, buffsize * sizeof(char));

	if(tcp) {
		//tcp sockets have a continuous connection going
		if(connect(sockfd, (const sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
			close(sockfd);
			delete[] buffer;
			throw std::runtime_error("could not connect");
		}
	}
}

int connection::send(const char* data, unsigned int len) {
	if(tcp) {
		return (int)::send(sockfd, data, len, 0);
	} else {
		return (int)::sendto(sockfd, data, len, 0, (const sockaddr*)&servaddr, sizeof(servaddr));
	}
}

int connection::send(std::string data) {
	return connection::send(data.c_str(), data.length());
}

int connection::recv(char* usr_buffer, unsigned int len) {
	ssize_t n;
	if(tcp) {
		n = ::recv(sockfd, usr_buffer, len-1, 0);
	} else {
		n = ::recvfrom(sockfd, (char*)usr_buffer, len-1, 0, (struct sockaddr*)&servaddr, &len);
	}
	usr_buffer[n] = '\0';
	return (int)n;
}

std::string connection::recv() {
	int n = connection::recv(buffer, buffsize);
	if(n <= 0) {
		return std::string();
	} else if(tcp) {
		int count;
		ioctl(sockfd, FIONREAD, &count);
		if(count > 0) {
			std::stringstream result;
			result << buffer;
			do {
				n = connection::recv(buffer, buffsize);
				ioctl(sockfd, FIONREAD, &count);
			} while(count > 0);
			return result.str();
		} else {
			return std::string(buffer);
		}
	} else {
		return std::string(buffer);
	}
}

connection::~connection() {
	if(buffer != nullptr) {
		delete[] buffer;
	}
	close(sockfd);
}
