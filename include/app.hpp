#pragma once
#include <string>

void version_info(void);
void usage(char* cmd);
void parse_cmdline(int argc, char **argv);

extern std::string libpod_version_override;
extern bool log_trace;
