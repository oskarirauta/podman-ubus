#pragma once

#include <unistd.h>
#include "podman_error.hpp"
#include "podman_constants.hpp"
#include "podman_query.hpp"

namespace Podman {

	struct Socket {

		private:

			bool open(void);
			bool send(Podman::Query query);
			bool read_headers(Podman::Query::Response &response);
			bool read_chunksize(int &bytes);
			bool read_chunk(std::string &content, int bytes);
			bool read(Podman::Query query, Podman::Query::Response &response);

			inline bool close(void) { // true if socket was open, no error

				if ( this -> fd < 0 ) return false;
				::close(this -> fd);
				this -> fd = -1;
				return true;
			}

		public:

			int timeout = 10;
			int buf_size = 1024;
			Podman::Error error = Podman::Error::NO_ERROR;
			int fd = -1;
			std::string path = Podman::API_SOCKET;

			inline ~Socket() {
				this -> close();
			}

			bool execute(Podman::Query query, Podman::Query::Response& response);

	};

}
