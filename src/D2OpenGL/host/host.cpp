#include "host.h"

#include "../../common/log.h"
#include "../renderer.h"

HostApi g_host;

bool host_init_114d();

bool host_init(GameBuildId build)
{
    switch (build) {
    case BUILD_114D:
        return host_init_114d();
    default:
        d2log("host: no bindings for %s", build_name(build));
        return false;
    }
}

extern "C" int renderer_host_init(void)
{
    return host_init(build_identify()) ? 1 : 0;
}
