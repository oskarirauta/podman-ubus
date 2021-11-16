#pragma once

#include <string>
#include <vector>

#include "podman_constants.hpp"
#include "podman_socket.hpp"
#include "podman_query.hpp"
#include "podman_network.hpp"
#include "podman_pod.hpp"
#include "podman_node.hpp"

namespace Podman {

	enum PODMAN_STATUS {
		UNAVAILABLE = 0,
		RUNNING,
		UNKNOWN
	};

	class podman_t {

		private:
			bool needsSystemUpdate;

		public:

			typedef struct {
				uint64_t networks = 0;
				uint64_t pods = 0;
				uint64_t containers = 0;
				uint64_t stats = 0;
			} Hashes;

			typedef struct {
				Podman::Node::State networks = Podman::Node::INCOMPLETE;
				Podman::Node::State pods = Podman::Node::INCOMPLETE;
				Podman::Node::State containers = Podman::Node::INCOMPLETE;
				Podman::Node::State stats = Podman::Node::INCOMPLETE;
			} States;

			Podman::Socket socket;
			PODMAN_STATUS status = UNKNOWN;

			std::string arch;
			std::string os, os_version;
			std::string hostname;
			std::string kernel;
			std::string memTotal, memFree;
			std::string swapTotal, swapFree;
			std::string conmon, ociRuntime;
			std::string version, api_version;

			podman_t(std::string socket_path = Podman::API_SOCKET);
			podman_t(void (*creator_func)(Podman::podman_t*), std::string socket_path = Podman::API_SOCKET);
			//~podman_t();

			std::vector<Podman::Network> networks;
			std::vector<Podman::Pod> pods;

			Podman::podman_t::Hashes hash;
			Podman::podman_t::States state;

			/* --- Calls --- */

			// System
			bool update_system(void);

			// Network
			bool update_networks(void);

			// Pods
			bool update_pods(void);
			int podIndex(std::string name);

			// Containers
			bool update_containers(void);
			bool update_stats(void);
			bool update_logs(void);

			// Commands: containers
			bool container_stop(std::string name);
			bool container_start(std::string name);
			bool container_restart(std::string name);

			// Commands: pods
			bool pod_stop(std::string name);
			bool pod_start(std::string name);
			bool pod_restart(std::string name);

	};

}
