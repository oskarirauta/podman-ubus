#include "common.hpp"
#include "log.hpp"
#include "podman_validate.hpp"

bool Podman::verifyJsonElements(Json::Value jsondata, std::vector<std::string> names, Podman::Query *query) {

	for ( auto it = names.begin(); it != names.end(); ++it )
		if ( !jsondata.isMember(*it)) {
			if ( query != NULL )
				log::verbose << "failed to call: " << common::trim_leading(query -> path()) << std::endl;
			log::vverbose << "error: json element mismatch for element \"" << *it << "\"" << std::endl;
			return false;
		}

	return true;
}
