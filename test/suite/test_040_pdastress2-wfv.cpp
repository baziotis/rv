// Shapes: U_TrT, LaunchCode: foout2f

#include <cmath>

float Global;

float Inc(float v) __attribute__((noinline));

float Inc(float v) {
  Global += v;
  return v;
}

extern "C" float
foo(float u, float t) {
float a, b, c, d, e, f, g, v, R, z;

  float x = cosf(sqrtf(fabs(5000 * u - 4))); // scrambled u

  if (sinf(3*u) <= 0.75f) goto A;
  else if (sinf(59*u) <= .5) goto E; else goto K;
// entry -> {A, E, K}

A:
  Inc(1.0);
  a = (-t);
  if (cosf(t) >= .5) goto B; else goto H; // div

B:
  Inc(2.0);
  t = t + 1.0;
  goto C;
C:
  Inc(45436456.0f);
  if (cosf(u) <= .75) goto D; else goto E;
D:
  Inc(454366.0f);
  R = 4.0;
  if (cosf(u) <= .25) goto F; else goto end;
E:
  Inc(4366.0f);
  goto F;
F:
  Inc(2342345);
  goto G;
G:
  v = 1.0;
  Inc(23452345);
  goto Z;

H:
  Inc(98123);
  goto I;
I:
  Inc(23425);
  if (cosf(x) <= .75) goto L; else goto K;
K:
  Inc(13452345);
  goto M;
L:
  Inc(232345);
  R = 3.0;
  if (cosf(x) <= .25 ) goto M; else goto end;
M:
  Inc(324234);
  goto N;
N:
  v = 2.0;
  Inc(4545);
  goto Z;

Z:
  R = v;  // phi
  Inc(7.0);

end:
  Inc(8.0);
  return R;
}