#pragma once
#include "CommonHeaders.h"

#if !defined(SHIPPING)

namespace WAVEENGINE::CONTENT {

bool load_game();

void unload_game();

}

#endif // !defined(SHIPPING)