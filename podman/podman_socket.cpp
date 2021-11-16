#include <iostream>

#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <algorithm>
#include <map>

#include <sys/un.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "common.hpp"
#include "mutex.hpp"
#include "log.hpp"
#include "podman_error.hpp"
#include "podman_query.hpp"
#include "podman_socket.hpp"

/*
TODO:

 - check if some of headers are unnecessary

*/

bool Podman::Socket::open(void) {

	struct sockaddr_un server_addr;
	struct timeval tv = {
		.tv_sec = this -> timeout,
		.tv_usec = 0
	};

	this -> error = Podman::Error::NO_ERROR;
	this -> close();

	bzero(&server_addr, sizeof(server_addr));
	server_addr.sun_family = AF_UNIX;
	strncpy(server_addr.sun_path, this -> path.c_str(), this -> path.length());

	this -> fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if ( !this -> fd ) {
		this -> error = Podman::Error::SOCKET_UNAVAILABLE;
		this -> fd = -1;
		return false;
	}

	if ( this -> timeout ) {
		setsockopt(this -> fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval));
		setsockopt(this -> fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(struct timeval));
	}

	if ( connect(this -> fd, (const struct sockaddr*)&server_addr,
		sizeof(server_addr)) < 0 ) {
		this -> error = Podman::Error::SOCKET_NOT_ACCEPTING;
		// set err
		this -> close();
		return false;
	}

	return true;

}

bool Podman::Socket::send(Podman::Query query) {

	if ( this -> fd < 0 )
		return false;

	std::string request = query.request() + query.body;
	const char *ptr = request.c_str();
	int c = request.length(), n = 0;
	int retries = 0;

	while (c) {

		if ( retries >= 2 ) {
			this -> error = Podman::Error::SOCKET_SEND_TIMEOUT;
			this -> error.setExtended("send");
			return false;
		}

		if (( n = ::send(this -> fd, ptr, c, 0)) < 0 ) {
			if ( errno == EINTR ) continue;
			else if ( errno == EAGAIN ) {
				retries++;
				continue;
			} else {
				this -> error = Podman::Error::SOCKET_READONLY;
				return false;
			}
		} else if ( n == 0 ) {
			this -> error = Podman::Error::SOCKET_NOT_RECEIVING;
			return false;
		}

		c -= n;
		ptr += n;
	}

	return true;
}

bool Podman::Socket::read_headers(Podman::Query::Response &response) {

	char buf[1];
	int n = 0;
	std::string hdr = "";

	while ( true ) {

		buf[0] = 0;
		n = ::recv(this -> fd, buf, 1, 0);

		if ( n < 0 && errno == EINTR ) continue;
		else if ( n < 0 && errno == EAGAIN ) {
			this -> error = Podman::Error::SOCKET_READ_TIMEOUT;
			this -> error.setExtended("protocol");
			return false;
		} else if ( n < 0 ) {
			this -> error = Podman::Error::SOCKET_STREAM_UNAVAILABLE;
			this -> error.setExtended("protocol");
			return false;
		} else if ( n == 0 && hdr.empty()) continue;
		else if ( n == 0 ) {
			this -> error = Podman::Error::SOCKET_CLOSED_EARLY;
			this -> error.setExtended("protocol");
			return false;
		}

		if ( buf[0] == '\r' || buf[0] == 0 ) continue;
		else if ( buf[0] == '\n' && hdr.empty()) continue;
		else if ( buf[0] == '\n' ) break;

		hdr += buf[0];
	}

	if ( common::has_prefix(hdr, "HTTP/")) {

		std::vector<std::string> components = common::split(hdr, ' ');

		if ( components.size() >= 3 && common::is_number(components[1])) {
			response.protocol = components[0];
			response.code = std::stoi(components[1]);
			response.status = components[2];
		} else {
			this -> error = Podman::Error::PROTOCOL_MISMATCH;
			return false;
		}
	} else {
		this -> error = Podman::Error::PROTOCOL_INVALID;
		return false;
	}

	hdr = "";
	std::string val = "";
	bool namepart = true;

	while ( true ) {

		buf[0] = 0;
		n = ::recv(this -> fd, buf, 1, 0);

		if ( n < 0 && errno == EINTR ) continue;
		if ( n < 0 && errno == EAGAIN ) {
			this -> error = Podman::Error::SOCKET_READ_TIMEOUT;
			this -> error.setExtended("header");
			return false;
		} else if ( n < 0 ) {
			this -> error = Podman::Error::SOCKET_STREAM_UNAVAILABLE;
			this -> error.setExtended("header");
			return false;
		} else if ( n == 0 ) {
			this -> error = Podman::Error::SOCKET_CLOSED_EARLY;
			this -> error.setExtended("header");
			return false;
		}

		if ( buf[0] == '\r' || buf[0] == 0 ) continue;
		else if ( namepart && buf[0] == '\n' && hdr.empty()) break;
		else if ( buf[0] == '\n' ) {
			if ( !hdr.empty())
				response.headers.insert(std::pair<std::string, std::string>(common::to_lower(hdr), val));
			namepart = true;
			hdr = "";
			val = "";
			continue;
		} else if ( buf[0] == ':' ) {
			namepart = false;
			val = "";
			continue;
		} else if ( !namepart && buf[0] == ' ' && val.empty()) continue;

		if ( namepart ) hdr += buf[0];
		else val += buf[0];

	}

	/*
	// debug
	std::cout << "protocol: " << response.protocol << " code: " << response.code << " status: " << response.status << std::endl;

	for ( auto it = response.headers.begin(); it != response.headers.end(); it++) {
		std:: cout << it -> first << " = " << it -> second << std::endl;
	}
	*/

	return true;
}

bool Podman::Socket::read_chunksize(int &bytes) {

	char buf[1];
	int n = 0;
	std::string value = "0x";

	bytes = 0;

	while ( true ) {

		buf[0] = 0;
		n = ::recv(this -> fd, buf, 1, 0);

		if ( n < 0 && errno == EAGAIN ) {
			this -> error = Podman::Error::SOCKET_READ_TIMEOUT;
			this -> error.setExtended("chunk-size");
			return false;
		} if ( n < 0 && errno == EINTR ) continue;
		else if ( n < 0 ) {
			this -> error = Podman::Error::SOCKET_STREAM_UNAVAILABLE;
			this -> error.setExtended("chunk-size");
			return false;
		} else if ( n == 0 && value == "0x" ) continue;
		else if ( n == 0 ) {
			this -> error = Podman::Error::SOCKET_CLOSED_EARLY;
			this -> error.setExtended("chunk-size");
			return false;
		}

		if ( buf[0] == '\r' || buf[0] == 0 ) continue;
		else if ( buf[0] == '\n' && value == "0x" ) continue;
		else if ( buf[0] == ' ' && value == "0x" ) continue;
		else if ( buf[0] == '\n' ) break;

		value += buf[0];
	}

	if ( value == "0x0" ) return true;
	if ( !common::is_hex(value)) {
		this -> error = Podman::Error::CHUNK_SIZE_INVALID;
		return false;
	}

	bytes = std::stoul(value, nullptr, 16);
	return true;
}

bool Podman::Socket::read_chunk(std::string &content, int bytes) {

	char buf[bytes == 0 ? this -> buf_size : bytes];
	int n = 0;

	content = "";

	while ( true ) {

		n = ::recv(this -> fd, buf, sizeof(buf), 0);

		if ( n < 0 && errno == EINTR ) continue;
		else if ( n < 0 && errno == EAGAIN ) {
			this -> error = Podman::Error::SOCKET_READ_TIMEOUT;
			return false;
		} else if ( n < 0 ) {
			this -> error = Podman::Error::SOCKET_STREAM_UNAVAILABLE;
			return false;
                } else if ( n == 0 ) {
			this -> error = Podman::Error::SOCKET_CLOSED_EARLY;
                        return false;
                }

		content.append(buf, n);
		break;
	}

	/*
	// debug
	std::cout << "content received, requested size: " << bytes << ", got: " << content.length() << std::endl;
	*/

	if ( bytes > 0 && content.length() < bytes ) {
		this -> error = Podman::Error::CHUNK_SIZE_MISMATCH;
		return false;
	}

	return true;
}

std::string hex_string(int d) {

	std::stringstream ss;
	ss << std::hex << d;
	return ss.str();
}

bool Podman::Socket::read(Podman::Query query, Podman::Query::Response &response) {

	if ( !this -> read_headers(response))
		return false;

	bool chunked = response.headers.count("transfer-encoding") && common::to_lower(response.headers.at("transfer-encoding")) == "chunked" ? true : false;
	int chunksize = 0;
	int chunk_n = 0;
	std::string content;

	if ( !chunked && response.headers.count("content-length")) {
		std::string value = response.headers.at("content-length");
		if ( !common::is_number(value)) {
			this -> error = Podman::Error::CONTENT_SIZE_INVALID;
			return false;
		}
		chunksize = std::stoi(value);
	}

	if ( chunked ) {

		if ( !this -> read_chunksize(chunksize)) {
			this -> error.setExtended("chunk-size #0");
			return false;
		}

		while ( chunksize != 0 ) {

			response.raw += hex_string(chunksize) + "\n";

			if ( !this -> read_chunk(content, chunksize)) {
				if ( this -> error == Podman::Error::CHUNK_SIZE_MISMATCH )
					this -> error.setExtended("expected " + std::to_string(chunksize) + ", got " + std::to_string(content.length()));
				else this -> error.setExtended("chunk #" + std::to_string(chunk_n));
				return false;
			}

			response.raw += content;

			if ( query.chunks_to_array )
				response.body += chunk_n == 0 ? '[' : ',';

			response.body += common::trim_ws(content);

			chunk_n++;
			if ( query.chunks_allowed > 0 && chunk_n >= query.chunks_allowed )
				break;
			else if ( !this -> read_chunksize(chunksize)) {

				this -> error.setExtended("chunk-size #" + std::to_string(chunk_n));
				if ( query.chunks_to_array )
					response.body += ']';

				return false;
			}

		}

		if ( query.chunks_to_array )
			response.body += ']';

	} else { // Single chunk

		if ( !this -> read_chunk(content, chunksize)) {
			if ( this -> error == Podman::Error::CHUNK_SIZE_MISMATCH ) {
				this -> error = Podman::Error::CONTENT_SIZE_MISMATCH;
				this -> error.setExtended("expected " + std::to_string(chunksize) + ", got " + std::to_string(content.length()));
			} else {
				if ( !response.headers.count("content-length"))
					return true; // There was no body..
				this -> error.setExtended("body content");
			}

			return false;
		}

		//std::cout << "Single chunk received. Size requested: " << chunksize << ", received: " << content.length() << std::endl;
		response.raw = content;
		response.body = common::trim_ws(content);
	}

	//std::cout << "response:\n" << response.body << "--END" << std::endl;

	return true;
}

bool Podman::Socket::execute(Podman::Query query, Podman::Query::Response& response) {

	response.reset();
	std::lock_guard<std::mutex> guard(mutex.podman_sock);

	if ( this -> open() && this -> send(query) &&
		this -> read(query, response)) {
			this -> close();
			if ( response.body.length() != 0 )
				if ( query.parseJson && !response.parseJson()) {
					this -> error = Podman::Error::JSON_PARSE_FAILED;
					this -> error.setExtended(response.error_msg);
					response.error_msg = "";
					log::verbose << "failed to call: " << common::trim_leading(query.path()) << std::endl;
					log::vverbose << "error: " << this -> error.description() << std::endl;
					return false;
				}
			return true;
	}

	this -> close();

	log::verbose << "failed to call: " << common::trim_leading(query.path()) << std::endl;
	log::vverbose << "error: " << this -> error.description() << std::endl;
	return false;
}
