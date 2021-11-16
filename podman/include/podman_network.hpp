#pragma once

#include <string>
#include <vector>

namespace Podman {

	class Network {

		public:

			typedef struct {
				std::string gateway;
				std::string subnet;
			} Range;

			typedef struct {
				std::vector<Podman::Network::Range> ranges;
				std::vector<std::string> routes;
				std::string type;
			} Ipam;

			struct find_name: std::unary_function<Podman::Network, bool> {
				std::string name;
				find_name(std::string name):name(name) { }
				bool operator()(Podman::Network const &m) const {
					return m.name == name;
				}
			};

			std::string name;
			std::string version;
			std::string plugins;
			std::string type;
			bool isGateway;

			Podman::Network::Ipam ipam;
			Network(std::string name, std::string version);

	};

}
