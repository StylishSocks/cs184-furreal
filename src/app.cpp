#include <cmath>
#include <glad/glad.h>

#include <CGL/vector3D.h>
#include <nanogui/nanogui.h>

#include "app.h"

#include "camera.h"
#include "collision/plane.h"
#include "collision/sphere.h"
#include "misc/camera_info.h"
#include "misc/file_utils.h"
// Needed to generate stb_image binaries. Should only define in exactly one source file.
#define STB_IMAGE_IMPLEMENTATION
#include "misc/stb_image.h"

using namespace nanogui;
using namespace std;

Vector3D load_texture(int frame_idx, GLuint handle, const char* where) {
  Vector3D size_retval;

  if (strlen(where) == 0) return size_retval;

  glActiveTexture(GL_TEXTURE0 + frame_idx);
  glBindTexture(GL_TEXTURE_2D, handle);

  int img_x, img_y, img_n;
  unsigned char* img_data = stbi_load(where, &img_x, &img_y, &img_n, 3);
  size_retval.x = img_x;
  size_retval.y = img_y;
  size_retval.z = img_n;
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img_x, img_y, 0, GL_RGB, GL_UNSIGNED_BYTE, img_data);
  stbi_image_free(img_data);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  return size_retval;
}

void load_cubemap(int frame_idx, GLuint handle, const std::vector<std::string>& file_locs) {
  glActiveTexture(GL_TEXTURE0 + frame_idx);
  glBindTexture(GL_TEXTURE_CUBE_MAP, handle);
  for (int side_idx = 0; side_idx < 6; ++side_idx) {
    int img_x, img_y, img_n;
    unsigned char* img_data = stbi_load(file_locs[side_idx].c_str(), &img_x, &img_y, &img_n, 3);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + side_idx, 0, GL_RGB, img_x, img_y, 0, GL_RGB, GL_UNSIGNED_BYTE, img_data);
    stbi_image_free(img_data);
    std::cout << "Side " << side_idx << " has dimensions " << img_x << ", " << img_y << std::endl;

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  }
}

void App::load_textures() {
  glGenTextures(1, &m_gl_texture_1);
  glGenTextures(1, &m_gl_texture_2);
  glGenTextures(1, &m_gl_texture_3);
  glGenTextures(1, &m_gl_texture_4);
  glGenTextures(1, &m_gl_cubemap_tex);

  m_gl_texture_1_size = load_texture(1, m_gl_texture_1, (m_project_root + "/textures/texture_1.png").c_str());
  m_gl_texture_2_size = load_texture(2, m_gl_texture_2, (m_project_root + "/textures/texture_2.png").c_str());
  m_gl_texture_3_size = load_texture(3, m_gl_texture_3, (m_project_root + "/textures/texture_3.png").c_str());
  m_gl_texture_4_size = load_texture(4, m_gl_texture_4, (m_project_root + "/textures/texture_4.png").c_str());

  std::cout << "Texture 1 loaded with size: " << m_gl_texture_1_size << std::endl;
  std::cout << "Texture 2 loaded with size: " << m_gl_texture_2_size << std::endl;
  std::cout << "Texture 3 loaded with size: " << m_gl_texture_3_size << std::endl;
  std::cout << "Texture 4 loaded with size: " << m_gl_texture_4_size << std::endl;

  std::vector<std::string> cubemap_fnames = {
    m_project_root + "/textures/cube/posx.jpg",
    m_project_root + "/textures/cube/negx.jpg",
    m_project_root + "/textures/cube/posy.jpg",
    m_project_root + "/textures/cube/negy.jpg",
    m_project_root + "/textures/cube/posz.jpg",
    m_project_root + "/textures/cube/negz.jpg"
  };

  load_cubemap(5, m_gl_cubemap_tex, cubemap_fnames);
  std::cout << "Loaded cubemap texture" << std::endl;
}

void App::load_shaders() {
  std::set<std::string> shader_folder_contents;
  bool success = FileUtils::list_files_in_directory(m_project_root + "/shaders", shader_folder_contents);
  if (!success) {
    std::cout << "Error: Could not find the shaders folder!" << std::endl;
  }

  std::string std_vert_shader = m_project_root + "/shaders/Default.vert";

  for (const std::string& shader_fname : shader_folder_contents) {
    std::string file_extension;
    std::string shader_name;

    FileUtils::split_filename(shader_fname, shader_name, file_extension);

    if (file_extension != "frag") {
      std::cout << "Skipping non-shader file: " << shader_fname << std::endl;
      continue;
    }

    std::cout << "Found shader file: " << shader_fname << std::endl;

    // Check if there is a proper .vert shader or not for it
    std::string vert_shader = std_vert_shader;
    std::string associated_vert_shader_path = m_project_root + "/shaders/" + shader_name + ".vert";
    if (FileUtils::file_exists(associated_vert_shader_path)) {
      vert_shader = associated_vert_shader_path;
    }

    std::shared_ptr<GLShader> nanogui_shader = make_shared<GLShader>();
    nanogui_shader->initFromFiles(shader_name, vert_shader,
                                  m_project_root + "/shaders/" + shader_fname);

    ShaderTypeHint hint;
    if (shader_name == "Wireframe") {
      hint = ShaderTypeHint::WIREFRAME;
      std::cout << "Type: Wireframe" << std::endl;
    } else if (shader_name == "Normal") {
      hint = ShaderTypeHint::NORMALS;
      std::cout << "Type: Normal" << std::endl;
    } else if (shader_name == "Fur") {
      hint = ShaderTypeHint::FUR;
      std::cout << "Type: Fur" << std::endl;
    } else {
      hint = ShaderTypeHint::PHONG;
      std::cout << "Type: Custom" << std::endl;
    }

    UserShader user_shader(shader_name, nanogui_shader, hint);

    shaders.push_back(user_shader);
    shaders_combobox_names.push_back(shader_name);
  }

  // Default to Fur shader when available, fallback to Wireframe.
  for (size_t i = 0; i < shaders_combobox_names.size(); ++i) {
    if (shaders_combobox_names[i] == "Fur") {
      active_shader_idx = i;
      break;
    }
  }
  if (active_shader_idx == 0 && !shaders_combobox_names.empty() &&
      shaders_combobox_names[0] != "Fur") {
    for (size_t i = 0; i < shaders_combobox_names.size(); ++i) {
      if (shaders_combobox_names[i] == "Wireframe") {
        active_shader_idx = i;
        break;
      }
    }
  }
}

App::App(std::string project_root, Screen *screen)
: m_project_root(project_root) {
  this->screen = screen;

  this->load_shaders();
  this->load_textures();

  glEnable(GL_PROGRAM_POINT_SIZE);
  glEnable(GL_DEPTH_TEST);
}

App::~App() {
  for (auto shader : shaders) {
    shader.nanogui_shader->free();
  }
  glDeleteTextures(1, &m_gl_texture_1);
  glDeleteTextures(1, &m_gl_texture_2);
  glDeleteTextures(1, &m_gl_texture_3);
  glDeleteTextures(1, &m_gl_texture_4);
  glDeleteTextures(1, &m_gl_cubemap_tex);

  if (hair_system) delete hair_system;
  if (hp) delete hp;
  if (collision_objects) delete collision_objects;
}

void App::loadHairSystem(HairSystem *hs) { this->hair_system = hs; }
void App::loadHairParameters(HairParameters *hp) { this->hp = hp; }
void App::loadCollisionObjects(vector<CollisionObject *> *objects) {
  this->collision_objects = objects;
}

void App::init() {
  screen->setSize(default_window_size);
  initGUI(screen);

  // Initialize camera
  CGL::Collada::CameraInfo camera_info;
  camera_info.hFov = 50;
  camera_info.vFov = 35;
  camera_info.nClip = 0.01;
  camera_info.fClip = 10000;

  // Default camera target: center of scene
  CGL::Vector3D target(0.5, 0.5, 0.5);
  CGL::Vector3D c_dir(0., 0., 0.);
  canonical_view_distance = 1.5;
  scroll_rate = canonical_view_distance / 10;

  view_distance = canonical_view_distance * 2;
  min_view_distance = canonical_view_distance / 10.0;
  max_view_distance = canonical_view_distance * 20.0;

  camera.place(target, acos(c_dir.y), atan2(c_dir.x, c_dir.z), view_distance,
               min_view_distance, max_view_distance);
  canonicalCamera.place(target, acos(c_dir.y), atan2(c_dir.x, c_dir.z),
                        view_distance, min_view_distance, max_view_distance);

  screen_w = default_window_size(0);
  screen_h = default_window_size(1);

  camera.configure(camera_info, screen_w, screen_h);
  canonicalCamera.configure(camera_info, screen_w, screen_h);
}

bool App::isAlive() { return is_alive; }

void App::drawContents() {
  glEnable(GL_DEPTH_TEST);

  if (!is_paused && hair_system) {
    vector<CGL::Vector3D> external_accelerations = {gravity};

    for (int i = 0; i < simulation_steps; i++) {
      hair_system->simulate(frames_per_sec, simulation_steps, hp,
                            external_accelerations, collision_objects);
    }
  }

  // Bind the active shader
  const UserShader& active_shader = shaders[active_shader_idx];
  GLShader &shader = *active_shader.nanogui_shader;
  shader.bind();

  // Prepare the camera projection matrix
  Matrix4f model;
  model.setIdentity();

  Matrix4f view = getViewMatrix();
  Matrix4f projection = getProjectionMatrix();
  Matrix4f viewProjection = projection * view;

  shader.setUniform("u_model", model);
  shader.setUniform("u_view_projection", viewProjection);

  switch (active_shader.type_hint) {
  case WIREFRAME:
    shader.setUniform("u_color", color, false);
    drawWireframe(shader);
    break;
  case NORMALS:
    drawHair(shader);
    break;
  case PHONG: {
    Vector3D cam_pos = camera.position();
    shader.setUniform("u_color", color, false);
    shader.setUniform("u_cam_pos", Vector3f(cam_pos.x, cam_pos.y, cam_pos.z), false);
    shader.setUniform("u_light_pos", Vector3f(0.5, 2, 2), false);
    shader.setUniform("u_light_intensity", Vector3f(3, 3, 3), false);
    shader.setUniform("u_texture_1_size", Vector2f(m_gl_texture_1_size.x, m_gl_texture_1_size.y), false);
    shader.setUniform("u_texture_2_size", Vector2f(m_gl_texture_2_size.x, m_gl_texture_2_size.y), false);
    shader.setUniform("u_texture_3_size", Vector2f(m_gl_texture_3_size.x, m_gl_texture_3_size.y), false);
    shader.setUniform("u_texture_4_size", Vector2f(m_gl_texture_4_size.x, m_gl_texture_4_size.y), false);
    shader.setUniform("u_texture_1", 1, false);
    shader.setUniform("u_texture_2", 2, false);
    shader.setUniform("u_texture_3", 3, false);
    shader.setUniform("u_texture_4", 4, false);
    shader.setUniform("u_normal_scaling", m_normal_scaling, false);
    shader.setUniform("u_height_scaling", m_height_scaling, false);
    shader.setUniform("u_texture_cubemap", 5, false);
    drawHair(shader);
    break;
  }
  case FUR: {
    Vector3D cam_pos = camera.position();
    shader.setUniform("u_cam_pos", Vector3f(cam_pos.x, cam_pos.y, cam_pos.z), false);
    shader.setUniform("u_light_pos", Vector3f(0.5f, 2.0f, 2.0f), false);
    shader.setUniform("u_light_intensity", Vector3f(3.0f, 3.0f, 3.0f), false);
    shader.setUniform("u_root_color", Vector3f(color.r(), color.g(), color.b()), false);
    shader.setUniform("u_tip_color",
                      Vector3f(color.r() * 0.72f, color.g() * 0.72f,
                               color.b() * 0.72f),
                      false);
    shader.setUniform("u_alpha", hp ? hp->fur_render.alpha : 0.90f, false);
    shader.setUniform("u_primary_spec_power",
                      hp ? hp->fur_render.primary_spec_power : 70.0f, false);
    shader.setUniform("u_secondary_spec_power",
                      hp ? hp->fur_render.secondary_spec_power : 20.0f, false);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    drawHair(shader);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    break;
  }
  }

  // Render collision objects
  if (collision_objects) {
    for (CollisionObject *co : *collision_objects) {
      co->render(shader);
    }
  }
}

void App::drawWireframe(GLShader &shader) {
  // Render hair strands as wireframe lines
  if (hair_system && hp) {
    HairParameters debug_params = *hp;
    debug_params.render_strategy = HairRenderStrategy::DEBUG_LINES;
    hair_system->render(shader, &debug_params, camera.position());
  }
}

void App::drawHair(GLShader &shader) {
  // Render hair strands
  if (hair_system && hp) {
    hair_system->render(shader, hp, camera.position());
  }
}

// ----------------------------------------------------------------------------
// CAMERA CALCULATIONS
// ----------------------------------------------------------------------------

void App::resetCamera() { camera.copy_placement(canonicalCamera); }

Matrix4f App::getProjectionMatrix() {
  Matrix4f perspective;
  perspective.setZero();

  double cam_near = camera.near_clip();
  double cam_far = camera.far_clip();

  double theta = camera.v_fov() * M_PI / 360;
  double range = cam_far - cam_near;
  double invtan = 1. / tanf(theta);

  perspective(0, 0) = invtan / camera.aspect_ratio();
  perspective(1, 1) = invtan;
  perspective(2, 2) = -(cam_near + cam_far) / range;
  perspective(3, 2) = -1;
  perspective(2, 3) = -2 * cam_near * cam_far / range;
  perspective(3, 3) = 0;

  return perspective;
}

Matrix4f App::getViewMatrix() {
  Matrix4f lookAt;
  Matrix3f R;

  lookAt.setZero();

  CGL::Vector3D c_pos = camera.position();
  CGL::Vector3D c_udir = camera.up_dir();
  CGL::Vector3D c_target = camera.view_point();

  Vector3f eye(c_pos.x, c_pos.y, c_pos.z);
  Vector3f up(c_udir.x, c_udir.y, c_udir.z);
  Vector3f target(c_target.x, c_target.y, c_target.z);

  R.col(2) = (eye - target).normalized();
  R.col(0) = up.cross(R.col(2)).normalized();
  R.col(1) = R.col(2).cross(R.col(0));

  lookAt.topLeftCorner<3, 3>() = R.transpose();
  lookAt.topRightCorner<3, 1>() = -R.transpose() * eye;
  lookAt(3, 3) = 1.0f;

  return lookAt;
}

// ----------------------------------------------------------------------------
// EVENT HANDLING
// ----------------------------------------------------------------------------

bool App::cursorPosCallbackEvent(double x, double y) {
  if (left_down && !middle_down && !right_down) {
    if (ctrl_down) {
      mouseRightDragged(x, y);
    } else {
      mouseLeftDragged(x, y);
    }
  } else if (!left_down && !middle_down && right_down) {
    mouseRightDragged(x, y);
  } else if (!left_down && !middle_down && !right_down) {
    mouseMoved(x, y);
  }

  mouse_x = x;
  mouse_y = y;

  return true;
}

bool App::mouseButtonCallbackEvent(int button, int action, int modifiers) {
  switch (action) {
  case GLFW_PRESS:
    switch (button) {
    case GLFW_MOUSE_BUTTON_LEFT:
      left_down = true;
      break;
    case GLFW_MOUSE_BUTTON_MIDDLE:
      middle_down = true;
      break;
    case GLFW_MOUSE_BUTTON_RIGHT:
      right_down = true;
      break;
    }
    return true;

  case GLFW_RELEASE:
    switch (button) {
    case GLFW_MOUSE_BUTTON_LEFT:
      left_down = false;
      break;
    case GLFW_MOUSE_BUTTON_MIDDLE:
      middle_down = false;
      break;
    case GLFW_MOUSE_BUTTON_RIGHT:
      right_down = false;
      break;
    }
    return true;
  }

  return false;
}

void App::mouseMoved(double x, double y) { y = screen_h - y; }

void App::mouseLeftDragged(double x, double y) {
  float dx = x - mouse_x;
  float dy = y - mouse_y;
  camera.rotate_by(-dy * (M_PI / screen_h), -dx * (M_PI / screen_w));
}

void App::mouseRightDragged(double x, double y) {
  camera.move_by(mouse_x - x, y - mouse_y, canonical_view_distance);
}

bool App::keyCallbackEvent(int key, int scancode, int action, int mods) {
  ctrl_down = (bool)(mods & GLFW_MOD_CONTROL);

  if (action == GLFW_PRESS) {
    switch (key) {
    case GLFW_KEY_ESCAPE:
      is_alive = false;
      break;
    case 'r':
    case 'R':
      if (hair_system) hair_system->reset();
      break;
    case ' ':
      resetCamera();
      break;
    case 'p':
    case 'P':
      is_paused = !is_paused;
      break;
    case 'n':
    case 'N':
      if (is_paused) {
        is_paused = false;
        drawContents();
        is_paused = true;
      }
      break;

    // Object movement (ported from HW4 EC)
    case GLFW_KEY_TAB:
      if (collision_objects && !collision_objects->empty()) {
        selected_object_idx = (selected_object_idx + 1) % (int)collision_objects->size();
        std::cout << "Selected object " << selected_object_idx << ": "
                  << (*collision_objects)[selected_object_idx]->getType() << std::endl;
      }
      break;
    case GLFW_KEY_UP:
      if (selected_object_idx >= 0 && selected_object_idx < (int)collision_objects->size()) {
        (*collision_objects)[selected_object_idx]->translate(CGL::Vector3D(0, 0, -move_speed));
      }
      break;
    case GLFW_KEY_DOWN:
      if (selected_object_idx >= 0 && selected_object_idx < (int)collision_objects->size()) {
        (*collision_objects)[selected_object_idx]->translate(CGL::Vector3D(0, 0, move_speed));
      }
      break;
    case GLFW_KEY_LEFT:
      if (selected_object_idx >= 0 && selected_object_idx < (int)collision_objects->size()) {
        (*collision_objects)[selected_object_idx]->translate(CGL::Vector3D(-move_speed, 0, 0));
      }
      break;
    case GLFW_KEY_RIGHT:
      if (selected_object_idx >= 0 && selected_object_idx < (int)collision_objects->size()) {
        (*collision_objects)[selected_object_idx]->translate(CGL::Vector3D(move_speed, 0, 0));
      }
      break;
    case GLFW_KEY_LEFT_BRACKET:
      if (selected_object_idx >= 0 && selected_object_idx < (int)collision_objects->size()) {
        (*collision_objects)[selected_object_idx]->translate(CGL::Vector3D(0, -move_speed, 0));
      }
      break;
    case GLFW_KEY_RIGHT_BRACKET:
      if (selected_object_idx >= 0 && selected_object_idx < (int)collision_objects->size()) {
        (*collision_objects)[selected_object_idx]->translate(CGL::Vector3D(0, move_speed, 0));
      }
      break;
    }
  }

  return true;
}

bool App::dropCallbackEvent(int count, const char **filenames) {
  return true;
}

bool App::scrollCallbackEvent(double x, double y) {
  camera.move_forward(y * scroll_rate);
  return true;
}

bool App::resizeCallbackEvent(int width, int height) {
  screen_w = width;
  screen_h = height;
  camera.set_screen_size(screen_w, screen_h);
  return true;
}

void App::initGUI(Screen *screen) {
  Window *window;

  window = new Window(screen, "Simulation");
  window->setPosition(Vector2i(default_window_size(0) - 245, 15));
  window->setLayout(new GroupLayout(15, 6, 14, 5));

  auto request_rebuild = [this]() {
    if (hair_system && hp) {
      hair_system->rebuild(hp, collision_objects);
    }
  };

  // Hair parameters

  new Label(window, "Hair Parameters", "sans-bold");

  {
    Widget *panel = new Widget(window);
    GridLayout *layout =
        new GridLayout(Orientation::Horizontal, 2, Alignment::Middle, 5, 5);
    layout->setColAlignment({Alignment::Maximum, Alignment::Fill});
    layout->setSpacing(0, 10);
    panel->setLayout(layout);

    new Label(panel, "strands :", "sans-bold");

    IntBox<int> *fb = new IntBox<int>(panel);
    fb->setEditable(true);
    fb->setFixedSize(Vector2i(100, 20));
    fb->setFontSize(14);
    fb->setValue(hp ? hp->num_strands : 1000);
    fb->setSpinnable(true);
    fb->setMinValue(1);
    fb->setCallback([this, request_rebuild](int value) {
      if (hp) {
        hp->num_strands = value;
        request_rebuild();
      }
    });

    new Label(panel, "particles :", "sans-bold");

    fb = new IntBox<int>(panel);
    fb->setEditable(true);
    fb->setFixedSize(Vector2i(100, 20));
    fb->setFontSize(14);
    fb->setValue(hp ? hp->particles_per_strand : 10);
    fb->setSpinnable(true);
    fb->setMinValue(2);
    fb->setCallback([this, request_rebuild](int value) {
      if (hp) {
        hp->particles_per_strand = value;
        request_rebuild();
      }
    });

    new Label(panel, "length :", "sans-bold");

    FloatBox<double> *ffb = new FloatBox<double>(panel);
    ffb->setEditable(true);
    ffb->setFixedSize(Vector2i(100, 20));
    ffb->setFontSize(14);
    ffb->setValue(hp ? hp->strand_length : 0.3);
    ffb->setSpinnable(true);
    ffb->setCallback([this, request_rebuild](float value) {
      if (hp) {
        hp->strand_length = value;
        request_rebuild();
      }
    });
  }

  new Label(window, "Strategies", "sans-bold");
  {
    Widget *panel = new Widget(window);
    GridLayout *layout =
        new GridLayout(Orientation::Horizontal, 2, Alignment::Middle, 5, 5);
    layout->setColAlignment({Alignment::Maximum, Alignment::Fill});
    layout->setSpacing(0, 10);
    panel->setLayout(layout);

    new Label(panel, "sim :", "sans-bold");
    ComboBox *sim_cb =
        new ComboBox(panel, {"DFTL Core", "FTL Reference"});
    sim_cb->setCallback([this](int idx) {
      if (!hp) return;
      hp->sim_strategy = (idx == 1) ? HairSimStrategy::FTL_REFERENCE
                                    : HairSimStrategy::DFTL_CORE;
    });
    sim_cb->setSelectedIndex(
        hp && hp->sim_strategy == HairSimStrategy::FTL_REFERENCE ? 1 : 0);

    new Label(panel, "render :", "sans-bold");
    ComboBox *render_cb =
        new ComboBox(panel, {"Debug Lines", "Fur Ribbons"});
    render_cb->setCallback([this](int idx) {
      if (!hp) return;
      hp->render_strategy = (idx == 0) ? HairRenderStrategy::DEBUG_LINES
                                       : HairRenderStrategy::FUR_RIBBONS;
    });
    render_cb->setSelectedIndex(
        hp && hp->render_strategy == HairRenderStrategy::DEBUG_LINES ? 0 : 1);
  }

  {
    Button *rebuild_btn = new Button(window, "Rebuild Hair");
    rebuild_btn->setCallback([request_rebuild]() { request_rebuild(); });
  }

  // Simulation constants

  new Label(window, "Simulation", "sans-bold");

  {
    Widget *panel = new Widget(window);
    GridLayout *layout =
        new GridLayout(Orientation::Horizontal, 2, Alignment::Middle, 5, 5);
    layout->setColAlignment({Alignment::Maximum, Alignment::Fill});
    layout->setSpacing(0, 10);
    panel->setLayout(layout);

    new Label(panel, "frames/s :", "sans-bold");

    IntBox<int> *fsec = new IntBox<int>(panel);
    fsec->setEditable(true);
    fsec->setFixedSize(Vector2i(100, 20));
    fsec->setFontSize(14);
    fsec->setValue(frames_per_sec);
    fsec->setSpinnable(true);
    fsec->setCallback([this](int value) { frames_per_sec = value; });

    new Label(panel, "steps/frame :", "sans-bold");

    IntBox<int> *num_steps = new IntBox<int>(panel);
    num_steps->setEditable(true);
    num_steps->setFixedSize(Vector2i(100, 20));
    num_steps->setFontSize(14);
    num_steps->setValue(simulation_steps);
    num_steps->setSpinnable(true);
    num_steps->setMinValue(0);
    num_steps->setCallback([this](int value) { simulation_steps = value; });
  }

  // Damping slider

  new Label(window, "Damping", "sans-bold");

  {
    Widget *panel = new Widget(window);
    panel->setLayout(
        new BoxLayout(Orientation::Horizontal, Alignment::Middle, 0, 5));

    Slider *slider = new Slider(panel);
    slider->setValue(hp ? hp->damping : 0.01);
    slider->setFixedWidth(105);

    TextBox *percentage = new TextBox(panel);
    percentage->setFixedWidth(75);
    percentage->setValue(to_string(hp ? hp->damping : 0.01));
    percentage->setFontSize(14);

    slider->setCallback([percentage](float value) {
      percentage->setValue(std::to_string(value));
    });
    slider->setFinalCallback([this](float value) {
      if (hp) hp->damping = (double)value;
    });
  }

  // Gravity

  new Label(window, "Gravity", "sans-bold");

  {
    Widget *panel = new Widget(window);
    GridLayout *layout =
        new GridLayout(Orientation::Horizontal, 2, Alignment::Middle, 5, 5);
    layout->setColAlignment({Alignment::Maximum, Alignment::Fill});
    layout->setSpacing(0, 10);
    panel->setLayout(layout);

    new Label(panel, "x :", "sans-bold");

    FloatBox<double> *fb = new FloatBox<double>(panel);
    fb->setEditable(true);
    fb->setFixedSize(Vector2i(100, 20));
    fb->setFontSize(14);
    fb->setValue(gravity.x);
    fb->setUnits("m/s^2");
    fb->setSpinnable(true);
    fb->setCallback([this](float value) { gravity.x = value; });

    new Label(panel, "y :", "sans-bold");

    fb = new FloatBox<double>(panel);
    fb->setEditable(true);
    fb->setFixedSize(Vector2i(100, 20));
    fb->setFontSize(14);
    fb->setValue(gravity.y);
    fb->setUnits("m/s^2");
    fb->setSpinnable(true);
    fb->setCallback([this](float value) { gravity.y = value; });

    new Label(panel, "z :", "sans-bold");

    fb = new FloatBox<double>(panel);
    fb->setEditable(true);
    fb->setFixedSize(Vector2i(100, 20));
    fb->setFontSize(14);
    fb->setValue(gravity.z);
    fb->setUnits("m/s^2");
    fb->setSpinnable(true);
    fb->setCallback([this](float value) { gravity.z = value; });
  }

  // Wind

  new Label(window, "Wind", "sans-bold");
  {
    Button *b = new Button(window, "Enable Wind");
    b->setFlags(Button::ToggleButton);
    b->setPushed(hp ? hp->wind.enable : false);
    b->setFontSize(14);
    b->setChangeCallback(
        [this](bool state) { if (hp) hp->wind.enable = state; });
  }

  {
    Widget *panel = new Widget(window);
    GridLayout *layout =
        new GridLayout(Orientation::Horizontal, 2, Alignment::Middle, 5, 5);
    layout->setColAlignment({Alignment::Maximum, Alignment::Fill});
    layout->setSpacing(0, 10);
    panel->setLayout(layout);

    new Label(panel, "strength :", "sans-bold");

    FloatBox<double> *fb = new FloatBox<double>(panel);
    fb->setEditable(true);
    fb->setFixedSize(Vector2i(100, 20));
    fb->setFontSize(14);
    fb->setValue(hp ? hp->wind.strength : 20.0);
    fb->setSpinnable(true);
    fb->setCallback([this](float value) { if (hp) hp->wind.strength = value; });

    new Label(panel, "x :", "sans-bold");

    fb = new FloatBox<double>(panel);
    fb->setEditable(true);
    fb->setFixedSize(Vector2i(100, 20));
    fb->setFontSize(14);
    fb->setValue(hp ? hp->wind.direction.x : 1.0);
    fb->setSpinnable(true);
    fb->setCallback([this](float value) { if (hp) hp->wind.direction.x = value; });

    new Label(panel, "y :", "sans-bold");

    fb = new FloatBox<double>(panel);
    fb->setEditable(true);
    fb->setFixedSize(Vector2i(100, 20));
    fb->setFontSize(14);
    fb->setValue(hp ? hp->wind.direction.y : 0.0);
    fb->setSpinnable(true);
    fb->setCallback([this](float value) { if (hp) hp->wind.direction.y = value; });

    new Label(panel, "z :", "sans-bold");

    fb = new FloatBox<double>(panel);
    fb->setEditable(true);
    fb->setFixedSize(Vector2i(100, 20));
    fb->setFontSize(14);
    fb->setValue(hp ? hp->wind.direction.z : 0.0);
    fb->setSpinnable(true);
    fb->setCallback([this](float value) { if (hp) hp->wind.direction.z = value; });
  }

  // Object control

  new Label(window, "Object Control", "sans-bold");
  new Label(window, "Tab: select, Arrows: XZ, []: Y", "sans");

  // Appearance window

  window = new Window(screen, "Appearance");
  window->setPosition(Vector2i(15, 15));
  window->setLayout(new GroupLayout(15, 6, 14, 5));

  {
    ComboBox *cb = new ComboBox(window, shaders_combobox_names);
    cb->setFontSize(14);
    cb->setCallback(
        [this, screen](int idx) { active_shader_idx = idx; });
    cb->setSelectedIndex(active_shader_idx);
  }

  new Label(window, "Color", "sans-bold");

  {
    ColorWheel *cw = new ColorWheel(window, color);
    cw->setColor(this->color);
    cw->setCallback(
        [this](const nanogui::Color &color) { this->color = color; });
  }

  new Label(window, "Parameters", "sans-bold");

  {
    Widget *panel = new Widget(window);
    GridLayout *layout =
        new GridLayout(Orientation::Horizontal, 2, Alignment::Middle, 5, 5);
    layout->setColAlignment({Alignment::Maximum, Alignment::Fill});
    layout->setSpacing(0, 10);
    panel->setLayout(layout);

    new Label(panel, "Normal :", "sans-bold");

    FloatBox<double> *fb = new FloatBox<double>(panel);
    fb->setEditable(true);
    fb->setFixedSize(Vector2i(100, 20));
    fb->setFontSize(14);
    fb->setValue(this->m_normal_scaling);
    fb->setSpinnable(true);
    fb->setCallback([this](float value) { this->m_normal_scaling = value; });

    new Label(panel, "Height :", "sans-bold");

    fb = new FloatBox<double>(panel);
    fb->setEditable(true);
    fb->setFixedSize(Vector2i(100, 20));
    fb->setFontSize(14);
    fb->setValue(this->m_height_scaling);
    fb->setSpinnable(true);
    fb->setCallback([this](float value) { this->m_height_scaling = value; });
  }
}
