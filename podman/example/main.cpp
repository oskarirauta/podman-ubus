#include <iostream>
#include <thread>
#include <chrono>
#include <iomanip>

#include "common.hpp"
#include "log.hpp"
#include "mutex.hpp"
#include "podman_query.hpp"
#include "podman_socket.hpp"
#include "podman_error.hpp"
#include "podman_t.hpp"
#include "podman_dump.hpp"
#include "podman_scheduler.hpp"
#include "podman_loop.hpp"

bool shouldExit = false;

const auto starttime = std::chrono::system_clock::now();

uint64_t timestamp(void) {
	const auto p1 = std::chrono::system_clock::now();
	return std::chrono::duration_cast<std::chrono::milliseconds>(p1 - starttime).count();
}

int main() {

	log::output_level[log::verbose] = true;
	log::output_level[log::vverbose] = true;
	log::output_level[log::debug] = true;

	Podman::podman_t podman;

	if ( !podman.update_system())
		std::cout << "Failed to fetch system info\n" << std::endl;
	std::cout << "\nPodman system info:\n" << Podman::dump::system(podman) << std::endl;

	if ( !podman.update_networks())
		std::cout << "Failed to fetch networks\n" << std::endl;
	std::cout << "Networks:\n" << Podman::dump::networks(podman) << std::endl;

	if ( !podman.update_pods())
		std::cout << "Failed to fetch pods\n" << std::endl;
	if ( !podman.update_containers())
		std::cout << "Failed to fetch containers\n" << std::endl;

	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	if ( !podman.update_logs())
		std::cout << "Failed to fetch logs\n" << std::endl;

	if ( !podman.update_stats())
		std::cout << "Failed to update container stats\n" << std::endl;

	std::cout << "Pods:\n" << Podman::dump::pods(podman) << std::endl;

	Podman::Scheduler scheduler(&podman);

	while ( !shouldExit ) {

		std::string line;
		for ( int p = 0; p < podman.pods.size(); p++ )
			for ( int c = 0; c < podman.pods[p].containers.size(); c++ )
				line += ( line.empty() ? "" : " " ) + podman.pods[p].containers[c].name + " " + podman.pods[p].containers[c].cpu.text;

		if ( scheduler.nextTask() == Podman::Scheduler::Task::UPDATE_CONTAINERS )
			std::cout << line << std::endl;

		scheduler.run();
		std::this_thread::sleep_for(std::chrono::milliseconds(300));
	};

	return 0;
}
