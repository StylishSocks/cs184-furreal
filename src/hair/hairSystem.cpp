#include "hairSystem.h"
#include "ftlSolver.h"
#include "../core/integrator.h"
#include "../core/wind.h"

void HairSystem::buildStrands(const vector<Vector3D> &root_positions,
                              const vector<Vector3D> &root_normals,
                              HairParameters *params) {
  strands.clear();
  strands.resize(root_positions.size());

  for (size_t i = 0; i < root_positions.size(); i++) {
    Vector3D dir = root_normals[i].unit();
    strands[i].init(root_positions[i], dir,
                    params->strand_length,
                    params->particles_per_strand,
                    params->mass_per_particle);
  }
}

void HairSystem::simulate(double frames_per_sec, double simulation_steps,
                          HairParameters *params,
                          vector<Vector3D> external_accelerations,
                          vector<CollisionObject *> *collision_objects) {
  double delta_t = 1.0 / frames_per_sec / simulation_steps;
  double damping = params->damping;

  for (Strand &strand : strands) {
    // Step 1: Apply external forces (gravity, wind) to each particle
    for (PointMass &pm : strand.particles) {
      pm.forces = Vector3D(0, 0, 0);

      if (pm.pinned) {
        // Keep roots exactly at their attachment points.
        pm.position = pm.start_position;
        pm.last_position = pm.start_position;
        continue;
      }

      // External accelerations (gravity)
      for (const Vector3D &a : external_accelerations) {
        pm.forces += a * pm.mass;
      }

      // Wind force (use strand tangent as proxy for normal)
      if (params->wind.enable) {
        // Approximate normal from neighboring particles
        Vector3D normal(0, 1, 0); // fallback
        pm.forces += compute_wind_force(pm.position, normal, pm.mass,
                                        params->wind, sim_time);
      }
    }

    // Step 2: Verlet integration
    for (PointMass &pm : strand.particles) {
      verlet_step(pm, delta_t, damping);
    }

    // Step 3: Follow-The-Leader constraint solving (inextensibility)
    solve_ftl(strand);

    // Step 4: Collision with external objects
    if (collision_objects) {
      for (PointMass &pm : strand.particles) {
        if (pm.pinned) continue;
        for (CollisionObject *co : *collision_objects) {
          co->collide(pm);
        }
      }
    }
  }

  sim_time += delta_t;
}

void HairSystem::render(GLShader &shader) {
  // Render all strands as GL_LINES
  int total_segments = 0;
  for (const Strand &s : strands) {
    total_segments += s.num_particles() - 1;
  }

  if (total_segments == 0) return;

  Eigen::MatrixXf positions(4, total_segments * 2);
  int idx = 0;

  for (const Strand &s : strands) {
    for (int i = 0; i < s.num_particles() - 1; i++) {
      const Vector3D &p1 = s.particles[i].position;
      const Vector3D &p2 = s.particles[i + 1].position;
      positions.col(idx)     << p1.x, p1.y, p1.z, 1.0;
      positions.col(idx + 1) << p2.x, p2.y, p2.z, 1.0;
      idx += 2;
    }
  }

  shader.uploadAttrib("in_position", positions, false);
  shader.drawArray(GL_LINES, 0, total_segments * 2);
}

void HairSystem::reset() {
  for (Strand &s : strands) {
    s.reset();
  }
  sim_time = 0.0;
}
