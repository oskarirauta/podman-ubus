#include <string>
#include <vector>
#include <algorithm>

#include "common.hpp"
#include "log.hpp"
#include "podman_t.hpp"
#include "podman_pod.hpp"

Podman::Pod::Pod(const std::string id, const std::string name) {

	this -> id = id;
	this -> name = name;
	this -> status = "unknown";
	this -> isRunning = false;
	this -> infraId = "";
	this -> containers = std::vector<Podman::Container>();
}

const bool Podman::podman_t::update_pods(void) {

	Podman::Query::Response response;
	Podman::Query query = { .group = "pods", .action = "json" };

	if ( !socket.execute(query, response))
		return false;

	uint64_t hashValue = response.hash();

	if ( hashValue == this -> hash.pods ) {
		mutex.podman.lock();
		this -> state.pods == Podman::Node::OK;
		mutex.podman.unlock();
		return true;
	}

	if ( !response.json.isArray()) {
		log::verbose << "failed to call: " << common::trim_leading(query.path()) << std::endl;
		log::vverbose << "error: json result is not array" << std::endl;
		return false;
	}

	std::vector<Podman::Pod> pods;

	for ( int i = 0; i < response.json.size(); i++ ) {

		if (( !response.json[i]["Name"].isString()) ||
			( !response.json[i]["Id"].isString()) ||
			( !response.json[i]["Created"].isString()))
				continue;

		Podman::Pod pod(response.json[i]["Id"].asString(),
			response.json[i]["Name"].asString());

		if ( response.json[i]["Status"].isString()) {
			pod.status = response.json[i]["Status"].asString();
			if ( pod.status.empty()) pod.status = "unknown";
			pod.isRunning = ( common::to_lower(pod.status) == "running" || common::to_lower(pod.status) == "degraded" ) ? true : false;
		} else {
			pod.status = "unknown";
			pod.isRunning = false;
		}

		if ( response.json[i]["InfraId"].isString())
			pod.infraId = response.json[i]["InfraId"].asString();
		else pod.infraId = "";

		pods.push_back(pod);
	}

	mutex.podman.lock();
	this -> pods = pods;
	this -> hash.pods = pods.empty() ? 0 : hashValue;
	this -> state.pods = Podman::Node::OK;

	if ( !this -> state.containers == Podman::Node::INCOMPLETE )
		this -> state.containers = Podman::Node::NEEDS_UPDATE;

	mutex.podman.unlock();

	if ( pods.empty())
		log::debug << "pods array was empty" << std::endl;

	return true;
}

const int Podman::podman_t::podIndex(const std::string name) {

	std::lock_guard<std::mutex> guard(mutex.podman);

	for ( int i = 0; i < this -> pods.size(); i++ )
		if ( this -> pods[i].name == name )
			return i;

	return -1;
}
