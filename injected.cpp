#include <windows.h>
#include <iostream>
#include <fstream>
#include <psapi.h>
#include <string>
#include <regex>

#define PYTHON_SCRIPT_PATH "C:\\path\\to\\script.py"

// // Manually defined types for Python C API
typedef int PyGILState_STATE; // Assume enum compatibility
typedef int (*Py_IsInitializedFunc)();
typedef PyGILState_STATE (*PyGILState_EnsureFunc)();
typedef void (*PyGILState_ReleaseFunc)(PyGILState_STATE);
typedef int (*PyRun_SimpleStringFunc)(const char *);

#define DLL_EXPORT __declspec(dllexport)

std::string getPythonDll()
{
	HMODULE hMods[1024];
	DWORD cbNeeded;
	std::regex pattern(R"(python\d{1,5}\.dll)");
	std::smatch match;
	HANDLE hProcess = GetCurrentProcess(); // Get handle to the current process

	if (EnumProcessModulesEx(hProcess, hMods, sizeof(hMods), &cbNeeded, LIST_MODULES_ALL))
	{
		int count = cbNeeded / sizeof(HMODULE);
		for (int i = 0; i < count; i++)
		{
			char moduleName[MAX_PATH];
			if (GetModuleFileNameExA(hProcess, hMods[i], moduleName, sizeof(moduleName)))
			{
				std::string str(moduleName);
				if (std::regex_search(str, match, pattern))
				{
					return str;
				}
			}
		}
	}
	return "";
}

void run_python_script()
{
	std::ifstream file(PYTHON_SCRIPT_PATH);
	std::string pythonCode((std::istreambuf_iterator<char>(file)),
						   (std::istreambuf_iterator<char>()));
	file.close();
	std::string pythonDll = getPythonDll();
	if (pythonDll.empty())
		return;

	HMODULE hPython = GetModuleHandleA(pythonDll.c_str()); // Adjust for your Python version

	if (!hPython)
		return;

	// Resolve Python functions dynamically
	auto Py_IsInitialized = reinterpret_cast<Py_IsInitializedFunc>(
		GetProcAddress(hPython, "Py_IsInitialized"));
	auto PyGILState_Ensure = reinterpret_cast<PyGILState_EnsureFunc>(
		GetProcAddress(hPython, "PyGILState_Ensure"));
	auto PyGILState_Release = reinterpret_cast<PyGILState_ReleaseFunc>(
		GetProcAddress(hPython, "PyGILState_Release"));
	auto PyRun_SimpleString = reinterpret_cast<PyRun_SimpleStringFunc>(
		GetProcAddress(hPython, "PyRun_SimpleString"));

	if (!Py_IsInitialized || !PyGILState_Ensure ||
		!PyGILState_Release || !PyRun_SimpleString)
	{
		return;
	}

	// Verify Python interpreter state
	if (!Py_IsInitialized())
		return;

	// Execute Python code with GIL management
	PyGILState_STATE gstate = PyGILState_Ensure();
	PyRun_SimpleString(pythonCode.c_str());
	PyGILState_Release(gstate);
}
// DllMain (optional for initialization/cleanup)
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		run_python_script();
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
