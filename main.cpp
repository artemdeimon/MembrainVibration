/**
  Программа решения 2-ух мерного волнового уравнения методом
  конечных разностей
  Расчет уравнения производиться с помощью GLSL шейдеров и библиотеки OpenGL
*/

#include "widget.h"
#include <QApplication>

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);
  Widget w;
  w.show();

  return a.exec();
}
