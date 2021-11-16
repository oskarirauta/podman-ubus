#pragma once

namespace Podman {

	namespace Node {

		typedef enum {
			INCOMPLETE = 0,
			NEEDS_UPDATE,
			UNKNOWN,
			FAILED,
			OK
		} State;

	}
}
