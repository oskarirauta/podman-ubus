#include <string>
#include <vector>
#include <json/json.h>

#include "common.hpp"
#include "constants.hpp"
#include "podman_network.hpp"
#include "podman_query.hpp"
#include "podman_socket.hpp"
#include "podman_validate.hpp"
#include "log.hpp"
#include "mutex.hpp"
#include "podman_t.hpp"

Podman::podman_t::podman_t(std::string socket_path) {

	this -> needsSystemUpdate = true;
	this -> status = UNKNOWN;
	this -> version = "";
	this -> networks = std::vector<Podman::Network>();
	this -> pods = std::vector<Podman::Pod>();

	this -> state.networks = Podman::Node::INCOMPLETE;
	this -> state.pods = Podman::Node::INCOMPLETE;
	this -> state.containers = Podman::Node::INCOMPLETE;

	this -> hash.networks = 0;
	this -> hash.pods = 0;
	this -> hash.containers = 0;

	socket.timeout = Podman::API_TIMEOUT;
	if ( socket_path != Podman::API_SOCKET )
		this -> socket.path = socket_path;

}

Podman::podman_t::podman_t(void (*creator_func)(Podman::podman_t*), std::string socket_path) {

	this -> needsSystemUpdate = true;
	this -> status = UNKNOWN;
	this -> version = "";
	this -> networks = std::vector<Podman::Network>();
	this -> pods = std::vector<Podman::Pod>();

	this -> state.networks = Podman::Node::INCOMPLETE;
	this -> state.pods = Podman::Node::INCOMPLETE;
	this -> state.containers = Podman::Node::INCOMPLETE;

	this -> hash.networks = 0;
	this -> hash.pods = 0;
	this -> hash.containers = 0;

	socket.timeout = Podman::API_TIMEOUT;
	if ( socket_path != Podman::API_SOCKET )
		this -> socket.path = socket_path;

	creator_func(this);
}

const bool Podman::podman_t::update_system(void) {

	Podman::Query::Response response;
	Podman::Query query = { .action = "info" };

	if ( !socket.execute(query, response))
		return false;

	mutex.podman.lock();

	this -> needsSystemUpdate = false;

	if ( this -> status != RUNNING ) {

		if ( !Podman::verifyJsonElements(response.json, { "host", "version" }, &query))
			return false;

		this -> arch = response.json["host"]["arch"].asString();

		std::string conmon = response.json["host"]["conmon"]["version"].asString();
		conmon = common::lines(conmon)[0];
		std::string oci = response.json["host"]["ociRuntime"]["version"].asString();

		this -> conmon = common::lines(conmon, ',')[0];
		this -> os = common::trim_ws(response.json["host"]["distribution"]["distribution"].asString() +
				" " + response.json["host"]["os"].asString());
		this -> os_version = response.json["host"]["distribution"]["version"].asString();
		this -> hostname = response.json["host"]["hostname"].asString();
		this -> kernel = response.json["host"]["kernel"].asString();
		this -> memTotal = common::memToStr(response.json["host"]["memTotal"].asDouble());
		this -> swapTotal = common::memToStr(response.json["host"]["swapTotal"].asDouble());
		this -> ociRuntime = oci;
		this -> ociRuntime = common::lines(oci)[0];
		this -> api_version = response.json["version"]["APIVersion"].asString();
		this -> version = response.json["version"]["Version"].asString();

		this -> status = RUNNING;

	} else if ( !Podman::verifyJsonElements(response.json, { "host" }, &query)) {

		mutex.podman.unlock();
		log::verbose << "failed to call " << query.path() << std::endl;
		log::vverbose << "error: json element mismatch" << std::endl;
		return false;
	}

	this -> memFree = common::memToStr(response.json["host"]["memFree"].asDouble());
	this -> swapFree = common::memToStr(response.json["host"]["swapFree"].asDouble());

	mutex.podman.unlock();

	return true;
}

const std::string Podman::podman_t::containerNameForID(const std::string id) {

	if ( id.empty())
		return "";

	bool done = false;
	std::string name = "";

	mutex.podman.lock();

	for ( const auto& pod : this -> pods ) {

		for ( const auto& cntr : pod.containers ) {

			if ( common::to_lower(cntr.id) == common::to_lower(id)) {
				name = cntr.name;
				done = true;
				break;
			}
		}

		if ( done )
			break;
	}

	mutex.podman.unlock();
	return name;
}

const bool Podman::podman_t::setContainerBusyState(const std::string name, Podman::BusyStat::Value state) {

	if ( name.empty())
		return false;

	bool done = false;

	mutex.podman.lock();
	for ( int podI = 0; podI < this -> pods.size(); podI++ ) {

		for ( int idx = 0; idx < this -> pods[podI].containers.size(); idx++ ) {

			if ( this -> pods[podI].containers[idx].name == name ) {
				this -> pods[podI].containers[idx].busyState = state;
				// log::debug << APP_NAME << ": container " << this -> pods[podI].containers[idx].name << " state is now: ";
				// log::debug << this -> pods[podI].containers[idx].busyState.description() << std::endl;
				done = true;
				break;
			}
		}

		if ( done )
			break;
	}

	mutex.podman.unlock();
	return done;
}

const bool Podman::podman_t::resetContainerBusyState(const std::string name) {

	if ( name.empty())
		return false;

	bool done = false;

	mutex.podman.lock();
	for ( int podI = 0; podI < this -> pods.size(); podI++ ) {

		for ( int idx = 0; idx < this -> pods[podI].containers.size(); idx++ ) {

			if ( this -> pods[podI].containers[idx].name == name ) {
				this -> pods[podI].containers[idx].busyState.reset();
				done = true;
				break;
			}
		}

		if ( done )
			break;

	}

	mutex.podman.unlock();
	return done;
}
