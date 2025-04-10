/**

*/

#ifndef WIDGET_H
#define WIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_5_Core>
#include <QMatrix3x3>
#include <QTimer>

class Widget : public QOpenGLWidget
{
  Q_OBJECT
public:
  explicit Widget(QWidget *parent = 0);
  ~Widget();

protected:
  void initializeGL() override;
  void paintGL() override;

  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;


private slots:
  // Функция, срабатывающая по таймеру, осуществляет рендер сцены
  void renderTimeOut();

  // Метод смены модели распространения волны.
  // Аргумент mode задает модель:
  // "None" - отсутвие источников волн
  // "SimpleImpulse" - модель простого импульса
  // "CenterWave" - точечный источник в центре мембраны
  // "CornerWave" - точечный источник в углу мембраны
  // "JungeInterference" - двухщелевой опыт Юнга
  // "FlatWave" - источник плоской волны
  void changeModelMode(QString mode);

  // Метод смены режима света в сцене
  // Аргумент mode задает режим света в сцене:
  // "ADS mode" -   режим света, с точечным источником света,
  //                свечением самой месбраны (Avbient),
  //                диффузным отражением света мембранной (Diffuse) и
  //                зеркальным отрадением света (Specular reflection)
  // "Height map" - режим света, отображающая карту высот точек мембраны.
  //                Источником света является сама мембрана. Цвет окраса
  //                зависит от положения точки мембраны по оси z.
  //                Чем больше координата z в положительной области, тем
  //                болеее точка красаная. Чем меньше координата z в
  //                отрицательной в области, тем более она синяя.
  void changeShadeMode(QString mode);

  // Метод изменения границы смены цветов точек волны
  // в режиме Height map
  void changeZScale(double new_z_scale);

private:
  using t_dict_pair = std::pair<QString, QString>;
  using t_dict_vec = std::vector<t_dict_pair>;

  // Вектор названий моделей распротсранения волн
  std::vector<QString> model_modes;
  int model_mode;

  QOpenGLFunctions_4_5_Core *gl;
  bool is_error;

  GLuint render_program;
  GLuint compute_program;

  GLuint u_vbo;   // VBO U буффера см. membrane_vibration.glsl
  GLuint xy_vbo;  // VBO XY буффера см. membrane_vibration.glsl
  GLuint ebo;     // EBO треугольников образующих сетку бембраны
  GLuint vao;     // VAO с информацией о геометрии мембраны

  uint32_t size_x; // Рамзер сетки по оси х
  uint32_t size_y; // Рамзер сетки по оси y

  GLint model_uni;   // Uniform переменная поворота мембраны
  QMatrix3x3 model;  // Оператор поворота модели
  QPointF old_pos;   // Положение мыши на предыдущем рендере
  double x_angle;    // Угол поворта относительно оси x
  double z_angle;    // Угол поворта относительно оси z

  GLint c_out_uni;      // Uiform переменная c_out переменной для
                        // вычислительного шейдера
  GLint c_out_rend_uni; // Uiform переменная c_out переменной для
                        // для вершинного шейдера
  int c_out;            // Значение c_out переменной
                        // см. membrane_vibration.glsl

  double dx;            // Дистанция между соседними вершинами
                        // сетки по оси x
  double dy;            // Дистанция между соседними вершинами
                        // сетки по оси x
  double v;             // Фазовая скорость волны
  double dt;            // Временной шаг, между вычислениями
  QTimer timer;         // Таймер отрисовки

  uint32_t ti;          // Количество вычислений

  bool render_mode;     // Флаг при выставлении, которого
                        // функция paintGL выполнит рендер
  int shade_mode;       // Режим отрисовки мембраны см. changeShadeMode
  GLuint ADS_shade;     // Индекс попрограммы для вычисления освещения
                        // в режиме ADSModel
  GLuint hMap_shade;    // Индекс попрограммы для вычисления освещения
                        // в режиме heightShadeMode

  uint32_t speed_factor; // Скорость воспроизведения модели.
                         // Означает количество шагов решения дифференциального
                         // уравнения за один шаг рендера

  double z_scale;        // См. membrane_vibration.glsl
  GLint z_scale_uni;     // Uniform переменная для z_scale

  // Функция загрузки шейдера из файла и его компиляция
  GLuint createShader(GLuint shader_type, const char *filename, bool &is_error,
                      t_dict_vec *dict_vec = nullptr);
  // Фугкция сборки программы из шейдеров
  GLuint linkProgram(GLuint *shaders, uint32_t shaders_count, bool &is_error);
  const char* readFile(const char *filename);

  void initBufs();
  QWidget *settings;

  // Следующие методы отвечают за инициализацию моделей
  // распространения двухмерной волны (init...)
  // и запуск вычислительного шейдера моделирующего
  // рапространение волны за один временной шаг (calculation...)

  // Модель простого импульса
  void initSampleImpulse(float *u_init, float *xy_buf);
  void calculationSampleImpulse();

  // Модель точечного источника волны, находящегося в центре мембраны
  void initCenterWave(float *, float *);
  void calculationCenterWave();

  // Модель точечного источника волны, находящегося в углу мембраны
  void initCornerWave(float *, float *);
  void calculationCornerWave();

  // Модель двухщелевого опыта Юнга, иллюстрирующая интреференцию
  // когерентных волн
  void initJungeInterference(float *, float *);
  void calculationJungeInterference();

  // Модель с источником плоской волны
  void initFlatWave(float *, float *);
  void calculationFlatWave();

};

#endif // WIDGET_H
