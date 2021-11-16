#pragma once

#include "ubus.hpp"

enum {
        PODMAN_EXEC_ACTION,
        PODMAN_EXEC_GROUP,
        PODMAN_EXEC_NAME,
        __PODMAN_EXEC_ARGS_MAX
};

extern const struct blobmsg_policy podman_exec_policy[];
extern const struct blobmsg_policy podman_logs_policy[];

int podman_exec_policy_size(void);
int podman_logs_policy_size(void);

int systembus_podman_status(struct ubus_context *ctx, struct ubus_object *obj,
		struct ubus_request_data *req, const char *method,
		struct blob_attr *msg);

int systembus_podman_info(struct ubus_context *ctx, struct ubus_object *obj,
		struct ubus_request_data *req, const char *method,
		struct blob_attr *msg);

int systembus_podman_networks(struct ubus_context *ctx, struct ubus_object *obj,
		struct ubus_request_data *req, const char *method,
		struct blob_attr *msg);

int systembus_podman_list(struct ubus_context *ctx, struct ubus_object *obj,
                struct ubus_request_data *req, const char *method,
                struct blob_attr *msg);

int systembus_podman_exec(struct ubus_context *ctx, struct ubus_object *obj,
		struct ubus_request_data *req, const char *method,
		struct blob_attr *msg);

int systembus_podman_logs(struct ubus_context *ctx, struct ubus_object *obj,
		struct ubus_request_data *req, const char *method,
		struct blob_attr *msg);

/*
int systembus_podman_func(struct ubus_context *ctx, struct ubus_object *obj,
		struct ubus_request_data *req, const char *method,
		struct blob_attr *msg);
*/
