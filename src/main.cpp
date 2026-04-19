#include <iostream>
#include <fstream>
#include <cmath>
#include <chrono>
#include <nanogui/nanogui.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include "misc/getopt.h"
#else
#include <getopt.h>
#include <unistd.h>
#endif
#include <unordered_set>
#include <stdlib.h>

#include "CGL/CGL.h"
#include "app.h"
#include "collision/sphere.h"
#include "io/sceneLoader.h"
#include "misc/file_utils.h"

typedef uint32_t gid_t;

using namespace std;
using namespace nanogui;

#define msg(s) cerr << "[FurReal] " << s << endl;

App *app = nullptr;
GLFWwindow *window = nullptr;
Screen *screen = nullptr;

static int run_baseline_check(HairParameters hair_params,
                              vector<CollisionObject *> &objects,
                              vector<CGL::Vector3D> &root_positions,
                              vector<CGL::Vector3D> &root_normals) {
  if (hair_params.num_strands < 1000) {
    hair_params.num_strands = 1000;
  }

  HairSystem hs;
  hs.buildStrands(root_positions, root_normals, &hair_params, &objects);
  if (hs.visibleStrandCount() <= 0) {
    std::cout << "[baseline] failed: no strands generated." << std::endl;
    return 2;
  }

  const int steps = 420;
  vector<CGL::Vector3D> external_accelerations = {CGL::Vector3D(0, -9.8, 0)};
  auto t0 = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < steps; i++) {
    hs.simulate(120.0, 1.0, &hair_params, external_accelerations, &objects);
  }
  auto t1 = std::chrono::high_resolution_clock::now();

  double elapsed =
      std::chrono::duration_cast<std::chrono::duration<double>>(t1 - t0).count();
  double max_stretch = hs.maxRelativeLengthError();
  double max_root_drift = hs.maxRootDrift();
  int visible = hs.visibleStrandCount();
  double steps_per_sec = (elapsed > 0.0) ? (steps / elapsed) : 0.0;

  std::cout << "[baseline] visible_strands=" << visible << std::endl;
  std::cout << "[baseline] max_relative_stretch=" << max_stretch << std::endl;
  std::cout << "[baseline] max_root_drift=" << max_root_drift << std::endl;
  std::cout << "[baseline] sim_steps_per_sec=" << steps_per_sec << std::endl;

  const bool pass = visible >= 1000 && max_stretch < 0.08 && max_root_drift < 0.05;
  std::cout << "[baseline] result=" << (pass ? "PASS" : "FAIL") << std::endl;
  return pass ? 0 : 1;
}

void error_callback(int error, const char* description) {
  puts(description);
}

void createGLContexts() {
  if (!glfwInit()) {
    return;
  }

  glfwSetTime(0);

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  glfwWindowHint(GLFW_SAMPLES, 0);
  glfwWindowHint(GLFW_RED_BITS, 8);
  glfwWindowHint(GLFW_GREEN_BITS, 8);
  glfwWindowHint(GLFW_BLUE_BITS, 8);
  glfwWindowHint(GLFW_ALPHA_BITS, 8);
  glfwWindowHint(GLFW_STENCIL_BITS, 8);
  glfwWindowHint(GLFW_DEPTH_BITS, 24);
  glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

  window = glfwCreateWindow(800, 800, "FurReal - Hair & Fur Simulator", nullptr, nullptr);
  if (window == nullptr) {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return;
  }
  glfwMakeContextCurrent(window);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    throw std::runtime_error("Could not initialize GLAD!");
  }
  glGetError();

  glClearColor(0.2f, 0.25f, 0.3f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  screen = new Screen();
  screen->initialize(window, true);

  int width, height;
  glfwGetFramebufferSize(window, &width, &height);
  glViewport(0, 0, width, height);
  glfwSwapInterval(1);
  glfwSwapBuffers(window);
}

void setGLFWCallbacks() {
  glfwSetCursorPosCallback(window, [](GLFWwindow *, double x, double y) {
    if (!screen->cursorPosCallbackEvent(x, y)) {
      app->cursorPosCallbackEvent(x / screen->pixelRatio(),
                                  y / screen->pixelRatio());
    }
  });

  glfwSetMouseButtonCallback(
      window, [](GLFWwindow *, int button, int action, int modifiers) {
        if (!screen->mouseButtonCallbackEvent(button, action, modifiers) ||
            action == GLFW_RELEASE) {
          app->mouseButtonCallbackEvent(button, action, modifiers);
        }
      });

  glfwSetKeyCallback(
      window, [](GLFWwindow *, int key, int scancode, int action, int mods) {
        if (!screen->keyCallbackEvent(key, scancode, action, mods)) {
          app->keyCallbackEvent(key, scancode, action, mods);
        }
      });

  glfwSetCharCallback(window, [](GLFWwindow *, unsigned int codepoint) {
    screen->charCallbackEvent(codepoint);
  });

  glfwSetDropCallback(window,
                      [](GLFWwindow *, int count, const char **filenames) {
                        screen->dropCallbackEvent(count, filenames);
                        app->dropCallbackEvent(count, filenames);
                      });

  glfwSetScrollCallback(window, [](GLFWwindow *, double x, double y) {
    if (!screen->scrollCallbackEvent(x, y)) {
      app->scrollCallbackEvent(x, y);
    }
  });

  glfwSetFramebufferSizeCallback(window,
                                 [](GLFWwindow *, int width, int height) {
                                   screen->resizeCallbackEvent(width, height);
                                   app->resizeCallbackEvent(width, height);
                                 });
}

void usageError(const char *binaryName) {
  printf("Usage: %s [options]\n", binaryName);
  printf("Required program options:\n");
  printf("  -f     <STRING>    Filename of scene\n");
  printf("  -r     <STRING>    Project root.\n");
  printf("                     Should contain \"shaders/Default.vert\".\n");
  printf("                     Automatically searched for by default.\n");
  printf("  -a     <INT>       Sphere vertices latitude direction.\n");
  printf("  -o     <INT>       Sphere vertices longitude direction.\n");
  printf("  -t                 Run baseline acceptance checks without GUI.\n");
  printf("\n");
  exit(-1);
}

bool is_valid_project_root(const std::string& search_path) {
    std::stringstream ss;
    ss << search_path;
    ss << "/";
    ss << "shaders/Default.vert";

    return FileUtils::file_exists(ss.str());
}

bool find_project_root(const std::vector<std::string>& search_paths, std::string& retval) {
  for (std::string search_path : search_paths) {
    if (is_valid_project_root(search_path)) {
      retval = search_path;
      return true;
    }
  }
  return false;
}

int main(int argc, char **argv) {
  std::vector<std::string> search_paths = {
    ".",
    "..",
    "../..",
    "../../.."
  };
  std::string project_root;
  bool found_project_root = find_project_root(search_paths, project_root);

  HairParameters hair_params;
  vector<CollisionObject *> objects;
  vector<CGL::Vector3D> root_positions;
  vector<CGL::Vector3D> root_normals;

  int c;

  int sphere_num_lat = 40;
  int sphere_num_lon = 40;

  std::string file_to_load_from;
  bool file_specified = false;
  bool baseline_check_mode = false;

  while ((c = getopt (argc, argv, "f:r:a:o:t")) != -1) {
    switch (c) {
      case 'f': {
        file_to_load_from = optarg;
        file_specified = true;
        break;
      }
      case 'r': {
        project_root = optarg;
        if (!is_valid_project_root(project_root)) {
          std::cout << "Warn: Could not find required file \"shaders/Default.vert\" in specified project root: " << project_root << std::endl;
        }
        found_project_root = true;
        break;
      }
      case 'a': {
        int arg_int = atoi(optarg);
        if (arg_int < 1) {
          arg_int = 1;
        }
        sphere_num_lat = arg_int;
        break;
      }
      case 'o': {
        int arg_int = atoi(optarg);
        if (arg_int < 1) {
          arg_int = 1;
        }
        sphere_num_lon = arg_int;
        break;
      }
      case 't': {
        baseline_check_mode = true;
        break;
      }
      default: {
        usageError(argv[0]);
        break;
      }
    }
  }

  if (!found_project_root) {
    std::cout << "Error: Could not find required file \"shaders/Default.vert\" anywhere!" << std::endl;
    return -1;
  } else {
    std::cout << "Loading files starting from: " << project_root << std::endl;
  }

  if (!file_specified) {
    std::stringstream def_fname;
    def_fname << project_root;
    def_fname << "/scene/sphere.json";
    file_to_load_from = def_fname.str();
  }

  bool success = SceneLoader::loadScene(file_to_load_from, &hair_params,
                                        &objects, &root_positions, &root_normals,
                                        sphere_num_lat, sphere_num_lon);
  if (!success) {
    std::cout << "Warn: Unable to load from file: " << file_to_load_from << std::endl;
  }

  // Baseline requires dense fur target by default.
  if (hair_params.num_strands < 1000) {
    hair_params.num_strands = 1000;
  }

  if (baseline_check_mode) {
    return run_baseline_check(hair_params, objects, root_positions, root_normals);
  }

  glfwSetErrorCallback(error_callback);

  createGLContexts();

  // Initialize hair system
  HairSystem *hair_system = new HairSystem();
  hair_system->buildStrands(root_positions, root_normals, &hair_params, &objects);

  // Initialize the App
  HairParameters *hp = new HairParameters(hair_params);
  app = new App(project_root, screen);
  app->loadHairSystem(hair_system);
  app->loadHairParameters(hp);
  app->loadCollisionObjects(new vector<CollisionObject *>(objects));
  app->init();

  screen->setVisible(true);
  screen->performLayout();

  setGLFWCallbacks();

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    glClearColor(0.25f, 0.25f, 0.25f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    app->drawContents();

    screen->drawContents();
    screen->drawWidgets();

    glfwSwapBuffers(window);

    if (!app->isAlive()) {
      glfwSetWindowShouldClose(window, 1);
    }
  }

  return 0;
}
