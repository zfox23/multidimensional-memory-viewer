#include "MacPlatform.h"
#import <Foundation/Foundation.h>
#include <sys/sysctl.h>

namespace MacPlatform {

bool isRosetta()
{
    int val = 0;
    size_t size = sizeof(val);
    if (sysctlbyname("sysctl.proc_translated", &val, &size, nullptr, 0) == 0)
        return val == 1;
    return false;
}

QString appName()
{
    return QStringLiteral("Multidimensional Memory Viewer");
}

} // namespace MacPlatform
