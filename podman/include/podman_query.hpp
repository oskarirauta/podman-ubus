#pragma once

#include <string>
#include <map>
#include <json/json.h>

#include "common.hpp"
#include "podman_constants.hpp"

namespace Podman {

	struct Query {

			std::string method = "";
			std::string group = "";
			std::string id = "";
			std::string action = "";
			std::string query = "";
			std::string mime = "";
			std::string body = "";

			int chunks_allowed = 0;
			bool chunks_to_array = false;
			bool parseJson = true;

			inline const std::string path(void) {

				return (
					( this -> group.empty() ? "" : ( "/" + this -> group )) +
					( this -> id.empty() ? "" : ( "/" + this -> id )) +
					( this -> action.empty() ? "" : ( "/" + this -> action )) +
					( this -> query.empty() ? "" : ( "?" + this -> query ))
				);
			}

			inline const std::string request(void) {

				std::string path = this -> path();
				if ( path.empty()) return "";

				std::string header = ( this -> method.empty() ? "GET": this -> method ) + " ";
				header += Podman::API_VERSION.empty() ? "" : ( "/" + Podman::API_VERSION );
				header += Podman::API_PATH.empty() ? "" : ( "/" + Podman::API_PATH );
				header += path + " " + ( Podman::API_PROTOCOL.empty() ? "HTTP/1.1" : Podman::API_PROTOCOL ) + "\r\n";

				header += "Host: " + ( Podman::API_HOST.empty() ? "localhost" : Podman::API_HOST ) + "\r\n";
				header += "User-Agent: " + ( Podman::API_USERAGENT.empty() ? "podmancli" : Podman::API_USERAGENT ) + "\r\n";
				header += "Connection: close\r\n";

				if ( !this -> body.empty()) {
					if ( !this -> mime.empty())
						header += "Content-Type: " + this -> mime + "\r\n";
					header += "Content-Length: " + std::to_string(this -> body.length()) + "\r\n";
				}

				return header + "\r\n";
			}

		struct Response {

			std::string error_msg = "";
			int code = -1;

			std::string protocol = "";
			std::string status = "";
			std::map<std::string, std::string> headers;
			std::string body = "";
			std::string raw = "";
			Json::Value json;

			bool parseJson(void);

			inline uint64_t hash(void) {
				uint64_t val = 0;
				for ( const char &c: this -> body)
					val += c;
				return val;
				//return common::hash(this -> body.c_str());
			}

			inline std::string jsonDump(void) {
				if ( this -> body.empty())
					return "";
				Json::StreamWriterBuilder builder;
				const std::string dump = Json::writeString(builder, this -> json);
				return dump;
			}

			inline void reset(void) {

				this -> error_msg = "";
				this -> code = -1;

				this -> protocol = "";
				this -> status = "";
				this -> headers.clear();
				this -> body = "";
				this -> raw = "";
			}

		};
	};

}
