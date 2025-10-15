#include "stdafx.h"
#ifdef PLATFORM_WINDOWS
#include <Windows.h>

extern int main(int argc, char* argv[]);

int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nShowCmd)
{
	return main(__argc, __argv);
}

#endif // PLATFORM_WINDOWS