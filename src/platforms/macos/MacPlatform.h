#pragma once
#include <QString>

namespace MacPlatform {
// Returns true if the app is running under Rosetta 2 (ARM Mac + x86_64 binary).
bool isRosetta();
// Application name for display in native menus.
QString appName();
}
