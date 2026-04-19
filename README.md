# cs184-furreal
Real-time strand-based hair and fur simulation and rendering.

## Baseline implemented
- Runtime-switchable strategies:
  - Simulation: `dftl_core`, `ftl_reference`
  - Rendering: `fur_ribbons`, `debug_lines`
- Guide + follower strands for dense fur rendering.
- Proxy-local root attachment for sphere and box.
- DFTL-style inextensibility step with velocity correction.
- Fur ribbon renderer with anisotropic highlights (`shaders/Fur.frag`).

## Scenes
- Sphere dense fur: `scene/hair_demo.json`
- Box dense fur: `scene/dense_fur_box.json`
