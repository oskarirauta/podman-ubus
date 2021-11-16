#include <string>
#include <vector>
#include <algorithm>

#include "common.hpp"
#include "log.hpp"
#include "mutex.hpp"
#include "podman_t.hpp"
#include "podman_network.hpp"

Podman::Network::Network(std::string name, std::string version) {

	this -> name = name;
	this -> version = version;
	this -> plugins = "";
	this -> type = "";
	this -> isGateway = false;
	this -> ipam = Podman::Network::Ipam();
}

bool Podman::podman_t::update_networks(void) {

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

		if (( !response.json[i]["Name"].isString()) ||
			( !response.json[i]["CNIVersion"].isString()))
				continue;

		Podman::Network network(response.json[i]["Name"].asString(),
			response.json[i]["CNIVersion"].asString());

		network.type = response.json[i]["Plugins"][0]["Network"]["type"].asString();
		network.ipam.type = response.json[i]["Plugins"][0]["Network"]["ipam"]["type"].asString();
		network.plugins = "";

		if (( !network.name.empty()) && ( !network.version.empty()))
			networks.push_back(network);
	}

	for ( int i = 0; i < networks.size(); i++ ) {

		Podman::Query::Response response2;
		Podman::Query query2 = { .group = "networks", .id = networks[i].name, .action = "json" };

		if ( !socket.execute(query2, response2))
			continue;

		if ( response2.json["plugins"][0]["ipam"]["ranges"].isArray()) {
		
			std::vector<Podman::Network::Range> ranges;

			for ( int i2 = 0; i2 < response2.json["plugins"][0]["ipam"]["ranges"].size(); i2++ ) {

				Podman::Network::Range range;

				range.gateway = response2.json["plugins"][0]["ipam"]["ranges"][i2][0]["gateway"].asString();
				range.subnet = response2.json["plugins"][0]["ipam"]["ranges"][i2][0]["subnet"].asString();

				if ( range.gateway.empty() && range.subnet.empty())
					continue;

				ranges.push_back(range);
			}

			networks[i].ipam.ranges = ranges;
		}

		if ( response2.json["plugins"][0]["ipam"]["routes"].isArray()) {

			std::vector<std::string> routes;

			for ( int i2 = 0; i2 < response2.json["plugins"][0]["ipam"]["routes"].size(); i2++ ) {

				std::string route = response2.json["plugins"][0]["ipam"]["routes"][i2]["dst"].asString();
				if ( !route.empty())
					routes.push_back(route);				
			}

			networks[i].ipam.routes = routes;
		}

		networks[i].isGateway = response2.json["plugins"][0]["isGateway"].asBool();

		if ( response2.json["plugins"].size() > 1 )
			for ( int i2 = 1; i2 < response2.json["plugins"].size(); i2++ ) {
				std::string plugin = response2.json["plugins"][i2]["type"].asString();
				if ( plugin.empty()) continue;
				networks[i].plugins += ( i2 != 1 ? "," : "" ) + plugin;
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
