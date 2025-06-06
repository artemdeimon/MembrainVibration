# Membrain Vibration - Демо Проект

## Описание
<p align="justify">
Программа моделирует распространение двухмерной волны внутри 
ограниченной поверхности. В качестве модели распространения 
волны выступает волновое уравнение:

$$
  \frac{\partial^2 u}{\partial x^2} + 
  \frac{\partial^2 u}{\partial y^2} =
  \frac{1}{v^2}
  \frac{\partial^2 u}{\partial t^2},
$$

где $u(x,y,t)$ - положение точки мебраны по оси $Z$, $t$ - 
время, $v$ - фазовая скорость распространения волны. 
Волоновое уравнение решается с помощью метода конечных разностей, 
путем нанесения равномерной сетки на поверхность мембраны с шагом 
$\Delta x$ и $\Delta y$ по оси $X$ и $Y$ соответсвенно и решения 
разностного уравнения в точках сетки:

$$
  \frac{\Delta^2 u}{\Delta x^2} +
  \frac{\Delta^2 u}{\Delta y^2} =
  \frac{1}{v^2}
  \frac{\Delta^2 u}{\Delta t^2},
$$

где $\Delta^2u/\Delta x^2$ - вторая конечная разность 
сетки на $u$ по  переменной $x$. Решение конечно-разностного 
уравнения осуществляется относительно $u$ во временных точках
с шагом $\Delta t$ для каждой точки сетки, которая наноситься 
на поверхность $u$.

Программа осуществляет отрисовку моделируемой волны:
![alt text](images/CenterWave.gif)
![alt text](images/JungeInterference.gif)

</p>

## Используемые технологии

- Язык: C++, GLSL
- Компилятор: gcc, mingw
- Фреймворк: Qt5
- Сбокра: qmake, make

## Сборка
Для сборки необходимо запустить в директории проекта команды:
```
qmake
make
```