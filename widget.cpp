#include "widget.h"
#include "ui_widget.h"
#include <QDebug>
#include <fstream>
#include <QMouseEvent>
#include <QMatrix4x4>
#include <QGridLayout>
#include <QComboBox>
#include <QLabel>
#include <QDoubleSpinBox>
#include <math.h>

Widget::Widget(QWidget *parent):
  QOpenGLWidget(parent),
  model_mode(0),
  gl(new QOpenGLFunctions_4_5_Core),
  is_error(false),
  u_vbo(0),
  xy_vbo(0),
  size_x(500),
  size_y(500),
  x_angle(0),
  z_angle(0),
  ti(0),
  render_mode(false),
  shade_mode(0),
  speed_factor(1),
  z_scale(200),
  settings(new QWidget())
{
  settings->show();
  QGridLayout *grid_layout;
  settings->setLayout(grid_layout = new QGridLayout());


  QComboBox *model_mode_combo;
  grid_layout->addWidget(new QLabel("Модель"), 0, 0);
  grid_layout->addWidget(model_mode_combo = new QComboBox(settings), 0, 1);
  model_mode_combo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

  model_modes.push_back("None");
  model_modes.push_back("SimpleImpulse");
  model_modes.push_back("CenterWave");
  model_modes.push_back("CornerWave");
  model_modes.push_back("JungeInterference");
  model_modes.push_back("FlatWave");

  for (QString str_mode: model_modes)
    model_mode_combo->addItem(str_mode);

  connect(model_mode_combo, SIGNAL(currentIndexChanged(QString)), this, SLOT(changeModelMode(QString)));

  QComboBox *shade_mode_combo;
  grid_layout->addWidget(new QLabel("Shade mode"), 1, 0);
  grid_layout->addWidget(shade_mode_combo = new QComboBox(settings), 1, 1);
  shade_mode_combo->addItem("ADS mode");
  shade_mode_combo->addItem("Height map");
  connect(shade_mode_combo, SIGNAL(currentIndexChanged(QString)), this, SLOT(changeShadeMode(QString)));

  QDoubleSpinBox *z_scale_spinbox;
  grid_layout->addWidget(new QLabel("Z scale для Height\n map рендеринга"));
  grid_layout->addWidget(z_scale_spinbox = new QDoubleSpinBox(this));
  z_scale_spinbox->setMinimum(-1000);
  z_scale_spinbox->setMaximum(1000);
  z_scale_spinbox->setValue(z_scale);
  connect(z_scale_spinbox, SIGNAL(valueChanged(double)), this, SLOT(changeZScale(double)));

  grid_layout->setRowStretch(3, 1);
}

void Widget::initializeGL() {
  gl->initializeOpenGLFunctions();

  GLuint render_shaders[3] = {0, 0, 0};
  qDebug() << "Load vertex shader";
  render_shaders[0] = createShader(GL_VERTEX_SHADER, "./membrane_vibration.vert", is_error);
  qDebug() << "Geometry shader";
  render_shaders[1] = createShader(GL_GEOMETRY_SHADER, "./membrane_vibration.gsh", is_error);
  qDebug() << "Load fragment shader";
  render_shaders[2] = createShader(GL_FRAGMENT_SHADER, "./membrane_vibration.frag", is_error);

  qDebug() << "Load compute shader";
  GLuint compute_shader = createShader(GL_COMPUTE_SHADER,  "./membrane_vibration.glsl", is_error);

  if (is_error) {
    if (render_shaders[0] != 0) gl->glDeleteShader(render_shaders[0]);
    if (render_shaders[1] != 0) gl->glDeleteShader(render_shaders[1]);
    if (render_shaders[2] != 0) gl->glDeleteShader(render_shaders[1]);
    if (compute_shader != 0) gl->glDeleteShader(compute_shader);
    return;
  }

  qDebug() << "Linking render program";
  render_program = linkProgram(render_shaders, 3, is_error);

  qDebug() << "Linking compute program";
  compute_program = linkProgram(&compute_shader, 1, is_error);

  gl->glDeleteShader(render_shaders[0]);
  gl->glDeleteShader(render_shaders[1]);
  gl->glDeleteShader(render_shaders[2]);
  gl->glDeleteShader(compute_shader);
  if (is_error) return;

  qDebug() << "Shader program have been linked successfully";


  uint32_t *ebo_buf = new uint32_t[(size_x - 1)*(size_y - 1)*3*2]; // see above
  for (uint32_t i = 0; i < size_y - 1; ++i) {
    for (uint32_t j = 0; j < size_x - 1; ++j) {
      uint32_t c_idx = ((size_x - 1)*i + j)*3*2;

      ebo_buf[c_idx]     = size_x*i + j;
      ebo_buf[c_idx + 1] = size_x*i + j + 1;
      ebo_buf[c_idx + 2] = size_x*(i + 1) + j;

      ebo_buf[c_idx + 3] = size_x*i + j + 1;
      ebo_buf[c_idx + 4] = size_x*(i + 1) + j + 1;
      ebo_buf[c_idx + 5] = size_x*(i + 1) + j;
    }
  }

  gl->glGenBuffers(1, &ebo);
  gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  gl->glBufferData(GL_ELEMENT_ARRAY_BUFFER, (size_x - 1)*(size_y - 1)*3*2*sizeof(uint32_t),
                   ebo_buf, GL_STATIC_DRAW);

  delete[] ebo_buf;

  gl->glGenVertexArrays(1, &vao);
  initBufs();

  GLint shift_uni = gl->glGetUniformLocation(render_program, "shift");
  model_uni = gl->glGetUniformLocation(render_program, "model");
  GLint view_uni = gl->glGetUniformLocation(render_program, "view");
  GLint perspective_uni = gl->glGetUniformLocation(render_program, "perspective");

  QMatrix4x4 view;
  view.translate(0, 0, -2);
  QMatrix4x4 perspective;
  double aspect = double(width())/height();
  perspective.perspective(70, aspect, 0.01, 10);

  c_out_rend_uni = gl->glGetUniformLocation(render_program, "c_out");

  GLint l_pos = gl->glGetUniformLocation(render_program, "light.pos");
  GLint la = gl->glGetUniformLocation(render_program, "light.la");
  GLint ld = gl->glGetUniformLocation(render_program, "light.ld");
  GLint lr = gl->glGetUniformLocation(render_program, "light.lr");

  GLint ma = gl->glGetUniformLocation(render_program, "material.ma");
  GLint md = gl->glGetUniformLocation(render_program, "material.md");
  GLint mr = gl->glGetUniformLocation(render_program, "material.mr");
  GLint shininess = gl->glGetUniformLocation(render_program, "material.shininess");


  gl->glUseProgram(render_program);

  gl->glUniform3f(shift_uni, 0, 0, 0);
  gl->glUniformMatrix4fv(view_uni, 1, GL_FALSE, view.data());
  gl->glUniformMatrix4fv(perspective_uni, 1, GL_FALSE, perspective.data());

  gl->glUniform3f(l_pos, 0.0, 0.0, -2.0);
  gl->glUniform3f(la, 1.0, 1.0, 1.0);
  gl->glUniform3f(ld, 1.0, 1.0, 1.0);
  gl->glUniform3f(lr, 1.0, 1.0, 1.0);

  double ka = 0.2;
  double kd = 0.5;
  double kr = 0.8;

  gl->glUniform3f(ma, ka*1.0, ka*0.5, ka*0.0);
  gl->glUniform3f(md, kd*1.0, kd*0.5, kd*0.0);
  gl->glUniform3f(mr, kr*1.0, kr*1.0, kr*1.0);
  gl->glUniform1f(shininess, 50);

  gl->glUseProgram(0);

  dx = 1.0/(size_x - 1);
  dy = 1.0/(size_y - 1);
  v = 0.08;
  dt = 1.0/60;

  GLint sz_uni = gl->glGetUniformLocation(compute_program, "sz");
  c_out_uni = gl->glGetUniformLocation(compute_program, "c_out");

  GLint dx_uni  = gl->glGetUniformLocation(compute_program, "dx");
  GLint dy_uni  = gl->glGetUniformLocation(compute_program, "dy");
  GLint v_uni   = gl->glGetUniformLocation(compute_program, "v");
  GLint dt_uni  = gl->glGetUniformLocation(compute_program, "dt");
  GLint dx2_uni = gl->glGetUniformLocation(compute_program, "dx2");
  GLint dy2_uni = gl->glGetUniformLocation(compute_program, "dy2");
  GLint dt2_uni = gl->glGetUniformLocation(compute_program, "dt2");
  GLint v2_uni  = gl->glGetUniformLocation(compute_program, "v2");

  ADS_shade = gl->glGetSubroutineIndex(render_program, GL_FRAGMENT_SHADER,
                                       "ADSModel");
  hMap_shade = gl->glGetSubroutineIndex(render_program, GL_FRAGMENT_SHADER,
                                        "heightShadeMode");

  z_scale_uni = gl->glGetUniformLocation(render_program, "z_scale");

  gl->glUseProgram(compute_program);

  gl->glUniform2i(sz_uni, size_x, size_y);

  gl->glUniform1f(dx_uni, dx);
  gl->glUniform1f(dy_uni, dy);
  gl->glUniform1f(v_uni, v);
  gl->glUniform1f(dt_uni, dt);
  gl->glUniform1f(dx2_uni, dx*dx);
  gl->glUniform1f(dy2_uni, dy*dy);
  gl->glUniform1f(dt2_uni, dt*dt);
  gl->glUniform1f(v2_uni, v*v);

  gl->glUseProgram(0);

  connect(&timer, SIGNAL(timeout()), this, SLOT(renderTimeOut()));
  timer.start(1000*dt);
}

void Widget::renderTimeOut() {
  render_mode = true;
  repaint();
}

void Widget::changeModelMode(QString mode) {
  for (uint32_t i = 0; i < model_modes.size(); ++i) {
    if (mode == model_modes[i]) {
      model_mode = i;
      break;
    }
  }
  initBufs();
}

void Widget::changeShadeMode(QString mode) {
  if (mode == "ADS mode") shade_mode = 0;
  if (mode == "Height map") shade_mode = 1;
}

void Widget::changeZScale(double new_z_scale) {
  z_scale = new_z_scale;
}

void Widget::paintGL() {
  if (is_error || !render_mode) return;
  render_mode = false;

  gl->glUseProgram(compute_program);

  switch (model_mode) {
  case 1:
    calculationSampleImpulse();
    break;
  case 2:
    calculationCenterWave();
    break;
  case 3:
    calculationCornerWave();
    break;
  case 4:
    calculationJungeInterference();
    break;
  case 5:
    calculationFlatWave();
    break;
  }

  gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  gl->glEnable(GL_DEPTH_TEST);



  gl->glUseProgram(render_program);

  switch (shade_mode) {
  case 1:
    gl->glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &hMap_shade);
    break;
  default:
    gl->glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &ADS_shade);
    break;
  }

  gl->glUniform1f(z_scale_uni, z_scale);

  gl->glUniformMatrix3fv(model_uni, 1, GL_FALSE, model.data());

  gl->glUniform1i(c_out_rend_uni, c_out);

  gl->glBindVertexArray(vao);
  gl->glDrawElements(GL_TRIANGLES, (size_x - 1)*(size_y - 1)*3*2, GL_UNSIGNED_INT, NULL);
  gl->glBindVertexArray(0);

  gl->glUseProgram(0);
}

void Widget::mouseMoveEvent(QMouseEvent *event) {
  double sensitivity = 0.3;

  double dy = event->pos().y() - old_pos.y();
  z_angle += dy*sensitivity;
  z_angle = fmod(z_angle, 360);
  if (z_angle < 0) z_angle += 360;
  if (z_angle > 180) z_angle -= 360;
  QMatrix4x4 t_mat;
  t_mat.rotate(z_angle, 1, 0, 0);

  double dx = event->pos().x() - old_pos.x();
  if (z_angle > -180 && z_angle <= 0)
    x_angle += dx*sensitivity;
  else
    x_angle -= dx*sensitivity;

  x_angle = fmod(x_angle, 360);
  if (x_angle < 0) x_angle += 360;
  if (x_angle > 180) x_angle -= 360;
  t_mat.rotate(x_angle, 0, 0, 1);

  model = t_mat.toGenericMatrix<3, 3>();
  old_pos = event->pos();
}

void Widget::mousePressEvent(QMouseEvent *event) {
  old_pos = event->pos();
}

GLuint Widget::createShader(GLuint shader_type, const char *filename, bool &is_error,
                            t_dict_vec *dict_vec) {
  GLuint shader = 0;
  const char *shader_src = readFile(filename);
  if (shader_src == nullptr) {
    qDebug() << "Error loading shader source";
    is_error = true;
  }
  else {
    QString shader_txt(shader_src);
    if (dict_vec != nullptr) {
      for (t_dict_pair dict_pair: *dict_vec) {
        shader_txt = shader_txt.replace(dict_pair.first, dict_pair.second);
      }
    }
    delete shader_src;

    shader = gl->glCreateShader(shader_type);
    if (shader == 0) {
      qDebug() << "Shader creation is failed";
      is_error = true;
    }
    else {
      char *src = new char[shader_txt.length() + 1];
      memcpy(src, shader_txt.toStdString().c_str(), shader_txt.length());
      src[shader_txt.length()] = 0;
      gl->glShaderSource(shader, 1, &src, NULL);
      gl->glCompileShader(shader);
      delete[] src;

      GLint success = GL_FALSE;
      gl->glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
      if (success == GL_FALSE) {
        qDebug() << "Shader compilation is failed";
        is_error = true;

        GLint log_len = 0;
        gl->glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);
        if (log_len > 0) {
          char *log = new char[log_len];
          gl->glGetShaderInfoLog(shader, log_len, NULL, log);
          qDebug() << log;
          delete[] log;
        }
      }
    }
  }
  return shader;
}

GLuint Widget::linkProgram(GLuint *shaders, uint32_t shaders_count, bool &is_error) {
  GLuint program = gl->glCreateProgram();
  if (program == 0) {
    qDebug() << "Shader program creation os failed";
    is_error = true;
  }
  else {
    for (uint32_t i = 0; i < shaders_count; ++i)
      gl->glAttachShader(program, shaders[i]);

    gl->glLinkProgram(program);

    GLint success = GL_FALSE;
    gl->glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success == GL_FALSE) {
      qDebug() << "Error linking shader program";
      is_error = true;

      GLint log_len = 0;
      gl->glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_len);
      if (log_len > 0) {
        char *log = new char[log_len];
        gl->glGetProgramInfoLog(program, log_len, NULL, log);
        qDebug() << log;
        delete[] log;
      }
    }
  }
  return program;
}

const char *Widget::readFile(const char *filename) {
  std::ifstream file_in(filename, std::ios::binary);
  if (!file_in.is_open()) return nullptr;

  file_in.seekg(0, std::ios::end);
  uint32_t file_size = file_in.tellg();
  file_in.seekg(0, std::ios::beg);

  char *source = new char[file_size + 1];
  file_in.read(source, file_size);
  source[file_size] = 0;
  return source;
}


Widget::~Widget() {
  delete gl;
}


void Widget::initBufs() {
  float *u_init = new float[4*(size_x)*(size_y)];
  float *xy_buf = new float[4*(size_x)*(size_y)];

  float dx = 1.0/(size_x - 1);
  float dy = 1.0/(size_y - 1);

  for (uint32_t i = 0; i < size_y; ++i) {
    for (uint32_t j = 0; j < size_x; ++j) {
      uint32_t c_idx = 4*(size_x*i + j);

      u_init[c_idx]     = 0;
      u_init[c_idx + 1] = 0;
      u_init[c_idx + 2] = 0;
      u_init[c_idx + 3] = 0;

      xy_buf[c_idx]     = dx*j*2 - 1.0;
      xy_buf[c_idx + 1] = dy*i*2 - 1.0;
      xy_buf[c_idx + 2] = 0;
      xy_buf[c_idx + 3] = 0;
    }
  }

  switch (model_mode) {
  case 1:
    initSampleImpulse(u_init, xy_buf);
    break;
  case 2:
    initCenterWave(u_init, xy_buf);
    break;
  case 3:
    initCornerWave(u_init, xy_buf);
    break;
  case 4:
    initJungeInterference(u_init, xy_buf);
    break;
  case 5:
    initFlatWave(u_init, xy_buf);
    break;
  }


  // Граничные точки устанавливаються фиксироваными
  // со значениями по оси z равными нулю
  for (uint32_t i = 0; i < size_y; ++i) {
    u_init[4*size_x*i + 3] = 1;
    u_init[4*(size_x*i + (size_x - 1)) + 3] = 1;
  }

  for (uint32_t j = 0; j < size_x; ++j) {
    u_init[4*j + 3] = 1;
    u_init[4*(size_x*(size_y-1) + j) + 3] = 1;
  }


  if (u_vbo == 0) {
    gl->glGenBuffers(1, &u_vbo);
    gl->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, u_vbo);
    gl->glBufferData(GL_SHADER_STORAGE_BUFFER, 4*size_x*size_y*sizeof(float),
                     u_init, GL_DYNAMIC_DRAW);
  }
  else {
    gl->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, u_vbo);
    gl->glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, 4*size_x*size_y*sizeof(float),
                        u_init);
  }


  if (xy_vbo == 0) {
    gl->glGenBuffers(1, &xy_vbo);
    gl->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, xy_vbo);
    gl->glBufferData(GL_SHADER_STORAGE_BUFFER, 4*size_x*size_y*sizeof(float),
                     xy_buf, GL_STATIC_DRAW);
  }
  else {
    gl->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, xy_vbo);
    gl->glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, 4*size_x*size_y*sizeof(float),
                        xy_buf);
  }

  gl->glBindVertexArray(vao);
  gl->glEnableVertexAttribArray(0);
  gl->glEnableVertexAttribArray(1);

  gl->glBindBuffer(GL_ARRAY_BUFFER, u_vbo);
  gl->glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL);

  gl->glBindBuffer(GL_ARRAY_BUFFER, xy_vbo);
  gl->glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, NULL);

  gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

  gl->glBindVertexArray(0);

  c_out = 1;
  ti = 0;

  delete[] u_init;
  delete[] xy_buf;
}


void Widget::initSampleImpulse(float *u_init, float *) {
  // Положение центральной точки по оси z устанавливается
  // в 1, в дальнейшем отпускается.
  // Полученные решения волнового уравнений на разных
  // временных шагах дадут импульсную характеристику
  // полученной модели
  u_init[4*(size_x*size_y/2 + size_x/2) + 1] = 1;
}

void Widget::calculationSampleImpulse() {
  for (uint32_t i = 0; i < speed_factor; ++i) {
    c_out = (c_out - 1) % 3;
    if (c_out < 0) c_out += 3;
    gl->glUniform1i(c_out_uni, c_out);

    gl->glDispatchCompute(size_x/10, size_y/10, 1);
    gl->glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
  }
}


void Widget::initCenterWave(float *, float *xy_buf) {
  // Изначальное положение всех точек мембраны по
  // оси z нулевые, но коэффициент затухания волны
  // в них равен 2E-4
  // Функция calculationCenterWave сдвигает
  // центральную вершину мембраны по оси z в
  // соответвии с функцией sin. В результате
  // пораждается волна исходящаю их центральной
  // точки
  for (uint32_t i = 0; i < size_y; ++i) {
    for (uint32_t j = 0; j < size_x; ++j) {
      xy_buf[4*(size_x*i + j) + 2] = 2E-4;
    }
  }
}

void Widget::calculationCenterWave() {
    for (uint32_t i = 0; i < speed_factor; ++i) {
      uint32_t vi = size_y/2;
      uint32_t vj = size_x/2;
      uint32_t idx = vi*size_x + vj;
      float u[4] = {0, 0, 0, 1};
      ti++;
      u[c_out] = 0.5*sin(1*2*M_PI*ti*dt);

      c_out = (c_out - 1) % 3;
      if (c_out < 0) c_out += 3;
      gl->glUniform1i(c_out_uni, c_out);

      gl->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, u_vbo);
      gl->glBufferSubData(GL_SHADER_STORAGE_BUFFER, 4*idx*sizeof(float), 4*sizeof(float), u);

      gl->glDispatchCompute(size_x/10, size_y/10, 1);
      gl->glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }
}


void Widget::initCornerWave(float *, float *xy_buf) {
  // Изначальное положение всех точек мембраны по
  // оси z нулевые, но коэффициент затухания волны
  // в них равен 1E-4
  // Функция calculationCenterWave сдвигает
  // угловую вершину мембраны по оси z в соответвии
  // с функцией sin. В результате пораждается
  // волна исходящаю их угловой точки
  for (uint32_t i = 0; i < size_y; ++i) {
    for (uint32_t j = 0; j < size_x; ++j) {
      xy_buf[4*(size_x*i + j) + 2] = 1E-4;
    }
  }
}

void Widget::calculationCornerWave() {
    for (uint32_t i = 0; i < speed_factor; ++i) {
      uint32_t vi = 10;
      uint32_t vj = 10;
      uint32_t idx = vi*size_x + vj;
      float u[4] = {0, 0, 0, 1};
      ti++;
      u[c_out] = 0.5*sin(1*2*M_PI*ti*dt);

      c_out = (c_out - 1) % 3;
      if (c_out < 0) c_out += 3;
      gl->glUniform1i(c_out_uni, c_out);

      gl->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, u_vbo);
      gl->glBufferSubData(GL_SHADER_STORAGE_BUFFER, 4*idx*sizeof(float), 4*sizeof(float), u);

      gl->glDispatchCompute(size_x/10, size_y/10, 1);
      gl->glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }
}


void Widget::initJungeInterference(float *u_init, float *xy_buf) {
  // Изначально положение всех точек мембраны
  // по оси z нулевые, но коэффициент затухания
  // волны в них равен 1E-4
  for (uint32_t i = 0; i < size_y; ++i) {
    for (uint32_t j = 0; j < size_x; ++j) {
      xy_buf[4*(size_x*i + j) + 2] = 1E-4;
    }
  }

  // Часть точек по оси х фиксируються, образую стену
  // Они не учавствуют в распространении волны
  for (uint32_t i = 0; i < size_y; ++i) {
    u_init[4*(size_x*i + size_y/4) + 3] = 1;
  }

  uint32_t sh = 200;

  // Часть фиксированных точек опять освобождаються,
  // при этом они снова принимают участие в распротранении
  // волны.
  for (uint32_t i = sh - 10; i < sh + 10; ++i) {
    u_init[4*(size_x*i + size_y/4) + 3] = 0;
  }
  for (uint32_t i = 500 - sh - 10; i < 500 - sh + 10; ++i) {
    u_init[4*(size_x*i + size_y/4) + 3] = 0;
  }

  // Освобожденные точки вместе с фиксированными образуют
  // стену с вдумя щелями вдоль оси Х.
  // Функция calculationJungeInterference пораждает
  // волну напротив стены между щелями. Таким образом
  // моделируеться двухщелевой опыт Юнга.
}

void Widget::calculationJungeInterference() {
  for (uint32_t i = 0; i < speed_factor; ++i) {
    uint32_t vi = size_y/2;
    uint32_t vj = 10;
    uint32_t idx = vi*size_x + vj;
    float u[4] = {0, 0, 0, 1};
    ti++;
    u[c_out] = 1*sin(2*2*M_PI*ti*dt);

    c_out = (c_out - 1) % 3;
    if (c_out < 0) c_out += 3;
    gl->glUniform1i(c_out_uni, c_out);

    gl->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, u_vbo);
    gl->glBufferSubData(GL_SHADER_STORAGE_BUFFER, 4*idx*sizeof(float), 4*sizeof(float), u);

    gl->glDispatchCompute(size_x/10, size_y/10, 1);
    gl->glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
  }
}

void Widget::initFlatWave(float *, float *xy_buf) {
  // Изначальное положение всех точек мембраны по
  // оси z нулевые, но коэффициент затухания волны
  // в них равен 2.5E-5
  // Функция calculationFlatWave смещает точки
  // отрезка на краю по оси z в соответвии
  // с функцией sin. Таким образом на краю мембраны
  // пораждаеться плоская волна
  for (uint32_t i = 0; i < size_y; ++i) {
    for (uint32_t j = 0; j < size_x; ++j) {
      xy_buf[4*(size_x*i + j) + 2] = 2.5E-5;
    }
  }
}

void Widget::calculationFlatWave() {

  uint32_t y_idx = 10;
  uint32_t w = 200;
  uint32_t bx_idx = size_x/2 - w/2;

  for (uint32_t i = 0; i < speed_factor; ++i) {

    float *u = new float[4*(w + 1)];

    for (uint32_t j = 0; j <= w; ++j) {
      u[4*j + 0] = u[4*j+ 1] = u[4*j+ 2] = 0;
      u[4*j+ 3] = 1;
      u[4*j+ c_out] = 0.07*sin(0.5*2*M_PI*ti*dt);
    }
    ti++;

    c_out = (c_out - 1) % 3;
    if (c_out < 0) c_out += 3;
    gl->glUniform1i(c_out_uni, c_out);

    gl->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, u_vbo);
    gl->glBufferSubData(GL_SHADER_STORAGE_BUFFER, 4*(y_idx*size_x + bx_idx)*sizeof(float),
                        4*(w + 1)*sizeof(float), u);
    delete[] u;

    gl->glDispatchCompute(size_x/10, size_y/10, 1);
    gl->glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
  }

}


//
//   *----*----*----*----*----*
//   |\   |\   |\   |\   |\   |
//   | \  | \  | \  | \  | \  |                     ----+
//   |  \ |  \ |  \ |  \ |  \ |   (size_x)*2*3           |
//   |   \|   \|   \|   \|   \|                         |
//   *----*----*----*----*----*                         |
//   |\   |\   |\   |\   |\   |                         |
//   | \  | \  | \  | \  | \  |   (size_x)*2*3          |
//   |  \ |  \ |  \ |  \ |  \ |                         |
//   |   \|   \|   \|   \|   \|                         |
//   *----*----*----*----*----*                         |-- (size_y)
//   |\   |\   |\   |\   |\   |                         |
//   | \  | \  | \  | \  | \  |   (size_x)*2*3          |
//   |  \ |  \ |  \ |  \ |  \ |                         |
//   |   \|   \|   \|   \|   \|                         |
//   *----*----*----*----*----*                         |
//   |\   |\   |\   |\   |\   |                         |
//   | \  | \  | \  | \  | \  |   (size_x)*2*3          |
//   |  \ |  \ |  \ |  \ |  \ |                     ----+
//   |   \|   \|   \|   \|   \|
//   *----*----*----*----*----*
