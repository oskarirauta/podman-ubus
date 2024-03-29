#include <string>
#include <vector>
#include <algorithm>
#include <chrono>

#include "common.hpp"
#include "log.hpp"
#include "mutex.hpp"
#include "constants.hpp"
#include "podman_t.hpp"
#include "podman_node.hpp"
#include "podman_query.hpp"
#include "podman_pseudo_container.hpp"
#include "podman_container.hpp"

const bool get_firstline(const std::string s, std::string &line) {

	std::stringstream ss(s);
	std::string _line;

	if ( !std::getline(ss, _line))
		return false;

	line = _line;
	return true;
}

Podman::Container::Container(std::string name, std::string id) {

	this -> name = name;
	this -> id = id;
	this -> image = "";
	this -> pod = "";
	this -> isInfra = false;
	this -> isRunning = false;
	this -> isRestarting = false;
	this -> pid = -1;
	this -> state = "";
	this -> startedAt = 0;
	this -> uptime = 0;
	this -> pids = std::vector<pid_t>();
	this -> logs = std::vector<std::string>();
	this -> cpu = Podman::Container::CpuStats();
	this -> ram = Podman::Container::MemoryStats();
}

const bool Podman::podman_t::update_containers(void) {

	this -> update_pods();

	mutex.podman.lock();

	if (( this -> state.containers != Podman::Node::INCOMPLETE &&
			this -> state.containers != Podman::Node::NEEDS_UPDATE ) || (
			this -> state.pods == Podman::Node::INCOMPLETE ||
			this -> state.pods == Podman::Node::NEEDS_UPDATE )) {
		mutex.podman.unlock();
		log::debug << "did not update containers, states says no need to" << std::endl;
		return false;
	}

	mutex.podman.unlock();

	Podman::Query::Response response;
	Podman::Query query = { .group = "containers", .action = "json", .query = "all=true" };

	if ( !socket.execute(query, response)) {
		this -> state.containers = Podman::Node::INCOMPLETE;
		log::debug << "update containers failed, containers array state set to incomplete" << std::endl;
		return false;
	}

	uint64_t hashValue = response.hash();

	mutex.podman.lock();

	if ( hashValue == this -> hash.containers ) {
		this -> state.pods = Podman::Node::OK;
		this -> state.containers = Podman::Node::OK;
		mutex.podman.unlock();
		log::debug << "container state hash remained the same, success, but container array not updated" << std::endl;
		return true;
	}

	if ( !response.json.isArray()) {
		mutex.podman.unlock();
		log::verbose << "failed to call: " << common::trim_leading(query.path()) << std::endl;
		log::vverbose << "error: json result is not array" << std::endl;
		return false;
	}

	std::vector<Podman::Pseudo::Container> cntrs;
	for ( const auto& pod: this -> pods )
		for ( const auto& cntr : pod.containers )
			cntrs.push_back(Podman::Pseudo::Container(cntr));

	mutex.podman.unlock();

	std::vector<Podman::Container> containers;

	for ( int i = 0; i < response.json.size(); i++ ) {

		if ( !response.json[i]["Names"].isArray() ||
			!response.json[i]["Names"][0].isString())
			continue;

		Podman::Container container(response.json[i]["Names"][0].asString());

		if ( response.json[i]["Image"].isString()) {
			container.image = response.json[i]["Image"].asString();
			if ( container.image.empty()) container.image = "unknown";
		} else container.image = "unknown";

		if ( response.json[i]["Command"].isArray()) {

			container.command = "";

			for ( int i2 = 0; i2 < response.json[i]["Command"].size(); i2++ ) {

				if ( !response.json[i]["Command"][i2].isString())
					continue;
				container.command += ( i2 == 0 ? "" : " " ) +
					response.json[i]["Command"][i2].asString();
			}

		}

		container.id = response.json[i]["Id"].isString() ?
			response.json[i]["Id"].asString() : "";

		container.pod = response.json[i]["PodName"].isString() ?
			response.json[i]["PodName"].asString() : "";

		container.isInfra = response.json[i]["IsInfra"].isBool() ?
			response.json[i]["IsInfra"].asBool() : false;

		container.pid = response.json[i]["Pid"].isInt() ?
			response.json[i]["Pid"].asInt() : -1;

		container.state = response.json[i]["State"].isString() ?
			response.json[i]["State"].asString() : "";

		container.isRunning = common::to_lower(container.state) == "running" ? true : false;

		if (( response.json[i]["Paused"].isBool() ?
			response.json[i]["Paused"].asBool() : false)) {
			container.isRunning = false;
			container.state = "paused";
		}

		container.isRestarting = response.json[i]["Restarting"].isBool() ?
			response.json[i]["Restarting"].asBool() : false;

		container.startedAt = response.json[i]["StartedAt"].isUInt64() ?
			response.json[i]["StartedAt"].asUInt64() : 0;

		time_t now = std::chrono::duration_cast<std::chrono::seconds>
			(std::chrono::system_clock::now().time_since_epoch()).count();

		container.uptime = ( now - container.startedAt < 0 ) ? 0 : ( now - container.startedAt );

		bool retained = false;
		for ( auto& cntr : cntrs ) // retain previous internal container data
			if ( cntr.id == container.id || cntr.name == container.name ) {

				container.logs = cntr.logs;
				container.ram = cntr.ram;
				container.cpu = cntr.cpu;
				container.busyState = container.isRestarting ? Podman::BusyStat::Value::RESTARTING : cntr.busyState;

				if ( cntr.stateChanged(container))
					container.busyState.reset();

				retained = true;
			}

		if ( !retained )
			log::debug << "failed to retain stats for container id " << container.id << "(" << container.name << ") - container is new?" << std::endl;

		containers.push_back(container);
	}

	log::debug << "previous container count was " << cntrs.size() << std::endl;

	if ( containers.empty())
		log::vverbose << "container array is empty" << std::endl;

	//if ( containers.empty())
	//	return false;

	Podman::Pod noname("", "");

	for ( int i = 0; i < containers.size(); i++ ) {

		int ind = this -> podIndex(containers[i].pod);
		if ( ind == -1 && !containers[i].pod.empty())
			continue;

		if ( ind == -1 ) noname.containers.push_back(containers[i]);
		else {
			mutex.podman.lock();
			this -> pods[ind].containers.push_back(containers[i]);
			mutex.podman.unlock();
		}
	}

	mutex.podman.lock();

	if ( !noname.containers.empty())
		this -> pods.push_back(noname);

	this -> hash.containers = hashValue;
	this -> state.pods = Podman::Node::OK;
	this -> state.containers = Podman::Node::OK;
	mutex.podman.unlock();

	return true;
}

const bool Podman::podman_t::update_stats(void) {

	Podman::Query::Response response;
	Podman::Query query = { .group = "containers", .action = "stats", .query = "interval=2",
			.chunks_allowed = 2, .chunks_to_array = true };

	if ( this -> state.containers != Podman::Node::OK ) {
		log::vverbose << "error: failed to update stats, container array not ready" << std::endl;
		return false;
	}

	if ( !this -> socket.execute(query, response)) {
		log::verbose << "error: socket connection failure" << std::endl;
		return false;
	}

	if ( !response.json[1]["Stats"].isArray()) {
		log::verbose << "failed to call: " << common::trim_leading(query.path()) << std::endl;
		log::vverbose << "error: json result is not array" << std::endl;
		return false;
	}

	mutex.podman.lock();
	std::vector<Podman::Pod> pods = this -> pods;
	mutex.podman.unlock();

	for ( int i = 0; i < response.json[1]["Stats"].size(); i++ ) {

		if ( !response.json[1]["Stats"][i]["Name"].isString() ||
			!response.json[1]["Stats"][i]["CPU"].isDouble() ||
			!response.json[1]["Stats"][i]["MemLimit"].isDouble() ||
			!response.json[1]["Stats"][i]["MemPerc"].isDouble() ||
			!response.json[1]["Stats"][i]["MemUsage"].isDouble())
				continue;

		int pod = -1, idx = -1;
		for ( int i2 = 0; i2 < pods.size(); i2++ )
			for ( int i3 = 0; i3 < pods[i2].containers.size(); i3++ ) {
				if ( pods[i2].containers[i3].name == response.json[1]["Stats"][i]["Name"].asString()) {
					pod = i2;
					idx = i3;
					break;
				}
			}
		if ( !pods[pod].containers[idx].isRunning ) {

			pods[pod].containers[idx].cpu.percent = 0;
			pods[pod].containers[idx].cpu.text = "";

			pods[pod].containers[idx].ram.used = 0;
			pods[pod].containers[idx].ram.max = 0;
			pods[pod].containers[idx].ram.free = 0;
			pods[pod].containers[idx].ram.percent = 0;

			pods[pod].containers[idx].pids = { -1 };
			pods[pod].containers[idx].pid = -1;
			continue;
		}

		if ( pod == -1 || idx == -1 )
			continue;

		double r_usage = response.json[1]["Stats"][i]["CPU"].asFloat() * 10.0;
		int i_usage = (int)r_usage;
		double f_usage = (double)i_usage * 0.1;
		pods[pod].containers[idx].cpu.percent = f_usage;
		pods[pod].containers[idx].cpu.text = std::to_string((int)f_usage) + "%";

		pods[pod].containers[idx].ram.used = response.json[1]["Stats"][i]["MemUsage"].asDouble();
		pods[pod].containers[idx].ram.max = response.json[1]["Stats"][i]["MemLimit"].asDouble();
		pods[pod].containers[idx].ram.free = pods[pod].containers[idx].ram.max - pods[pod].containers[idx].ram.used;
		double r_percent = response.json[1]["Stats"][i]["MemPerc"].asFloat() * 10.0;
		int i_percent = (int)r_percent;
		pods[pod].containers[idx].ram.percent = (double)i_percent * 0.1;

		if ( response.json[1]["Stats"][i]["PIDs"].isInt()) {

			pid_t pid = (pid_t)response.json[1]["Stats"][i]["PIDs"].asInt();

			pods[pod].containers[idx].pids = { pid };
			pods[pod].containers[idx].pid = pid;

		} else if ( response.json[1]["Stats"][i]["PIDs"].isArray() &&
				response.json[1]["Stats"][i]["PIDs"].size() > 0 ) {

			pid_t pid = -1;
			pods[pod].containers[idx].pids.clear();

			for ( int i4 = 0; i4 < response.json[1]["Stats"][i]["PIDs"].size(); i4++ ) {

				if ( response.json[1]["Stats"][i]["PIDs"][i4].isInt()) {

					pid_t _pid = (pid_t)response.json[1]["Stats"][i]["Pids"][i4].asInt();
					pods[pod].containers[idx].pids.push_back(_pid);

					if ( i4 == 0 )
						pid = _pid;
				}
			}

			if ( pid != -1 )
				pods[pod].containers[idx].pid = pid;
		}
	}

	//if ( pods.empty())
	//	return false;

	mutex.podman.lock();
	this -> state.stats = Podman::Node::OK;
	this -> pods = pods;
	mutex.podman.unlock();

	return true;
}

bool Podman::Container::update_logs(Podman::Socket *socket) {

	if ( this -> isInfra ) {
		this -> logs = { "No logs available for infra containers." };
		return true;
	}

	/*
	if ( !this -> isRunning ) {
		// log::debug << "logs not updated for container " << this -> name << ", container is not running" << std::endl;
		return true;
	}
	*/

	if ( this -> id.empty())
		return false;

	// log::debug << APP_NAME << ": updating container log for " << this -> name << std::endl;

	Podman::Query::Response response;
	Podman::Query query = { .group = "containers",
				.id = this -> id.empty() ? this -> name : this -> id,
				.action = "logs",
				.query = "follow=false&stderr=true&stdout=true&tail=" + std::to_string(CONTAINER_LOG_SIZE) + "&timestamps=false",
				.chunks_to_array = true,
				.parseJson = false };

	if ( !socket -> execute(query, response))
		return false;

	// logs are outputted a bit wierd, let's fix/tolerate it..

	std::vector<std::string> lines;

	bool in_line = false;
	bool in_string = false;
	bool in_array = false;
	char prev = 0;
	std::string line = "";

	for ( char const &c: response.body ) {

		if ( !in_line && !in_string && c == '{' ) {
			in_line = true;
			prev = c;
			continue;
		} else if ( in_line && !in_string && c == '[' )
			in_array = true;
		else if ( in_line && !in_string && c == '\"' )
			in_string = true;
		else if ( in_line && in_string && prev != '\\' && c == '\"' )
			in_string = false;
		else if ( in_line && in_array && !in_string && c == ']' )
			in_array = false;
		else if ( in_line && !in_string && !in_array && c == '}' ) {
			in_line = false;
			prev = c;

			// strip non-ascii characters
			line.erase(std::remove_if(line.begin(), line.end(),
					[](unsigned char c) {
						return ( c == '\n' || c == '\t' ) ? false : ( c < 32 || c > 175 ? true : false );
					}), line.end());

			// clean up entry
			std::replace(line.begin(), line.end(), '\n', ' ');
			std::replace(line.begin(), line.end(), '\t', ' ');
			std::replace(line.begin(), line.end(), '\"', '\u0025');

			std::size_t pos;
			while (( pos = line.find("\u0025")) != std::string::npos )
				line.replace(pos, 1, "\\\"");
			while (( pos = line.find("\\\"\\\\\"")) != std::string::npos )
				line.replace(pos, 5, "\\\"'");
			while (( pos = line.find("\\\\\"\\\"")) != std::string::npos )
				line.replace(pos, 5, "'\\\"");
			while (( pos = line.find("\\\\\"")) != std::string::npos )
				line.replace(pos, 3, "\u0025");
			while (( pos = line.find("\u0025")) != std::string::npos )
				line.replace(pos, 1, "\\\\\\\"");

			if ( !line.empty())
				lines.push_back("\"{" + line + "}\"\n");
			line = "";
			continue;
		}

		if ( in_line )
			line += c;

		prev = c;
	}

	std::string body = "";
	Json::Value json;

	for ( int i1 = 0; i1 < lines.size(); i1++ )
		body += ( i1 == 0 ? "" : "," ) + lines[i1];

	if ( !body.empty()) {
		body = "{ \"logs\":[" + body + "]}";

		std::string err;
		const auto rawJsonLength = static_cast<int>(body.length());
		Json::CharReaderBuilder builder;
		const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());

		if ( !reader -> parse(body.c_str(), body.c_str() + rawJsonLength,
					&json, &err)) {

			log::vverbose << "error: log parser failed to parse json for container " << this -> name << std::endl;
			return false;
		}
	}

	if ( json["logs"].empty() && !response.raw.empty()) {

		std::string hexl, raw = response.raw;

		if ( get_firstline(raw, hexl) && !hexl.empty() && common::is_hex("0x" + hexl)) {

			int bytes = -1;

			lines.clear();

			while ( bytes != 0 ) {
				if ( bytes != -1 )
					if (!( get_firstline(raw, hexl) && !hexl.empty() && common::is_hex("0x" + hexl)))
						break;
				bytes = std::stoul("0x" + hexl, nullptr, 16);
				if ( bytes <= 0 ) break;
				raw.erase(0, hexl.size() + 1);
				std::string entry = raw.substr(0, bytes);
				raw.erase(0, entry.size());
				// strip non-ascii characters
				entry.erase(std::remove_if(entry.begin(), entry.end(),
						[](unsigned char c) { 
							return ( c == '\n' || c == '\t' ) ? false : ( c < 32 || c > 175 ? true : false );
						}), entry.end());
				// clean up entry
				std::replace(entry.begin(), entry.end(), '\n', ' ');
				std::replace(entry.begin(), entry.end(), '\t', ' ');
				std::replace(line.begin(), line.end(), '\"', '\u0025');

				std::size_t pos;
				while (( pos = line.find("\u0025")) != std::string::npos )
					line.replace(pos, 1, "\\\"");
				while (( pos = line.find("\\\"\\\\\"")) != std::string::npos )
					line.replace(pos, 5, "\\\"'");
				while (( pos = line.find("\\\\\"\\\"")) != std::string::npos )
					line.replace(pos, 5, "'\\\"");
				while (( pos = line.find("\\\\\"")) != std::string::npos )
					line.replace(pos, 3, "\u0025");
				while (( pos = line.find("\u0025")) != std::string::npos )
					line.replace(pos, 1, "\\\\\\\"");

				if ( !entry.empty())
					lines.push_back("\"" + common::trim_ws(entry) + "\"");

				hexl = "";
				if ( raw == "0" || raw.empty())
					bytes = 0;
			}

			body = "";
			for ( int i1 = 0; i1 < lines.size(); i1++ )
				body += ( i1 == 0 ? "" : "," ) + lines[i1];

			if ( !body.empty()) {
				body = "{ \"logs\":[" + body + "]}";

				std::string err;
				const auto rawJsonLength = static_cast<int>(body.length());
				Json::CharReaderBuilder builder;
				const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());

				if ( !reader -> parse(body.c_str(), body.c_str() + rawJsonLength,
							&json, &err)) {

					log::vverbose << "error: log parser failed to parse json for container " << this -> name << std::endl;
					return false;
				}
			}
		}
	}

	if ( json["logs"].empty()) {
		body = "{\"logs\":[]}";
		std::string err;
		const auto rawJsonLength = static_cast<int>(body.length());
		Json::CharReaderBuilder builder;
		const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());

		if ( !reader -> parse(body.c_str(), body.c_str() + rawJsonLength,
					&json, &err)) {

			log::vverbose << "error: log parser failed to parse json for container " << this -> name << std::endl;
			return false;
		}		
	}

	std::vector<std::string> logs;
	for ( int i5 = 0; i5 < json["logs"].size(); i5++ )
		if ( json["logs"][i5].isString())
			logs.push_back(json["logs"][i5].asString());

	mutex.podman.lock();
	this -> logs = logs;
	mutex.podman.unlock();

	return true;
}

const bool Podman::podman_t::update_logs(void) {

	if ( this -> state.containers != Podman::Node::OK ) {
		log::vverbose << "error: failed to update logs, container array not ready" << std::endl;
		return false;
	}

	mutex.podman.lock();
	std::vector<Podman::Pod> pods = this -> pods;
	mutex.podman.unlock();

	for ( int p = 0; p < pods.size(); p++ )
		for ( int c = 0; c < pods[p].containers.size(); c++ )
			if ( !pods[p].containers[c].update_logs(&this -> socket)) {
				log::vverbose << "error: failed to retrieve logs for " << pods[p].containers[c].name << std::endl;
				return false;
			}

	mutex.podman.lock();
	this -> pods = pods;
	mutex.podman.unlock();

	return true;
}
