#include <string>

#include "constants.hpp"
#include "common.hpp"
#include "log.hpp"
#include "podman.hpp"
#include "mutex.hpp"
#include "ubus_podman.hpp"

enum PODMAN_EXEC_ACTION_TYPE {
	PODMAN_EXEC_ACTION_UNKNOWN = 0,
	PODMAN_EXEC_ACTION_START,
	PODMAN_EXEC_ACTION_STOP,
	PODMAN_EXEC_ACTION_RESTART,
};

enum PODMAN_EXEC_GROUP_TYPE {
	PODMAN_EXEC_GROUP_UNKNOWN = 0,
	PODMAN_EXEC_GROUP_POD,
	PODMAN_EXEC_GROUP_CONTAINER,
};

const struct blobmsg_policy podman_exec_policy[] = {
	[PODMAN_EXEC_ACTION] = { .name = "action", .type = BLOBMSG_TYPE_STRING },
	[PODMAN_EXEC_GROUP] = { .name = "group", .type = BLOBMSG_TYPE_STRING },
	[PODMAN_EXEC_NAME] = { .name = "name", .type = BLOBMSG_TYPE_STRING },
};

int podman_exec_policy_size(void) {
	return ARRAY_SIZE(podman_exec_policy);
}

int systembus_podman_status(struct ubus_context *ctx, struct ubus_object *obj,
		struct ubus_request_data *req, const char *method,
		struct blob_attr *msg) {

	log::debug << APP_NAME << ": ubus call podman::status received" << std::endl;
	std::lock_guard<std::mutex> guard(mutex.podman);

	blob_buf_init(&b, 0);
	blobmsg_add_u8(&b, "running", podman_data -> status == Podman::RUNNING ? true : false);
	blobmsg_add_string(&b, "status", podman_data -> status == Podman::RUNNING ? "running" : ( podman_data -> status == Podman::UNAVAILABLE ? "unavailable" : "unknown" ));
	ubus_send_reply(ctx, req, b.head);
	return 0;
}

int systembus_podman_info(struct ubus_context *ctx, struct ubus_object *obj,
                struct ubus_request_data *req, const char *method,
                struct blob_attr *msg) {

	log::debug << APP_NAME << ": ubus call podman::info received" << std::endl;
	std::lock_guard<std::mutex> guard(mutex.podman);

	blob_buf_init(&b, 0);
	blobmsg_add_string(&b, "status", podman_data -> status == Podman::RUNNING ? "running" : ( podman_data -> status == Podman::UNAVAILABLE ? "unavailable" : "unknown"));
	blobmsg_add_string(&b, "arch", podman_data -> arch.c_str());
	blobmsg_add_string(&b, "os", podman_data -> os == "\"openwrt\" linux" ? "openwrt" : podman_data -> os.c_str());
	blobmsg_add_string(&b, "os_version", podman_data -> os_version.c_str());
	blobmsg_add_string(&b, "hostname", podman_data -> hostname.c_str());
	blobmsg_add_string(&b, "kernel", podman_data -> kernel.c_str());

	void *cookie = blobmsg_open_table(&b, "mem");
	blobmsg_add_string(&b, "free", podman_data -> memFree.c_str());
	blobmsg_add_string(&b, "total", podman_data -> memTotal.c_str());
	blobmsg_close_table(&b, cookie);

	void *cookie2 = blobmsg_open_table(&b, "swap");
	blobmsg_add_string(&b, "free", podman_data -> swapFree.c_str());
	blobmsg_add_string(&b, "total", podman_data -> swapTotal.c_str());
	blobmsg_close_table(&b, cookie2);

	blobmsg_add_string(&b, "conmon", podman_data -> conmon.c_str());
	blobmsg_add_string(&b, "ociRuntime", podman_data -> ociRuntime.c_str());
	blobmsg_add_string(&b, "version", podman_data -> version.c_str());
	blobmsg_add_string(&b, "api_version", podman_data -> api_version.c_str());
	ubus_send_reply(ctx, req, b.head);
	return 0;
}

int systembus_podman_networks(struct ubus_context *ctx, struct ubus_object *obj,
		struct ubus_request_data *req, const char *method,
		struct blob_attr *msg) {

	log::debug << APP_NAME << ": ubus call podman::networks received" << std::endl;
	std::lock_guard<std::mutex> guard(mutex.podman);

	blob_buf_init(&b, 0);

	if ( podman_data -> status != Podman::PODMAN_STATUS::RUNNING ) {
		blobmsg_add_string(&b, "message", podman_data -> status == Podman::PODMAN_STATUS::UNAVAILABLE ?
			"Podman unavailable" : "Podman state unknown");
		ubus_send_reply(ctx, req, b.head);
		return 0;
	}

	void *cookie = blobmsg_open_array(&b, "networks");

	for ( const auto& network : podman_data -> networks ) {
		void *cookie2 = blobmsg_open_table(&b, "");
		blobmsg_add_string(&b, "name", network.name.c_str());
		blobmsg_add_string(&b, "cni_version", network.version.c_str());
		blobmsg_add_string(&b, "type", network.type.c_str());
		blobmsg_add_u8(&b, "gateway", network.isGateway);
		blobmsg_add_string(&b, "plugins", network.plugins.c_str());

		void *cookie3 = blobmsg_open_table(&b, "ipam");
		blobmsg_add_string(&b, "type", network.ipam.type.c_str());

		void *cookie4 = blobmsg_open_array(&b, "ranges");
		for ( const auto& range : network.ipam.ranges ) {
			void *cookie5 = blobmsg_open_table(&b, "");
			blobmsg_add_string(&b, "gateway", range.gateway.c_str());
			blobmsg_add_string(&b, "subnet", range.subnet.c_str());
			blobmsg_close_table(&b, cookie5);
		}
		blobmsg_close_array(&b, cookie4);

		void *cookie6 = blobmsg_open_array(&b, "routes");
		for ( const auto& route : network.ipam.routes )
			blobmsg_add_string(&b, "", route.c_str());
		blobmsg_close_array(&b, cookie6);
		blobmsg_close_table(&b, cookie3);
		blobmsg_close_table(&b, cookie2);
	}
	blobmsg_close_array(&b, cookie);

	ubus_send_reply(ctx, req, b.head);
	return 0;
}

int systembus_podman_list(struct ubus_context *ctx, struct ubus_object *obj,
		struct ubus_request_data *req, const char *method,
		struct blob_attr *msg) {

	log::debug << APP_NAME << ": ubus call podman::list received" << std::endl;
	std::lock_guard<std::mutex> guard(mutex.podman);

	blob_buf_init(&b, 0);

	if ( podman_data -> status != Podman::PODMAN_STATUS::RUNNING ) {
		blobmsg_add_string(&b, "message", podman_data -> status == Podman::PODMAN_STATUS::UNAVAILABLE ?
			"Podman unavailable" : "Podman state unknown");
		ubus_send_reply(ctx, req, b.head);
		return 0;
	}

	void *cookie = blobmsg_open_array(&b, "pods");

	for ( const auto& pod : podman_data -> pods ) {

		int indexOfInfra = -1;

		for ( int i = 0; i < pod.containers.size(); i++ ) {
			if ( pod.containers[i].isInfra ) {
				indexOfInfra = i;
				break;
			}
		}

		void *cookie2 = blobmsg_open_table(&b, "");
		blobmsg_add_string(&b, "name", pod.name.c_str());
		blobmsg_add_string(&b, "id", pod.id.c_str());
		blobmsg_add_string(&b, "status", pod.status.c_str());
		blobmsg_add_string(&b, "infraid", pod.infraId.c_str());
		blobmsg_add_u8(&b, "hasinfra", !pod.infraId.empty());
		blobmsg_add_u8(&b, "running", pod.isRunning);
		blobmsg_add_u16(&b, "num_containers", pod.containers.size());

		void *cookie3 = blobmsg_open_array(&b, "containers");
		for ( const auto& container : pod.containers ) {
			void *cookie4 = blobmsg_open_table(&b, "");
			blobmsg_add_string(&b, "name", container.name.c_str());
			blobmsg_add_string(&b, "image", container.image.c_str());
			blobmsg_add_string(&b, "cmd", container.command.c_str());
			blobmsg_add_u8(&b, "infra", container.isInfra);
			blobmsg_add_string(&b, "pod", container.pod.c_str());
			blobmsg_add_u8(&b, "running", container.isRunning);
			blobmsg_add_u32(&b, "pid", container.pid);
			blobmsg_add_string(&b, "state", container.state.c_str());
			blobmsg_add_string(&b, "status", container.status.c_str());
			blobmsg_add_string(&b, "started", common::time_str(container.startedAt).c_str());

			std::time_t uptime = container.uptime;
			int d = uptime > 86400 ? uptime / 86400 : 0;
			if ( d != 0 ) uptime -= d * 86400;
			int h = uptime > 3600 ? uptime / 3600 : 0;
			if ( h != 0 ) uptime -= h * 3600;
			int m = uptime > 60 ? uptime / 60 : 0;
			if ( m != 0 ) uptime -= m * 60;

			void *cookie5 = blobmsg_open_table(&b, "uptime");
			blobmsg_add_u16(&b, "days", d);
			blobmsg_add_u16(&b, "hours", h);
			blobmsg_add_u16(&b, "minutes", m);
			blobmsg_add_u16(&b, "seconds", (int)uptime);
			blobmsg_close_table(&b, cookie5);

			void *cookie6 = blobmsg_open_table(&b, "cpu");
			blobmsg_add_string(&b, "load", container.cpu.text.c_str());
			blobmsg_add_u16(&b, "percent", (int)container.cpu.percent);
			blobmsg_close_table(&b, cookie6);

			void *cookie7 = blobmsg_open_table(&b, "ram");
			blobmsg_add_string(&b, "used", common::memToStr(container.ram.used, true).c_str());
			blobmsg_add_string(&b, "free", common::memToStr(container.ram.free, true).c_str());
			blobmsg_add_string(&b, "max", common::memToStr(container.ram.max, true).c_str());
			blobmsg_add_u16(&b, "percent", container.ram.percent);
			blobmsg_close_table(&b, cookie7);

			if ( !container.isInfra ) {

				bool canStart = !container.isRunning;
				bool canStop = !canStart;
				bool canRestart = canStop;

				if ( indexOfInfra != -1 && ( !pod.isRunning || !pod.containers[indexOfInfra].isRunning )) {
					canStart = false;
					canStop = false;
					canRestart = false;
				}

				void *cookie8 = blobmsg_open_table(&b, "actions");
				blobmsg_add_u8(&b, "start", canStart);
				blobmsg_add_u8(&b, "stop", canStop);
				blobmsg_add_u8(&b, "restart", canRestart);
				blobmsg_close_table(&b, cookie8);
			} else {

				void *cookie8 = blobmsg_open_table(&b, "actions");
				blobmsg_add_u8(&b, "start", false);
				blobmsg_add_u8(&b, "stop", false);
				blobmsg_add_u8(&b, "restart", false);
				blobmsg_close_table(&b, cookie8);
			}

			blobmsg_close_table(&b, cookie4);
		}
		blobmsg_close_array(&b, cookie3);
		blobmsg_close_table(&b, cookie2);
	}

	blobmsg_close_array(&b, cookie);
	ubus_send_reply(ctx, req, b.head);
	return 0;
}

int systembus_podman_exec(struct ubus_context *ctx, struct ubus_object *obj,
		struct ubus_request_data *req, const char *method,
		struct blob_attr *msg) {

	struct blob_attr *tb[__PODMAN_EXEC_ARGS_MAX];
	blobmsg_parse(podman_exec_policy, ARRAY_SIZE(podman_exec_policy), tb, blob_data(msg), blob_len(msg));

	PODMAN_EXEC_ACTION_TYPE action = PODMAN_EXEC_ACTION_UNKNOWN;
	PODMAN_EXEC_GROUP_TYPE group = PODMAN_EXEC_GROUP_UNKNOWN;

	std::string _action = "";
	std::string _group = "";

	if ( tb[PODMAN_EXEC_ACTION] ) {
		_action = common::to_lower(std::string((char*)blobmsg_data(tb[PODMAN_EXEC_ACTION])));
		if ( _action == "start" ) action = PODMAN_EXEC_ACTION_START;
		else if ( _action == "stop" ) action = PODMAN_EXEC_ACTION_STOP;
		else if ( _action == "restart" ) action = PODMAN_EXEC_ACTION_RESTART;
	}

	if ( tb[PODMAN_EXEC_GROUP] ) {
		_group = common::to_lower(std::string((char*)blobmsg_data(tb[PODMAN_EXEC_GROUP])));
		if ( _group == "pod" ) group = PODMAN_EXEC_GROUP_POD;
		else if ( _group == "container" ) group = PODMAN_EXEC_GROUP_CONTAINER;
	}

	if ( action == PODMAN_EXEC_ACTION_UNKNOWN || group == PODMAN_EXEC_GROUP_UNKNOWN ) {
		log::debug << APP_NAME << ": ubus call podman::exec received" << std::endl;
		log::vverbose << APP_NAME << ": ubus_podman_exec error, unknown action or group" << std::endl;
		return UBUS_STATUS_INVALID_ARGUMENT;
	}

	std::string name = tb[PODMAN_EXEC_NAME] ?
		std::string((char*)blobmsg_data(tb[PODMAN_EXEC_NAME])) : "";

	if ( name.empty()) {
		log::debug << APP_NAME << ": ubus call podman::exec received" << std::endl;
		log::vverbose << APP_NAME << ": ubus_podman_exec error, missing name" << std::endl;
		return UBUS_STATUS_INVALID_ARGUMENT;
	}

	Podman::Scheduler::Cmd cmd;

	if ( group == PODMAN_EXEC_GROUP_POD ) {
		if ( action == PODMAN_EXEC_ACTION_STOP )
			cmd.type = Podman::Scheduler::CmdType::POD_STOP;
		else if ( action == PODMAN_EXEC_ACTION_START )
			cmd.type = Podman::Scheduler::CmdType::POD_START;
		else if ( action == PODMAN_EXEC_ACTION_RESTART )
			cmd.type = Podman::Scheduler::CmdType::POD_RESTART;
	} else if ( group == PODMAN_EXEC_GROUP_CONTAINER ) {
		if ( action == PODMAN_EXEC_ACTION_STOP )
			cmd.type = Podman::Scheduler::CmdType::CONTAINER_STOP;
		else if ( action == PODMAN_EXEC_ACTION_START )
			cmd.type = Podman::Scheduler::CmdType::CONTAINER_START;
		else if ( action == PODMAN_EXEC_ACTION_RESTART )
			cmd.type = Podman::Scheduler::CmdType::CONTAINER_RESTART;
	}

	cmd.name = name;
	podman_scheduler -> addCmd(cmd);

	log::debug << APP_NAME << ": ubus call podman::exec::" << _group << "::" << _action << " received" << std::endl;

	blob_buf_init(&b, 0);

	mutex.podman.lock();
	if ( podman_data -> status != Podman::PODMAN_STATUS::RUNNING ) {
		blobmsg_add_string(&b, "message", podman_data -> status == Podman::PODMAN_STATUS::UNAVAILABLE ?
			"Podman unavailable" : "Podman state unknown");
		mutex.podman.unlock();
		ubus_send_reply(ctx, req, b.head);
		return 0;
	}
	mutex.podman.unlock();

	blobmsg_add_string(&b, "action", _action.c_str());
	blobmsg_add_string(&b, "group", _group.c_str());
	blobmsg_add_string(&b, "name", name.c_str());
	ubus_send_reply(ctx, req, b.head);
	return 0;
}

/*

int systembus_podman_func(struct ubus_context *ctx, struct ubus_object *obj,
		struct ubus_request_data *req, const char *method,
		struct blob_attr *msg) {

	log::debug << APP_NAME << ": ubus call podman::func received" << std::endl;
	std::lock_guard<std::mutex> guard(mutex.podman);

	blob_buf_init(&b, 0);
	blobmsg_add_u8(&b, "bool", true);
	blobmsg_add_string(&b, "string", "string");
	ubus_send_reply(ctx, req, b.head);
	return 0;
}

*/
