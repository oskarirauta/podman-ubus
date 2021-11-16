#include <iostream>

#include <string>
#include <vector>
#include <json/json.h>

#include "common.hpp"
#include "podman_constants.hpp"
#include "podman_query.hpp"

bool Podman::Query::Response::parseJson(void) {

	this -> error_msg = "";

	std::string err;
	const auto rawJsonLength = static_cast<int>(this -> body.length());
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	if ( !reader->parse(this -> body.c_str(), this -> body.c_str() + rawJsonLength,
		&this -> json, &err)) {
		std::string errmsg = "unknown json error";
                std::vector<std::string> lines = common::lines(err);
                if ( lines.size() > 0 )
                        errmsg = lines.size() == 1 ? common::trim_ws(lines[0]) :
                                ( "[" + common::trim_ws(lines[0]) + "]: " +
                                        common::trim_ws(lines[1]));
		this -> error_msg = errmsg;
		return false;
	}

	return true;
}
