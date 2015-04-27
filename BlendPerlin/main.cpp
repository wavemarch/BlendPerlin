#include <Windows.h>
#include <cstdio>
#include <algorithm>

#include "BlendPerlin.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd) {
	BlendPerlin app(hInstance);

	if (!app.Init())
		return 0;

	return app.Run();
}