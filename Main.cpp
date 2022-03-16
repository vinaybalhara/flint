#include "Engine.hpp"
#include "Platform.hpp"
#include <windows.h>
#include <chrono>
#include <iostream>


int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR  lpCmdLine, int nShowCmd)
{
	flint::EngineParameters params;
	params.commandLine = lpCmdLine;
	flint::Engine* pEngine = flint::Engine::Create(params);
	if (pEngine->initialize())
		pEngine->run();
	pEngine->release();
	return 0;
}