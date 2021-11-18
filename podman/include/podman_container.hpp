#pragma once

#ifndef CONTAINER_LOG_SIZE
#define CONTAINER_LOG_SIZE 25
#endif

#include <string>
#include <vector>

#include "podman_socket.hpp"

namespace Podman {

	class Container {

		public:

			enum BusyState : uint8_t {

				none = 0,
				starting,
				stopping,
				restarting

			};

			struct BusyStats {

				bool busy = false;
				unsigned char countdown = 0; // cycles to wait until releasing busy status even if no changes in container
				Podman::Container::BusyState state = Podman::Container::BusyState::none;				

			};

			struct MemoryStats {
				double used = 0, max = 0, free = 0, percent = 0;
			};

			struct CpuStats {
				double percent = 0;
				std::string text = "--";
			};

			struct find_id: std::unary_function<Podman::Container, bool> {
				std::string id;
				find_id(std::string id):id(id) { }
				bool operator()(Podman::Container const &m) const {
					return m.id == id;
				}
			};

			struct find_name: std::unary_function<Podman::Container, bool> {
				std::string name;
				find_name(std::string name):name(name) { }
				bool operator()(Podman::Container const &m) const {
					return m.name == name;
				}
			};

			std::string id;
			std::string name;
			std::string image;
			std::string command;
			std::string pod;
			bool isInfra = false;
			bool isRunning = false;
			pid_t pid;
			std::string state;
			std::string status;
			std::time_t startedAt;
			std::time_t uptime;
			std::vector<pid_t> pids;

			std::vector<std::string> logs;
			Podman::Container::CpuStats cpu;
			Podman::Container::MemoryStats ram;

			Container(std::string name, std::string id = "");

			bool update_logs(Podman::Socket *socket);
	};

}
