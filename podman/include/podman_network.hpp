#pragma once

#include <string>
#include <vector>

namespace Podman {

	class Network {

		public:

			struct Range {
				std::string gateway;
				std::string subnet;
			};

			struct Ipam {
				std::vector<Podman::Network::Range> ranges;
				std::string type;
			};

			struct find_name: std::unary_function<Podman::Network, bool> {
				std::string name;
				find_name(std::string name):name(name) { }
				bool operator()(Podman::Network const &m) const {
					return m.name == name;
				}
			};

			std::string name;
			std::string type;

			Podman::Network::Ipam ipam;
			Network(std::string name);

	};

}
