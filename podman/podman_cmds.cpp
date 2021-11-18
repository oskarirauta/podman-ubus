#include <string>

#include "podman_busystat.hpp"
#include "podman_query.hpp"
#include "podman_socket.hpp"
#include "podman_t.hpp"

const bool Podman::podman_t::container_stop(std::string name) {

	Podman::Query::Response response;
	Podman::Query query = { 
		.method = "POST",
		.group = "containers",
		.id = name,
		.action = "stop",
		.parseJson = false
	};

	if ( !socket.execute(query, response))
		return false;

	return response.code == 204 ? true : false;
}

const bool Podman::podman_t::container_start(std::string name) {

	Podman::Query::Response response;
	Podman::Query query = {
		.method = "POST",
		.group = "containers",
		.id = name,
		.action = "start",
		.parseJson = false
	};

	if ( !socket.execute(query, response))
		return false;

	return response.code == 204 ? true : false;
}

const bool Podman::podman_t::container_restart(std::string name) {

	Podman::Query::Response response;
	Podman::Query query = {
		.method = "POST",
		.group = "containers",
		.id = name,
		.action = "restart",
		.parseJson = false
	};

	if ( !socket.execute(query, response))
		return false;

	return response.code == 204 ? true : false;
}


const bool Podman::podman_t::pod_stop(std::string name) {

	Podman::Query::Response response;
	Podman::Query query = {
		.method = "POST",
		.group = "pods",
		.id = name,
		.action = "stop",
		.parseJson = false
	};

	if ( !socket.execute(query, response))
		return false;

	return response.code == 200 ? true : false;
}

const bool Podman::podman_t::pod_start(std::string name) {

	Podman::Query::Response response;
	Podman::Query query = {
		.method = "POST",
		.group = "pods",
		.id = name,
		.action = "start",
		.parseJson = false
	};

	if ( !socket.execute(query, response))
		return false;

	return response.code == 200 ? true : false;
}

const bool Podman::podman_t::pod_restart(std::string name) {

	Podman::Query::Response response;
	Podman::Query query = {
		.method = "POST",
		.group = "pods",
		.id = name,
		.action = "restart",
		.parseJson = false
	};

	if ( !socket.execute(query, response))
		return false;


	return response.code == 200 ? true : false;
}
