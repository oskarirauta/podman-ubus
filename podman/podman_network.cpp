#include <string>
#include <vector>
#include <algorithm>

#include "common.hpp"
#include "log.hpp"
#include "mutex.hpp"
#include "podman_t.hpp"
#include "podman_network.hpp"

Podman::Network::Network(std::string name) {

	this -> name = name;
	this -> type = "";
	this -> ipam = Podman::Network::Ipam();
}

const bool Podman::podman_t::update_networks(void) {

	Podman::Query::Response response;
	Podman::Query query = { .group = "networks", .action = "json" };

	if ( !socket.execute(query, response))
		return false;

	uint64_t hashValue = response.hash();

	// Simple comparison only against names, cni versions, and types. Subnets, ranges, etc.
	// are not retrieved by this query and are not part of comparison.

	mutex.podman.lock();

	if ( hashValue == this -> hash.networks ) {
		mutex.podman.unlock();
		return true;
	}

	mutex.podman.unlock();

	if ( !response.json.isArray()) {
		log::verbose << "failed to call: " << common::trim_leading(query.path()) << std::endl;
		log::vverbose << "error: json result is not array" << std::endl;
		return false;
	}

	std::vector<Podman::Network> networks;

	for ( int i = 0; i < response.json.size(); i++ ) {

		if ( !response.json[i]["name"].isString()) continue;

		Podman::Network network(response.json[i]["name"].asString());

		network.type = response.json[i]["driver"].asString();
		network.ipam.type = response.json[i]["ipam_options"]["driver"].asString();

		if ( !network.name.empty())
			networks.push_back(network);
	}

	for ( int i = 0; i < networks.size(); i++ ) {

		Podman::Query::Response response2;
		Podman::Query query2 = { .group = "networks", .id = networks[i].name, .action = "json" };

		if ( !socket.execute(query2, response2))
			continue;

		if ( response2.json["subnets"].isArray()) {

			std::vector<Podman::Network::Range> ranges;

			for ( int i2 = 0; i2 < response2.json["subnets"].size(); i2++ ) {

				Podman::Network::Range range;

				range.gateway = response2.json["subnets"][i2]["gateway"].asString();
				range.subnet = response2.json["subnets"][i2]["subnet"].asString();

				if ( range.gateway.empty() && range.subnet.empty())
					continue;

				ranges.push_back(range);
			}

			networks[i].ipam.ranges = ranges;
		}

	}

	if ( networks.empty())
		return false;

	mutex.podman.lock();
	this -> networks = networks;
	this -> hash.networks = hashValue;
	this -> state.networks = Podman::Node::OK;
	mutex.podman.unlock();

	return true;
}
