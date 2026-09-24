#include "host.h"

#include "../../common/log.h"
#include "../renderer.h"

HostApi g_host;

bool host_init_114d();
bool host_init_113d();
bool host_init_113c();
bool host_init_110f();

bool host_init(GameBuildId build)
{
    switch (build) {
    case BUILD_114D:
        return host_init_114d();
    case BUILD_113D:
        return host_init_113d();
    case BUILD_113C:
        return host_init_113c();
    case BUILD_110F:
        return host_init_110f();
    default:
        d2log("host: no bindings for %s", build_name(build));
        return false;
    }
}

extern "C" int renderer_host_init(void)
{
    return host_init(build_identify()) ? 1 : 0;
}
