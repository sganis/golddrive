#include "mainwindow.h"
#include <QApplication>
#ifdef _DEBUG
  #undef _DEBUG
  #include <Python.h>
  #define _DEBUG
#else
  #include <Python.h>
#endif

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    wchar_t *program = Py_DecodeLocale(argv[0], nullptr);
    if (program == nullptr) {
        fprintf(stderr, "Fatal error: cannot decode argv[0]\n");
        exit(1);
    }
    Py_SetProgramName(program);  /* optional but recommended */
    Py_Initialize();
    PyRun_SimpleStringFlags("import app\n", nullptr);

    MainWindow w;
    w.show();
    int ret = a.exec();

    if (Py_FinalizeEx() < 0)
        exit(120);
    PyMem_RawFree(program);
    return ret;
}
