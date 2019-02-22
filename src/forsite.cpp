#include <iostream>
#include "connection.h"

#define REQUEST_LENGTH 28

int main(int argc, char** argv) {

	/* Load in our request data. */
	unsigned char request[REQUEST_LENGTH] = {
		0xDE, 0xAD, 0x01, 0x00,
		0x00, 0x01, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x06, 0x67, 0x6F, 0x6F,
		0x67, 0x6C, 0x65, 0x03,
		0x63, 0x6F, 0x6D, 0x00,
		0x00, 0x01, 0x00, 0x01
	};

	/*
	 * Connect to the DNS server.
	 * 8.8.8.8 is a very common DNS server owned by Google
	 * 53 is the port we are using
	 * and false = UDP (true = TCP)
	 */
	connection conn("8.8.8.8", 53, false);

	conn.send((const char*)request, REQUEST_LENGTH);

	char response[4096];
	int chars_read = conn.recv(response, 4096);

	std::cout << "Received " << chars_read << " character(s) from the server." << std::endl;


	/* Print out our response */
	std::cout << "0x";
	for(int i = 0; i < chars_read; i++) {
		std::cout << std::hex << (0xff & response[i]);
	}
	std::cout << std::endl;
}