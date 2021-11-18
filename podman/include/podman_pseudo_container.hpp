#pragma once

#include <string>
#include <vector>

#include "podman_container.hpp"
#include "podman_busystat.hpp"

namespace Podman {

	namespace Pseudo {

		class Container {

			public:

				std::string id;
				std::string name;

				bool isRunning;
				std::string state;
				std::string status;

				std::vector<std::string> logs;
				Podman::Container::CpuStats cpu;
				Podman::Container::MemoryStats ram;
				Podman::BusyStat busyState;

				Container(Podman::Container cntr) {

					this -> id = cntr.id;
					this -> name = cntr.name;
					this -> isRunning = cntr.isRunning;
					this -> state = cntr.state;
					this -> status = cntr.status;
					this -> logs = cntr.logs;
					this -> cpu = cntr.cpu;
					this -> ram = cntr.ram;
					this -> busyState = cntr.busyState;
				}

				inline bool stateChanged(const Podman::Container cntr) const {

					return ( this -> isRunning != cntr.isRunning ||
						this -> state != cntr.state ||
						this -> status != cntr.status ) ? true : false;
				}

		};
	}

}
