#pragma once

#include <string>
#include <vector>
#include <json/json.h>

#include "podman_query.hpp"

namespace Podman {

	bool verifyJsonElements(Json::Value jsondata, std::vector<std::string> names, Podman::Query *query = NULL);

}
