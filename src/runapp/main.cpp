#include <Windows.h>
#include <string>
#include <fstream>

std::string getVersionApp(std::string exepath)
{

	// get version
	std::string apppath = exepath.substr(0, exepath.find_last_of("\\/"));
	std::ifstream f(apppath + "\\version.txt");	
	if (f.is_open()) {
		std::string line;
		std::getline(f, line);
		f.close();
		std::string exename = exepath.substr(exepath.find_last_of("\\/")+1, exepath.length());
		std::string app = apppath + "\\v" + line + "\\" + exename;
		std::ifstream exe(app);
		if (exe.good()) {
			exe.close();
			return app;
		}			
	}
	return "";
}

// load python37.dll from lib directory
// use explicit dll loading instead of implicit
// then import app.py from app directory and call app.run

typedef void(*P_INITIALIZEEX)(int);
typedef void(*P_PYSYS_SETARGVEX)(int, char**, int);
typedef void(*P_RUN_SIMPLESTRING)(const char*);
typedef void(*P_FINALIZEEX)(int);

//int main(int argc, char *argv1[])
// program without window
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int cmdShow)
{
	char buffer[MAX_PATH];
	char fullpath[MAX_PATH];
	GetModuleFileName(NULL, buffer, MAX_PATH);
	_fullpath(fullpath, buffer, MAX_PATH);
	std::string exepath = fullpath;

	// check launch mode and run version/app.exe if so
	std::string app = getVersionApp(exepath);
	if (app != "") {
		int ret = (int)ShellExecute(0, "open", app.c_str(), "", 0, 1);
		return ret;
	}

	std::string apppath = exepath.substr(0, exepath.find_last_of("\\/"));
	std::string libpath = apppath + "\\python\\python37.dll";

	HINSTANCE hPylib = LoadLibrary(TEXT(libpath.c_str()));
	if (hPylib == NULL)
		return 1;

	// locate and run the Py_InitializeEx() function
	FARPROC pPy_InitializeEx = GetProcAddress(hPylib, "Py_InitializeEx");
	if (pPy_InitializeEx == NULL)
		return 2;
	((P_INITIALIZEEX)pPy_InitializeEx)(0);

	// locate and run the PySys_SetArgvEx() function
	FARPROC pPySys_SetArgvEx = GetProcAddress(hPylib, "PySys_SetArgvEx");
	if (pPySys_SetArgvEx == NULL)
		return 3;
	char *argv[] = { (char*)exepath.c_str() };
	((P_PYSYS_SETARGVEX)pPySys_SetArgvEx)(1, (char**)argv, 0);


	// locate and run the Py_InitializeEx() function
	FARPROC pPyRun_SimpleString = GetProcAddress(hPylib, "PyRun_SimpleString");
	if (pPyRun_SimpleString == NULL)
		return 4;
	char pycode[1000];
	snprintf(pycode, sizeof(pycode),
		"import sys\n"
		"import os\n"
		"appdir = os.path.dirname(r'%s')\n"
		"sys.path.insert(0, fr'{appdir}\\app')\n"
		"#print(sys.path)\n"
		"#input('press any key')\n"
		"import app\n"
		"app.run()\n", exepath.c_str());
	((P_RUN_SIMPLESTRING)pPyRun_SimpleString)(pycode);
		
	// locate and run the Py_FinalizeEx() function
	FARPROC pPy_FinalizeEx = GetProcAddress(hPylib, "Py_FinalizeEx");
	if (pPy_FinalizeEx == NULL)
		return 5;
	((P_FINALIZEEX)pPy_FinalizeEx)(0);

	FreeLibrary(hPylib);
	return 0;
}