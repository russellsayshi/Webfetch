#include "connection.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <cstring>

std::vector<unsigned char> hex_to_vec(std::string chars) {
	std::istringstream stream(chars);
	std::vector<unsigned char> ret;

	unsigned int c;
	while(stream >> std::hex >> c) {
		ret.push_back(c);
	}
	return ret;
}

std::string hex(char c) {
	std::stringstream stream;
	stream << std::hex << std::uppercase << (c & 0xFF);
	return stream.str();
}

std::string bin(char c) {
	char res[9];
	int bitmask = 1;
	for(int i = 0; i < 8; i++) {
		res[7-i] = '0' + (((c & bitmask) == 0) ? 0 : 1);
		bitmask = bitmask << 1;
	}
	res[8] = 0;
	return std::string(res);
}

int dblint(char* loc) {
	return (*loc & 0xFF) * 256 + (*(loc+1) & 0xFF);
}

int print_name(std::string prefix, char* response, int* ctr) {
	std::cout << prefix << "Start QNAME at " << *ctr << std::endl;
	do {
		int len = response[*ctr];
		if(len == 0) {
			std::cout << prefix << "End QNAME" << std::endl;
			break;
		}
		std::cout << prefix << "Found question of length " << len << ": ";
		for(int i = 0; i < len; i++) {
			(*ctr)++;
			std::cout << response[*ctr];
		}
		std::cout << std::endl;
		(*ctr)++;
	} while(1);
}

int main(int argc, char** argv) {
	if(argc < 3) {
		std::cerr << "Please use this in the form: " << argv[0] << " dns_server host" << std::endl;
		std::cerr << "e.g. " << argv[0] << " 8.8.8.8 example.com" << std::endl;
		return 3;
	}

	//parse hostname
	std::vector<std::string> domain_parts;
	int word_start_loc = 0;
	int len_domain = strlen(argv[2]);
	int num_bytes_required_to_store_domain = 0;
	for(int i = 1; i <= len_domain; i++) {
		if(i == len_domain || argv[2][i] == '.') {
			int len = i - word_start_loc;
			domain_parts.push_back(std::string(argv[2] + word_start_loc, len));
			word_start_loc = i+1;
			num_bytes_required_to_store_domain += 1 + len;
		}
	}

	//build up request
	//format: AA AA 01 00 00 01 00 00 00 00 00 00 <domain stuff> 00 00 01 00 01
	unsigned int bytes_len = 17 + num_bytes_required_to_store_domain;
	unsigned char bytes[bytes_len];
	bytes[0] = 0xAA;
	bytes[1] = 0xAA;
	bytes[2] = 0x01;
	bytes[3] = 0x00;
	bytes[4] = 0x00;
	bytes[5] = 0x01;
	bytes[6] = 0x00;
	bytes[7] = 0x00;
	bytes[8] = 0x00;
	bytes[9] = 0x00;
	bytes[10] = 0x00;
	bytes[11] = 0x00;

	int byteptr = 12;
	for(int i = 0; i < domain_parts.size(); i++) {
		int len = domain_parts[i].length();
		if(len > 253) {
			std::cerr << "Cannot have a domain part of length 253." << std::endl;
			return 6;
		}
		bytes[byteptr++] = len;
		for(int o = 0; o < len; o++) {
			bytes[byteptr++] = domain_parts[i][o];
		}
	}

	bytes[bytes_len-1] = 0x01;
	bytes[bytes_len-2] = 0x00;
	bytes[bytes_len-3] = 0x01;
	bytes[bytes_len-4] = 0x00;
	bytes[bytes_len-5] = 0x00;

	connection c(argv[1], 53, false);
	std::cout << c.send((const char*)bytes, bytes_len) << std::endl;

	std::cout << "sent" << std::endl;
	char response[4096];
	int chars_read = c.recv(response, 4096);
	std::cout << "recv'd " << chars_read << " characters." << std::endl;

	std::cout << "ID: " << hex(response[0]) << " " << hex(response[1]) << std::endl;
	std::cout << "Flags: " << bin(response[2]) << " " << bin(response[3]) << std::endl;
	std::cout << "# questions: " << dblint(&response[4]) << std::endl;
	int num_responses = dblint(&response[6]);
	std::cout << "# responses: " << num_responses << std::endl;
	std::cout << "# authority records: " << dblint(&response[8]) << std::endl;
	std::cout << "# additional records: " << dblint(&response[10]) << std::endl;

	if(chars_read <= 12) {
		//no name was sent
		std::cout << "No name sent." << std::endl;
		return 0;
	}

	int ctr = 12;

	print_name("", response, &ctr);

	ctr++;
	std::cout << "QTYPE: " << dblint(&response[ctr]) << std::endl;
	ctr += 2;
	std::cout << "QCLASS: " << dblint(&response[ctr]) << std::endl;
	ctr += 2;

	for(int i = 0; i < num_responses; i++) {
		std::cout << "Response " << i << ":" << std::endl;
		int name_pos = dblint(&response[ctr]);
		ctr += 2;
		const int name_bitmask = 3 << 14;
		if((name_pos & name_bitmask) != 0xC000) {
			std::cerr << "I don't know how to parse this name." << std::endl;
			return 2;
		}
		name_pos = name_pos & ~(name_bitmask);
		print_name("    ", response, &name_pos);
		std::cout << "    QTYPE: " << dblint(&response[ctr]) << std::endl;
		ctr += 2;
		std::cout << "    QCLASS: " << dblint(&response[ctr]) << std::endl;
		ctr += 2;
		unsigned long ttl = ((unsigned long)(dblint(&response[ctr])) << 16) + dblint(&response[ctr+2]);
		std::cout << "    TTL: " << ttl << std::endl;
		ctr += 4;
		int rdlength = dblint(&response[ctr]);
		ctr += 2;
		std::cout << "    RDLENGTH: " << rdlength << std::endl;
		std::cout << "    RDATA: ";
		for(int o = 0; o < rdlength; o++) {
			std::cout << (response[ctr] & 0xFF);
			ctr++;
			if(o != rdlength-1) {
				std::cout << ".";
			}
		}
		std::cout << std::endl;
	}

	/*std::cout << "ENTERED" << std::endl;
	connection c("172.217.9.174", 80, true);
	std::cout << "CONNECTED"<< std::endl;
	std::cout << c.send("GET /index.html HTTP/1.0\r\nHost: www.google.com\r\n\r\n\r\n\r\n") << std::endl;
	std::cout << "SENT" << std::endl;
	std::cout << c.recv() << std::endl;
	std::cout << c.recv() << std::endl;
	std::cout << "DONE" << std::endl;*/
}
