#pragma once

#include "Test.h"
#include "..\Platform\Platform.h"
#include "..\Platform\PlatformTypes.h"
#include "..\Graphics\Renderer.h"
#include "ShaderCompilation.h"


class engineTest : public test {
public:
	bool initialize() override;
	void run() override;
	void shutdown() override;
};
