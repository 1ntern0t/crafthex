// Crafthex sky fragment shader (enhanced)
//
// Drop-in replacement for the original sky_fragment.glsl.
// Adds:
//  - subtle procedural aurora bands at night
//  - twinkling stars (procedural, no extra textures)
//  - keeps original sky texture sampling as the base layer
//
// Assumptions (matches Michael Fogleman-style Craft shaders):
//  - varying vec2 v_uv from sky_vertex.glsl
//  - uniform sampler2D sampler (bound to unit 2)
//  - uniform float timer in [0,1)

#version 120

uniform sampler2D sampler;
uniform float timer;

varying vec2 v_uv;

// Hash / noise helpers (cheap, deterministic)
float hash21(vec2 p) {
  p = fract(p * vec2(123.34, 456.21));
  p += dot(p, p + 45.32);
  return fract(p.x * p.y);
}

float noise(vec2 p) {
  vec2 i = floor(p);
  vec2 f = fract(p);
  float a = hash21(i);
  float b = hash21(i + vec2(1.0, 0.0));
  float c = hash21(i + vec2(0.0, 1.0));
  float d = hash21(i + vec2(1.0, 1.0));
  vec2 u = f * f * (3.0 - 2.0 * f);
  return mix(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}

void main() {
  // Base sky texture
  vec4 base = texture2D(sampler, v_uv);

  // Night factor: 0 at noon-ish, 1 at deep night.
  // timer cycles 0..1 with sunrise/sunset around the middle; we don't need
  // to be perfect, just visually plausible.
  float d = abs(timer - 0.5) * 2.0;       // 0 at noon, 1 at midnight-ish
  float night = smoothstep(0.45, 0.95, d);

  // Stars: high-frequency thresholded noise with twinkle.
  vec2 sp = v_uv * vec2(900.0, 450.0);
  float n = noise(sp);
  float tw = 0.65 + 0.35 * sin((timer * 6.28318) + n * 12.0);
  float stars = smoothstep(0.985, 1.0, n) * tw;

  // Aurora: flowing bands near the "horizon" (lower v). Keep it subtle.
  // We bias it towards the top half of the dome and fade near edges.
  float v = v_uv.y;
  float aurora_zone = smoothstep(0.15, 0.65, v) * smoothstep(0.98, 0.70, v);
  float flow = timer * 2.0;
  float band = 0.0;
  // Layer a few sine bands with noise-driven warping
  vec2 ap = v_uv * vec2(6.0, 2.5);
  float w1 = noise(ap * 2.0 + vec2(flow, -flow));
  float w2 = noise(ap * 5.0 + vec2(-flow * 1.3, flow * 0.7));
  float x = ap.x + (w1 - 0.5) * 0.8;
  float y = ap.y + (w2 - 0.5) * 0.6;
  band += sin(x * 2.0 + flow * 1.3) * 0.5 + 0.5;
  band *= sin(y * 3.0 - flow * 0.9) * 0.5 + 0.5;
  band = pow(band, 3.0);
  float aurora = band * aurora_zone;

  // Color the aurora (green/teal with slight purple rim)
  vec3 aur_col = mix(vec3(0.10, 0.85, 0.55), vec3(0.55, 0.25, 0.95), v_uv.x);
  aur_col *= (0.25 + 0.75 * smoothstep(0.2, 0.9, v));

  // Composite
  vec3 col = base.rgb;
  col += night * stars * vec3(1.0);
  col = mix(col, col + aurora * aur_col, night * 0.85);

  gl_FragColor = vec4(col, 1.0);
}
