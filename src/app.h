#ifndef APP_H
#define APP_H

#include <nanogui/nanogui.h>
#include <memory>
#include "camera.h"
#include "collision/collisionObject.h"
#include "hair/hairSystem.h"
#include "core/wind.h"

using namespace nanogui;

struct UserShader;
enum ShaderTypeHint { WIREFRAME = 0, NORMALS = 1, PHONG = 2, FUR = 3 };

class App {
public:
  App(std::string project_root, Screen *screen);
  ~App();

  void init();

  void loadHairSystem(HairSystem *hair_system);
  void loadHairParameters(HairParameters *hp);
  void loadCollisionObjects(vector<CollisionObject *> *objects);

  virtual bool isAlive();
  virtual void drawContents();

  // Screen events
  virtual bool cursorPosCallbackEvent(double x, double y);
  virtual bool mouseButtonCallbackEvent(int button, int action, int modifiers);
  virtual bool keyCallbackEvent(int key, int scancode, int action, int mods);
  virtual bool dropCallbackEvent(int count, const char **filenames);
  virtual bool scrollCallbackEvent(double x, double y);
  virtual bool resizeCallbackEvent(int width, int height);

private:
  virtual void initGUI(Screen *screen);
  void drawWireframe(GLShader &shader);
  void drawHair(GLShader &shader);
  void load_shaders();
  void load_textures();

  std::string m_project_root;

  // Camera
  virtual void resetCamera();
  virtual Matrix4f getProjectionMatrix();
  virtual Matrix4f getViewMatrix();

  // Simulation defaults
  int frames_per_sec = 90;
  int simulation_steps = 30;
  CGL::Vector3D gravity = CGL::Vector3D(0, -9.8, 0);
  nanogui::Color color = nanogui::Color(1.0f, 1.0f, 1.0f, 1.0f);

  // Simulation objects
  HairSystem *hair_system = nullptr;
  HairParameters *hp = nullptr;
  vector<CollisionObject *> *collision_objects = nullptr;

  // Shaders
  int active_shader_idx = 0;
  vector<UserShader> shaders;
  vector<std::string> shaders_combobox_names;

  // Textures
  Vector3D m_gl_texture_1_size, m_gl_texture_2_size;
  Vector3D m_gl_texture_3_size, m_gl_texture_4_size;
  GLuint m_gl_texture_1, m_gl_texture_2, m_gl_texture_3, m_gl_texture_4;
  GLuint m_gl_cubemap_tex;

  double m_normal_scaling = 2.0;
  double m_height_scaling = 0.1;

  // Camera
  CGL::Camera camera, canonicalCamera;
  double view_distance, canonical_view_distance, min_view_distance, max_view_distance;
  double scroll_rate;

  Screen *screen;
  void mouseLeftDragged(double x, double y);
  void mouseRightDragged(double x, double y);
  void mouseMoved(double x, double y);

  bool left_down = false, right_down = false, middle_down = false;
  bool ctrl_down = false;
  bool is_paused = true;

  // Object movement (ported from HW4 EC)
  int selected_object_idx = -1;
  double move_speed = 0.02;

  int mouse_x, mouse_y, screen_w, screen_h;
  bool is_alive = true;
  Vector2i default_window_size = Vector2i(1024, 800);
};

struct UserShader {
  UserShader(std::string display_name, std::shared_ptr<GLShader> nanogui_shader,
             ShaderTypeHint type_hint)
      : display_name(display_name), nanogui_shader(nanogui_shader),
        type_hint(type_hint) {}
  std::shared_ptr<GLShader> nanogui_shader;
  std::string display_name;
  ShaderTypeHint type_hint;
};

#endif /* APP_H */
