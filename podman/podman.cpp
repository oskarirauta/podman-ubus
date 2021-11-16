#include <thread>

#include "log.hpp"
#include "podman.hpp"

void Podman::init(Podman::podman_t *podman) {

	if ( !podman -> update_system())
		log::info << "Failed to fetch podman system info\n" << std::endl;

	if ( !podman -> update_networks())
		log::info << "Failed to fetch podman networks" << std::endl;

	if ( !podman -> update_pods())
		log::info << "Failed to fetch podman pods" << std::endl;

	if ( !podman -> update_containers())
		log::info << "Failed to fetch podman containers" << std::endl;

	if ( !podman -> update_stats())
		log::vverbose << "Failed to fetch podman stats" << std::endl;

	if ( !podman -> update_logs())
		log::vverbose << "Failed to fetch podman logs" << std::endl;

	std::this_thread::sleep_for(std::chrono::milliseconds(500));

	podman -> update_logs();
	podman -> update_stats();
}

Podman::podman_t *podman_data;
Podman::Scheduler *podman_scheduler;
