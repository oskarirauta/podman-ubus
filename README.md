# podman-ubus

podman-ubus extends ubus with bindings to podman
and allows controlling and statistics over pods
and containers through ubus.

podman-ubus is written in C++ except for parts which
have been externed from C.

### Requirements/compatibility
 - openwrt
 - openwrt variants with ubus (untested)

### Work in progress
 - extend capabilities of software
 - expose more features over ubus (logs for example)
 - luci

### Anything else?
Podman code can be used for other purposes as well.
It is broken down to sub-directory and comes with example.

podman-ubus was previously included in my other very similar
project systembus but I decided to break it down into it's own.

### Building
```make```
or if you want to build podman example project
```make podman_example```

author: Oskari Rauta
license: MIT
