#pragma once

#include <unistd.h>
#include "curl/curl.h"
#include "podman_error.hpp"
#include "podman_constants.hpp"
#include "podman_query.hpp"

namespace Podman {

	struct Socket {

		private:

			CURL *curl;

		public:

			int timeout = 10;
			Podman::Error error = Podman::Error::NO_ERROR;
			std::string path = Podman::API_SOCKET;

			Socket() : curl(curl_easy_init()) {}
			inline ~Socket() { curl_easy_cleanup(this -> curl); }

			bool execute(Podman::Query query, Podman::Query::Response& response);

	};

}
