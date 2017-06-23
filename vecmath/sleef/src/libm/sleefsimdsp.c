//          Copyright Naoki Shibata 2010 - 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// Always use -ffp-contract=off option to compile SLEEF.

#include <stdint.h>
#include <assert.h>
#include <math.h>
#include <limits.h>
#include <float.h>

#include "misc.h"

void Sleef_x86CpuID(int32_t out[4], uint32_t eax, uint32_t ecx);

#if (defined(_MSC_VER))
#pragma fp_contract (off)
#endif

#ifdef ENABLE_SSE2
#define CONFIG 2
#include "helpersse2.h"
#ifdef DORENAME
#include "renamesse2.h"
#endif
#endif

#ifdef ENABLE_AVX
#define CONFIG 1
#include "helperavx.h"
#ifdef DORENAME
#include "renameavx.h"
#endif
#endif

#ifdef ENABLE_FMA4
#define CONFIG 4
#include "helperavx.h"
#ifdef DORENAME
#include "renamefma4.h"
#endif
#endif

#ifdef ENABLE_AVX2
#define CONFIG 1
#include "helperavx2.h"
#ifdef DORENAME
#include "renameavx2.h"
#endif
#endif

#ifdef ENABLE_AVX512F
#define CONFIG 1
#include "helperavx512f.h"
#ifdef DORENAME
#include "renameavx512f.h"
#endif
#endif

#ifdef ENABLE_VECEXT
#define CONFIG 1
#include "helpervecext.h"
#ifdef DORENAME
#include "renamevecext.h"
#endif
#endif

#ifdef ENABLE_PUREC
#define CONFIG 1
#include "helperpurec.h"
#ifdef DORENAME
#include "renamepurec.h"
#endif
#endif

#ifdef ENABLE_NEON32
#define CONFIG 1
#include "helperneon32.h"
#ifdef DORENAME
#include "renameneon32.h"
#endif
#endif

#ifdef ENABLE_ADVSIMD
#define CONFIG 1
#include "helperadvsimd.h"
#ifdef DORENAME
#include "renameadvsimd.h"
#endif
#endif

//

#include "df.h"

//

static INLINE CONST vopmask visnegzero_vo_vf(vfloat d) {
  return veq_vo_vi2_vi2(vreinterpret_vi2_vf(d), vreinterpret_vi2_vf(vcast_vf_f(-0.0)));
}

static INLINE CONST vmask vsignbit_vm_vf(vfloat f) {
  return vand_vm_vm_vm(vreinterpret_vm_vf(f), vreinterpret_vm_vf(vcast_vf_f(-0.0f)));
}

static INLINE CONST vfloat vmulsign_vf_vf_vf(vfloat x, vfloat y) {
  return vreinterpret_vf_vm(vxor_vm_vm_vm(vreinterpret_vm_vf(x), vsignbit_vm_vf(y)));
}

static INLINE CONST vfloat vcopysign_vf_vf_vf(vfloat x, vfloat y) {
  return vreinterpret_vf_vm(vxor_vm_vm_vm(vandnot_vm_vm_vm(vreinterpret_vm_vf(vcast_vf_f(-0.0f)), vreinterpret_vm_vf(x)),
					  vand_vm_vm_vm   (vreinterpret_vm_vf(vcast_vf_f(-0.0f)), vreinterpret_vm_vf(y))));
}

static INLINE CONST vfloat vsign_vf_vf(vfloat f) {
  return vreinterpret_vf_vm(vor_vm_vm_vm(vreinterpret_vm_vf(vcast_vf_f(1.0f)), vand_vm_vm_vm(vreinterpret_vm_vf(vcast_vf_f(-0.0f)), vreinterpret_vm_vf(f))));
}

static INLINE CONST vopmask vsignbit_vo_vf(vfloat d) {
  return veq_vo_vi2_vi2(vand_vi2_vi2_vi2(vreinterpret_vi2_vf(d), vcast_vi2_i(0x80000000)), vcast_vi2_i(0x80000000));
}

static INLINE CONST vint2 vsel_vi2_vf_vf_vi2_vi2(vfloat f0, vfloat f1, vint2 x, vint2 y) {
  return vsel_vi2_vo_vi2_vi2(vlt_vo_vf_vf(f0, f1), x, y);
}

static INLINE CONST vint2 vsel_vi2_vf_vi2(vfloat d, vint2 x) {
  return vand_vi2_vo_vi2(vsignbit_vo_vf(d), x);
}

#ifndef ENABLE_AVX512F
static INLINE CONST vint2 vilogbk_vi2_vf(vfloat d) {
  vopmask o = vlt_vo_vf_vf(d, vcast_vf_f(5.421010862427522E-20f));
  d = vsel_vf_vo_vf_vf(o, vmul_vf_vf_vf(vcast_vf_f(1.8446744073709552E19f), d), d);
  vint2 q = vand_vi2_vi2_vi2(vsrl_vi2_vi2_i(vcast_vi2_vm(vreinterpret_vm_vf(d)), 23), vcast_vi2_i(0xff));
  q = vsub_vi2_vi2_vi2(q, vsel_vi2_vo_vi2_vi2(o, vcast_vi2_i(64 + 0x7f), vcast_vi2_i(0x7f)));
  return q;
}

static INLINE CONST vint2 vilogb2k_vi2_vf(vfloat d) {
  vint2 q = vreinterpret_vi2_vf(d);
  q = vsrl_vi2_vi2_i(q, 23);
  q = vand_vi2_vi2_vi2(q, vcast_vi2_i(0xff));
  q = vsub_vi2_vi2_vi2(q, vcast_vi2_i(0x7f));
  return q;
}
#endif

//

EXPORT CONST vint2 xilogbf(vfloat d) {
  vint2 e = vilogbk_vi2_vf(vabs_vf_vf(d));
  e = vsel_vi2_vo_vi2_vi2(veq_vo_vf_vf(d, vcast_vf_f(0.0f)), vcast_vi2_i(FP_ILOGB0), e);
  e = vsel_vi2_vo_vi2_vi2(visnan_vo_vf(d), vcast_vi2_i(FP_ILOGBNAN), e);
  e = vsel_vi2_vo_vi2_vi2(visinf_vo_vf(d), vcast_vi2_i(INT_MAX), e);
  return e;
}

static INLINE CONST vfloat vpow2i_vf_vi2(vint2 q) {
  return vreinterpret_vf_vm(vcast_vm_vi2(vsll_vi2_vi2_i(vadd_vi2_vi2_vi2(q, vcast_vi2_i(0x7f)), 23)));
}

static INLINE CONST vfloat vldexp_vf_vf_vi2(vfloat x, vint2 q) {
  vfloat u;
  vint2 m = vsra_vi2_vi2_i(q, 31);
  m = vsll_vi2_vi2_i(vsub_vi2_vi2_vi2(vsra_vi2_vi2_i(vadd_vi2_vi2_vi2(m, q), 6), m), 4);
  q = vsub_vi2_vi2_vi2(q, vsll_vi2_vi2_i(m, 2));
  m = vadd_vi2_vi2_vi2(m, vcast_vi2_i(0x7f));
  m = vand_vi2_vi2_vi2(vgt_vi2_vi2_vi2(m, vcast_vi2_i(0)), m);
  vint2 n = vgt_vi2_vi2_vi2(m, vcast_vi2_i(0xff));
  m = vor_vi2_vi2_vi2(vandnot_vi2_vi2_vi2(n, m), vand_vi2_vi2_vi2(n, vcast_vi2_i(0xff)));
  u = vreinterpret_vf_vm(vcast_vm_vi2(vsll_vi2_vi2_i(m, 23)));
  x = vmul_vf_vf_vf(vmul_vf_vf_vf(vmul_vf_vf_vf(vmul_vf_vf_vf(x, u), u), u), u);
  u = vreinterpret_vf_vm(vcast_vm_vi2(vsll_vi2_vi2_i(vadd_vi2_vi2_vi2(q, vcast_vi2_i(0x7f)), 23)));
  return vmul_vf_vf_vf(x, u);
}

static INLINE CONST vfloat vldexp2_vf_vf_vi2(vfloat d, vint2 e) {
  return vmul_vf_vf_vf(vmul_vf_vf_vf(d, vpow2i_vf_vi2(vsra_vi2_vi2_i(e, 1))), vpow2i_vf_vi2(vsub_vi2_vi2_vi2(e, vsra_vi2_vi2_i(e, 1))));
}

static INLINE CONST vfloat vldexp3_vf_vf_vi2(vfloat d, vint2 q) {
  return vreinterpret_vf_vi2(vadd_vi2_vi2_vi2(vreinterpret_vi2_vf(d), vsll_vi2_vi2_i(q, 23)));
}

EXPORT CONST vfloat xldexpf(vfloat x, vint2 q) { return vldexp_vf_vf_vi2(x, q); }

EXPORT CONST vfloat xsinf(vfloat d) {
  vint2 q;
  vfloat u, s, r = d;

  q = vrint_vi2_vf(vmul_vf_vf_vf(d, vcast_vf_f((float)M_1_PI)));
  u = vcast_vf_vi2(q);

  d = vmla_vf_vf_vf_vf(u, vcast_vf_f(-PI_Af), d);
  d = vmla_vf_vf_vf_vf(u, vcast_vf_f(-PI_Bf), d);
  d = vmla_vf_vf_vf_vf(u, vcast_vf_f(-PI_Cf), d);
  d = vmla_vf_vf_vf_vf(u, vcast_vf_f(-PI_Df), d);

  s = vmul_vf_vf_vf(d, d);

  d = vreinterpret_vf_vm(vxor_vm_vm_vm(vand_vm_vo32_vm(veq_vo_vi2_vi2(vand_vi2_vi2_vi2(q, vcast_vi2_i(1)), vcast_vi2_i(1)), vreinterpret_vm_vf(vcast_vf_f(-0.0f))), vreinterpret_vm_vf(d)));

  u = vcast_vf_f(2.6083159809786593541503e-06f);
  u = vmla_vf_vf_vf_vf(u, s, vcast_vf_f(-0.0001981069071916863322258f));
  u = vmla_vf_vf_vf_vf(u, s, vcast_vf_f(0.00833307858556509017944336f));
  u = vmla_vf_vf_vf_vf(u, s, vcast_vf_f(-0.166666597127914428710938f));

  u = vadd_vf_vf_vf(vmul_vf_vf_vf(s, vmul_vf_vf_vf(u, d)), d);

  u = vsel_vf_vo_vf_vf(vor_vo_vo_vo(visnegzero_vo_vf(r),
				    vgt_vo_vf_vf(vabs_vf_vf(r), vcast_vf_f(TRIGRANGEMAXf))),
		       vcast_vf_f(-0.0), u);

  u = vreinterpret_vf_vm(vor_vm_vo32_vm(visinf_vo_vf(d), vreinterpret_vm_vf(u)));

  return u;
}

EXPORT CONST vfloat xcosf(vfloat d) {
  vint2 q;
  vfloat u, s, r = d;

  q = vrint_vi2_vf(vsub_vf_vf_vf(vmul_vf_vf_vf(d, vcast_vf_f((float)M_1_PI)), vcast_vf_f(0.5f)));
  q = vadd_vi2_vi2_vi2(vadd_vi2_vi2_vi2(q, q), vcast_vi2_i(1));

  u = vcast_vf_vi2(q);
  d = vmla_vf_vf_vf_vf(u, vcast_vf_f(-PI_Af*0.5f), d);
  d = vmla_vf_vf_vf_vf(u, vcast_vf_f(-PI_Bf*0.5f), d);
  d = vmla_vf_vf_vf_vf(u, vcast_vf_f(-PI_Cf*0.5f), d);
  d = vmla_vf_vf_vf_vf(u, vcast_vf_f(-PI_Df*0.5f), d);

  s = vmul_vf_vf_vf(d, d);

  d = vreinterpret_vf_vm(vxor_vm_vm_vm(vand_vm_vo32_vm(veq_vo_vi2_vi2(vand_vi2_vi2_vi2(q, vcast_vi2_i(2)), vcast_vi2_i(0)), vreinterpret_vm_vf(vcast_vf_f(-0.0f))), vreinterpret_vm_vf(d)));

  u = vcast_vf_f(2.6083159809786593541503e-06f);
  u = vmla_vf_vf_vf_vf(u, s, vcast_vf_f(-0.0001981069071916863322258f));
  u = vmla_vf_vf_vf_vf(u, s, vcast_vf_f(0.00833307858556509017944336f));
  u = vmla_vf_vf_vf_vf(u, s, vcast_vf_f(-0.166666597127914428710938f));

  u = vadd_vf_vf_vf(vmul_vf_vf_vf(s, vmul_vf_vf_vf(u, d)), d);

  u = vreinterpret_vf_vm(vandnot_vm_vo32_vm(vgt_vo_vf_vf(vabs_vf_vf(r), vcast_vf_f(TRIGRANGEMAXf)),
					    vreinterpret_vm_vf(u)));

  u = vreinterpret_vf_vm(vor_vm_vo32_vm(visinf_vo_vf(d), vreinterpret_vm_vf(u)));

  return u;
}

EXPORT CONST vfloat2 xsincosf(vfloat d) {
  vint2 q;
  vopmask o;
  vfloat u, s, t, rx, ry;
  vfloat2 r;

  q = vrint_vi2_vf(vmul_vf_vf_vf(d, vcast_vf_f((float)M_2_PI)));

  s = d;

  u = vcast_vf_vi2(q);
  s = vmla_vf_vf_vf_vf(u, vcast_vf_f(-PI_Af*0.5f), s);
  s = vmla_vf_vf_vf_vf(u, vcast_vf_f(-PI_Bf*0.5f), s);
  s = vmla_vf_vf_vf_vf(u, vcast_vf_f(-PI_Cf*0.5f), s);
  s = vmla_vf_vf_vf_vf(u, vcast_vf_f(-PI_Df*0.5f), s);

  t = s;

  s = vmul_vf_vf_vf(s, s);

  u = vcast_vf_f(-0.000195169282960705459117889f);
  u = vmla_vf_vf_vf_vf(u, s, vcast_vf_f(0.00833215750753879547119141f));
  u = vmla_vf_vf_vf_vf(u, s, vcast_vf_f(-0.166666537523269653320312f));

  rx = vmla_vf_vf_vf_vf(vmul_vf_vf_vf(u, s), t, t);
  rx = vsel_vf_vo_vf_vf(visnegzero_vo_vf(d), vcast_vf_f(-0.0f), rx);

  u = vcast_vf_f(-2.71811842367242206819355e-07f);
  u = vmla_vf_vf_vf_vf(u, s, vcast_vf_f(2.47990446951007470488548e-05f));
  u = vmla_vf_vf_vf_vf(u, s, vcast_vf_f(-0.00138888787478208541870117f));
  u = vmla_vf_vf_vf_vf(u, s, vcast_vf_f(0.0416666641831398010253906f));
  u = vmla_vf_vf_vf_vf(u, s, vcast_vf_f(-0.5));

  ry = vmla_vf_vf_vf_vf(s, u, vcast_vf_f(1));

  o = veq_vo_vi2_vi2(vand_vi2_vi2_vi2(q, vcast_vi2_i(1)), vcast_vi2_i(0));
  r.x = vsel_vf_vo_vf_vf(o, rx, ry);
  r.y = vsel_vf_vo_vf_vf(o, ry, rx);

  o = veq_vo_vi2_vi2(vand_vi2_vi2_vi2(q, vcast_vi2_i(2)), vcast_vi2_i(2));
  r.x = vreinterpret_vf_vm(vxor_vm_vm_vm(vand_vm_vo32_vm(o, vreinterpret_vm_vf(vcast_vf_f(-0.0))), vreinterpret_vm_vf(r.x)));

  o = veq_vo_vi2_vi2(vand_vi2_vi2_vi2(vadd_vi2_vi2_vi2(q, vcast_vi2_i(1)), vcast_vi2_i(2)), vcast_vi2_i(2));
  r.y = vreinterpret_vf_vm(vxor_vm_vm_vm(vand_vm_vo32_vm(o, vreinterpret_vm_vf(vcast_vf_f(-0.0))), vreinterpret_vm_vf(r.y)));

  o = vgt_vo_vf_vf(vabs_vf_vf(d), vcast_vf_f(TRIGRANGEMAXf));
  r.x = vreinterpret_vf_vm(vandnot_vm_vo32_vm(o, vreinterpret_vm_vf(r.x)));
  r.y = vreinterpret_vf_vm(vandnot_vm_vo32_vm(o, vreinterpret_vm_vf(r.y)));

  o = visinf_vo_vf(d);
  r.x = vreinterpret_vf_vm(vor_vm_vo32_vm(o, vreinterpret_vm_vf(r.x)));
  r.y = vreinterpret_vf_vm(vor_vm_vo32_vm(o, vreinterpret_vm_vf(r.y)));

  return r;
}

EXPORT CONST vfloat xtanf(vfloat d) {
  vint2 q;
  vopmask o;
  vfloat u, s, x;

  q = vrint_vi2_vf(vmul_vf_vf_vf(d, vcast_vf_f((float)(2 * M_1_PI))));

  x = d;

  u = vcast_vf_vi2(q);
  x = vmla_vf_vf_vf_vf(u, vcast_vf_f(-PI_Af*0.5f), x);
  x = vmla_vf_vf_vf_vf(u, vcast_vf_f(-PI_Bf*0.5f), x);
  x = vmla_vf_vf_vf_vf(u, vcast_vf_f(-PI_Cf*0.5f), x);
  x = vmla_vf_vf_vf_vf(u, vcast_vf_f(-PI_Df*0.5f), x);

  s = vmul_vf_vf_vf(x, x);

  o = veq_vo_vi2_vi2(vand_vi2_vi2_vi2(q, vcast_vi2_i(1)), vcast_vi2_i(1));
  x = vreinterpret_vf_vm(vxor_vm_vm_vm(vand_vm_vo32_vm(o, vreinterpret_vm_vf(vcast_vf_f(-0.0f))), vreinterpret_vm_vf(x)));

  u = vcast_vf_f(0.00927245803177356719970703f);
  u = vmla_vf_vf_vf_vf(u, s, vcast_vf_f(0.00331984995864331722259521f));
  u = vmla_vf_vf_vf_vf(u, s, vcast_vf_f(0.0242998078465461730957031f));
  u = vmla_vf_vf_vf_vf(u, s, vcast_vf_f(0.0534495301544666290283203f));
  u = vmla_vf_vf_vf_vf(u, s, vcast_vf_f(0.133383005857467651367188f));
  u = vmla_vf_vf_vf_vf(u, s, vcast_vf_f(0.333331853151321411132812f));

  u = vmla_vf_vf_vf_vf(s, vmul_vf_vf_vf(u, x), x);

  u = vsel_vf_vo_vf_vf(o, vrec_vf_vf(u), u);

#ifndef ENABLE_AVX512F
  u = vreinterpret_vf_vm(vor_vm_vo32_vm(visinf_vo_vf(d), vreinterpret_vm_vf(u)));
#else
  u = vfixup_vf_vf_vf_vi2_i(u, d, vcast_vi2_i((3 << (4*4)) | (3 << (5*4))), 0);
#endif

  return u;
}

EXPORT CONST vfloat xsinf_u1(vfloat d) {
  vint2 q;
  vfloat u;
  vfloat2 s, t, x;

  q = vrint_vi2_vf(vmul_vf_vf_vf(d, vcast_vf_f(M_1_PI)));
  u = vcast_vf_vi2(q);

  s = dfadd_vf2_vf_vf (d, vmul_vf_vf_vf(u, vcast_vf_f(-PI_Af)));
  s = dfadd_vf2_vf2_vf(s, vmul_vf_vf_vf(u, vcast_vf_f(-PI_Bf)));
  s = dfadd_vf2_vf2_vf(s, vmul_vf_vf_vf(u, vcast_vf_f(-PI_Cf)));
  s = dfadd_vf2_vf2_vf(s, vmul_vf_vf_vf(u, vcast_vf_f(-PI_Df)));

  t = s;
  s = dfsqu_vf2_vf2(s);

  u = vcast_vf_f(2.6083159809786593541503e-06f);
  u = vmla_vf_vf_vf_vf(u, s.x, vcast_vf_f(-0.0001981069071916863322258f));
  u = vmla_vf_vf_vf_vf(u, s.x, vcast_vf_f(0.00833307858556509017944336f));

  x = dfadd_vf2_vf_vf2(vcast_vf_f(1), dfmul_vf2_vf2_vf2(dfadd_vf2_vf_vf(vcast_vf_f(-0.166666597127914428710938f), vmul_vf_vf_vf(u, s.x)), s));

  u = dfmul_vf_vf2_vf2(t, x);

  u = vreinterpret_vf_vm(vxor_vm_vm_vm(vand_vm_vo32_vm(veq_vo_vi2_vi2(vand_vi2_vi2_vi2(q, vcast_vi2_i(1)), vcast_vi2_i(1)), vreinterpret_vm_vf(vcast_vf_f(-0.0))), vreinterpret_vm_vf(u)));
  u = vsel_vf_vo_vf_vf(vandnot_vo_vo_vo(visinf_vo_vf(d), vor_vo_vo_vo(visnegzero_vo_vf(d),
								      vgt_vo_vf_vf(vabs_vf_vf(d), vcast_vf_f(TRIGRANGEMAXf)))),
		       vcast_vf_f(-0.0), u);

  return u;
}

EXPORT CONST vfloat xcosf_u1(vfloat d) {
  vint2 q;
  vfloat u;
  vfloat2 s, t, x;

  q = vrint_vi2_vf(vmla_vf_vf_vf_vf(d, vcast_vf_f(M_1_PI), vcast_vf_f(-0.5)));
  q = vadd_vi2_vi2_vi2(vadd_vi2_vi2_vi2(q, q), vcast_vi2_i(1));
  u = vcast_vf_vi2(q);

  s = dfadd2_vf2_vf_vf (d, vmul_vf_vf_vf(u, vcast_vf_f(-PI_Af*0.5f)));
  s = dfadd2_vf2_vf2_vf(s, vmul_vf_vf_vf(u, vcast_vf_f(-PI_Bf*0.5f)));
  s = dfadd2_vf2_vf2_vf(s, vmul_vf_vf_vf(u, vcast_vf_f(-PI_Cf*0.5f)));
  s = dfadd2_vf2_vf2_vf(s, vmul_vf_vf_vf(u, vcast_vf_f(-PI_Df*0.5f)));

  t = s;
  s = dfsqu_vf2_vf2(s);

  u = vcast_vf_f(2.6083159809786593541503e-06f);
  u = vmla_vf_vf_vf_vf(u, s.x, vcast_vf_f(-0.0001981069071916863322258f));
  u = vmla_vf_vf_vf_vf(u, s.x, vcast_vf_f(0.00833307858556509017944336f));

  x = dfadd_vf2_vf_vf2(vcast_vf_f(1), dfmul_vf2_vf2_vf2(dfadd_vf2_vf_vf(vcast_vf_f(-0.166666597127914428710938f), vmul_vf_vf_vf(u, s.x)), s));

  u = dfmul_vf_vf2_vf2(t, x);

  u = vreinterpret_vf_vm(vxor_vm_vm_vm(vand_vm_vo32_vm(veq_vo_vi2_vi2(vand_vi2_vi2_vi2(q, vcast_vi2_i(2)), vcast_vi2_i(0)), vreinterpret_vm_vf(vcast_vf_f(-0.0))), vreinterpret_vm_vf(u)));

  u = vreinterpret_vf_vm(vandnot_vm_vo32_vm(vandnot_vo_vo_vo(visinf_vo_vf(d), vgt_vo_vf_vf(vabs_vf_vf(d), vcast_vf_f(TRIGRANGEMAXf))),
					    vreinterpret_vm_vf(u)));

  return u;
}

EXPORT CONST vfloat2 xsincosf_u1(vfloat d) {
  vint2 q;
  vopmask o;
  vfloat u, rx, ry;
  vfloat2 r, s, t, x;

  q = vrint_vi2_vf(vmul_vf_vf_vf(d, vcast_vf_f(2 * M_1_PI)));
  u = vcast_vf_vi2(q);

  s = dfadd_vf2_vf_vf (d, vmul_vf_vf_vf(u, vcast_vf_f(-PI_Af*0.5f)));
  s = dfadd_vf2_vf2_vf(s, vmul_vf_vf_vf(u, vcast_vf_f(-PI_Bf*0.5f)));
  s = dfadd_vf2_vf2_vf(s, vmul_vf_vf_vf(u, vcast_vf_f(-PI_Cf*0.5f)));
  s = dfadd_vf2_vf2_vf(s, vmul_vf_vf_vf(u, vcast_vf_f(-PI_Df*0.5f)));

  t = s;

  s.x = dfsqu_vf_vf2(s);

  u = vcast_vf_f(-0.000195169282960705459117889f);
  u = vmla_vf_vf_vf_vf(u, s.x, vcast_vf_f(0.00833215750753879547119141f));
  u = vmla_vf_vf_vf_vf(u, s.x, vcast_vf_f(-0.166666537523269653320312f));

  u = vmul_vf_vf_vf(u, vmul_vf_vf_vf(s.x, t.x));

  x = dfadd_vf2_vf2_vf(t, u);
  rx = vadd_vf_vf_vf(x.x, x.y);

  rx = vsel_vf_vo_vf_vf(visnegzero_vo_vf(d), vcast_vf_f(-0.0f), rx);

  u = vcast_vf_f(-2.71811842367242206819355e-07f);
  u = vmla_vf_vf_vf_vf(u, s.x, vcast_vf_f(2.47990446951007470488548e-05f));
  u = vmla_vf_vf_vf_vf(u, s.x, vcast_vf_f(-0.00138888787478208541870117f));
  u = vmla_vf_vf_vf_vf(u, s.x, vcast_vf_f(0.0416666641831398010253906f));
  u = vmla_vf_vf_vf_vf(u, s.x, vcast_vf_f(-0.5));

  x = dfadd_vf2_vf_vf2(vcast_vf_f(1), dfmul_vf2_vf_vf(s.x, u));
  ry = vadd_vf_vf_vf(x.x, x.y);

  o = veq_vo_vi2_vi2(vand_vi2_vi2_vi2(q, vcast_vi2_i(1)), vcast_vi2_i(0));
  r.x = vsel_vf_vo_vf_vf(o, rx, ry);
  r.y = vsel_vf_vo_vf_vf(o, ry, rx);

  o = veq_vo_vi2_vi2(vand_vi2_vi2_vi2(q, vcast_vi2_i(2)), vcast_vi2_i(2));
  r.x = vreinterpret_vf_vm(vxor_vm_vm_vm(vand_vm_vo32_vm(o, vreinterpret_vm_vf(vcast_vf_f(-0.0))), vreinterpret_vm_vf(r.x)));

  o = veq_vo_vi2_vi2(vand_vi2_vi2_vi2(vadd_vi2_vi2_vi2(q, vcast_vi2_i(1)), vcast_vi2_i(2)), vcast_vi2_i(2));
  r.y = vreinterpret_vf_vm(vxor_vm_vm_vm(vand_vm_vo32_vm(o, vreinterpret_vm_vf(vcast_vf_f(-0.0))), vreinterpret_vm_vf(r.y)));

  o = vgt_vo_vf_vf(vabs_vf_vf(d), vcast_vf_f(TRIGRANGEMAXf));
  r.x = vreinterpret_vf_vm(vandnot_vm_vo32_vm(o, vreinterpret_vm_vf(r.x)));
  r.y = vreinterpret_vf_vm(vandnot_vm_vo32_vm(o, vreinterpret_vm_vf(r.y)));

  o = visinf_vo_vf(d);
  r.x = vreinterpret_vf_vm(vor_vm_vo32_vm(o, vreinterpret_vm_vf(r.x)));
  r.y = vreinterpret_vf_vm(vor_vm_vo32_vm(o, vreinterpret_vm_vf(r.y)));

  return r;
}

EXPORT CONST vfloat2 xsincospif_u05(vfloat d) {
  vopmask o;
  vfloat u, s, t, rx, ry;
  vfloat2 r, x, s2;

  u = vmul_vf_vf_vf(d, vcast_vf_f(4));
  vint2 q = vand_vi2_vi2_vi2(vrint_vi2_vf(vadd_vf_vf_vf(u, vcast_vf_f(0.5))), vcast_vi2_i(~1));
  s = vsub_vf_vf_vf(u, vcast_vf_vi2(q));

  t = s;
  s = vmul_vf_vf_vf(s, s);
  s2 = dfmul_vf2_vf_vf(t, t);

  //

  u = vcast_vf_f(+0.3093842054e-6);
  u = vmla_vf_vf_vf_vf(u, s, vcast_vf_f(-0.3657307388e-4));
  u = vmla_vf_vf_vf_vf(u, s, vcast_vf_f(+0.2490393585e-2));
  x = dfadd2_vf2_vf_vf2(vmul_vf_vf_vf(u, s), vcast_vf2_f_f(-0.080745510756969451904, -1.3373665339076936258e-09));
  x = dfadd2_vf2_vf2_vf2(dfmul_vf2_vf2_vf2(s2, x), vcast_vf2_f_f(0.78539818525314331055, -2.1857338617566484855e-08));

  x = dfmul_vf2_vf2_vf(x, t);
  rx = vadd_vf_vf_vf(x.x, x.y);

  rx = vsel_vf_vo_vf_vf(visnegzero_vo_vf(d), vcast_vf_f(-0.0f), rx);

  //

  u = vcast_vf_f(-0.2430611801e-7);
  u = vmla_vf_vf_vf_vf(u, s, vcast_vf_f(+0.3590577080e-5));
  u = vmla_vf_vf_vf_vf(u, s, vcast_vf_f(-0.3259917721e-3));
  x = dfadd2_vf2_vf_vf2(vmul_vf_vf_vf(u, s), vcast_vf2_f_f(0.015854343771934509277, 4.4940051354032242811e-10));
  x = dfadd2_vf2_vf2_vf2(dfmul_vf2_vf2_vf2(s2, x), vcast_vf2_f_f(-0.30842512845993041992, -9.0728339030733922277e-09));

  x = dfadd2_vf2_vf2_vf(dfmul_vf2_vf2_vf2(x, s2), vcast_vf_f(1));
  ry = vadd_vf_vf_vf(x.x, x.y);

  //

  o = veq_vo_vi2_vi2(vand_vi2_vi2_vi2(q, vcast_vi2_i(2)), vcast_vi2_i(0));
  r.x = vsel_vf_vo_vf_vf(o, rx, ry);
  r.y = vsel_vf_vo_vf_vf(o, ry, rx);

  o = veq_vo_vi2_vi2(vand_vi2_vi2_vi2(q, vcast_vi2_i(4)), vcast_vi2_i(4));
  r.x = vreinterpret_vf_vm(vxor_vm_vm_vm(vand_vm_vo32_vm(o, vreinterpret_vm_vf(vcast_vf_f(-0.0))), vreinterpret_vm_vf(r.x)));

  o = veq_vo_vi2_vi2(vand_vi2_vi2_vi2(vadd_vi2_vi2_vi2(q, vcast_vi2_i(2)), vcast_vi2_i(4)), vcast_vi2_i(4));
  r.y = vreinterpret_vf_vm(vxor_vm_vm_vm(vand_vm_vo32_vm(o, vreinterpret_vm_vf(vcast_vf_f(-0.0))), vreinterpret_vm_vf(r.y)));

  o = vgt_vo_vf_vf(vabs_vf_vf(d), vcast_vf_f(TRIGRANGEMAXf));
  r.x = vreinterpret_vf_vm(vandnot_vm_vo32_vm(o, vreinterpret_vm_vf(r.x)));
  r.y = vreinterpret_vf_vm(vandnot_vm_vo32_vm(o, vreinterpret_vm_vf(r.y)));

  o = visinf_vo_vf(d);
  r.x = vreinterpret_vf_vm(vor_vm_vo32_vm(o, vreinterpret_vm_vf(r.x)));
  r.y = vreinterpret_vf_vm(vor_vm_vo32_vm(o, vreinterpret_vm_vf(r.y)));

  return r;
}

EXPORT CONST vfloat2 xsincospif_u35(vfloat d) {
  vopmask o;
  vfloat u, s, t, rx, ry;
  vfloat2 r;

  u = vmul_vf_vf_vf(d, vcast_vf_f(4));
  vint2 q = vand_vi2_vi2_vi2(vrint_vi2_vf(vadd_vf_vf_vf(u, vcast_vf_f(0.5))), vcast_vi2_i(~1));
  s = vsub_vf_vf_vf(u, vcast_vf_vi2(q));

  t = s;
  s = vmul_vf_vf_vf(s, s);

  //

  u = vcast_vf_f(-0.3600925265e-4);
  u = vmla_vf_vf_vf_vf(u, s, vcast_vf_f(+0.2490088111e-2));
  u = vmla_vf_vf_vf_vf(u, s, vcast_vf_f(-0.8074551076e-1));
  u = vmla_vf_vf_vf_vf(u, s, vcast_vf_f(+0.7853981853e+0));

  rx = vmul_vf_vf_vf(u, t);

  //

  u = vcast_vf_f(+0.3539815225e-5);
  u = vmla_vf_vf_vf_vf(u, s, vcast_vf_f(-0.3259574005e-3));
  u = vmla_vf_vf_vf_vf(u, s, vcast_vf_f(+0.1585431583e-1));
  u = vmla_vf_vf_vf_vf(u, s, vcast_vf_f(-0.3084251285e+0));
  u = vmla_vf_vf_vf_vf(u, s, vcast_vf_f(1));

  ry = u;

  //

  o = veq_vo_vi2_vi2(vand_vi2_vi2_vi2(q, vcast_vi2_i(2)), vcast_vi2_i(0));
  r.x = vsel_vf_vo_vf_vf(o, rx, ry);
  r.y = vsel_vf_vo_vf_vf(o, ry, rx);

  o = veq_vo_vi2_vi2(vand_vi2_vi2_vi2(q, vcast_vi2_i(4)), vcast_vi2_i(4));
  r.x = vreinterpret_vf_vm(vxor_vm_vm_vm(vand_vm_vo32_vm(o, vreinterpret_vm_vf(vcast_vf_f(-0.0))), vreinterpret_vm_vf(r.x)));

  o = veq_vo_vi2_vi2(vand_vi2_vi2_vi2(vadd_vi2_vi2_vi2(q, vcast_vi2_i(2)), vcast_vi2_i(4)), vcast_vi2_i(4));
  r.y = vreinterpret_vf_vm(vxor_vm_vm_vm(vand_vm_vo32_vm(o, vreinterpret_vm_vf(vcast_vf_f(-0.0))), vreinterpret_vm_vf(r.y)));

  o = vgt_vo_vf_vf(vabs_vf_vf(d), vcast_vf_f(TRIGRANGEMAXf));
  r.x = vreinterpret_vf_vm(vandnot_vm_vo32_vm(o, vreinterpret_vm_vf(r.x)));
  r.y = vreinterpret_vf_vm(vandnot_vm_vo32_vm(o, vreinterpret_vm_vf(r.y)));

  o = visinf_vo_vf(d);
  r.x = vreinterpret_vf_vm(vor_vm_vo32_vm(o, vreinterpret_vm_vf(r.x)));
  r.y = vreinterpret_vf_vm(vor_vm_vo32_vm(o, vreinterpret_vm_vf(r.y)));

  return r;
}

EXPORT CONST vfloat xtanf_u1(vfloat d) {
  vint2 q;
  vfloat u;
  vfloat2 s, t, x;
  vopmask o;

  q = vrint_vi2_vf(vmul_vf_vf_vf(d, vcast_vf_f(M_2_PI)));
  u = vcast_vf_vi2(q);

  s = dfadd_vf2_vf_vf (d, vmul_vf_vf_vf(u, vcast_vf_f(-PI_Af*0.5f)));
  s = dfadd_vf2_vf2_vf(s, vmul_vf_vf_vf(u, vcast_vf_f(-PI_Bf*0.5f)));
  s = dfadd_vf2_vf2_vf(s, vmul_vf_vf_vf(u, vcast_vf_f(-PI_Cf*0.5f)));
  s = dfadd_vf2_vf2_vf(s, vmul_vf_vf_vf(u, vcast_vf_f(-PI_XDf*0.5f)));
  s = dfadd_vf2_vf2_vf(s, vmul_vf_vf_vf(u, vcast_vf_f(-PI_XEf*0.5f)));

  o = veq_vo_vi2_vi2(vand_vi2_vi2_vi2(q, vcast_vi2_i(1)), vcast_vi2_i(1));
  vmask n = vand_vm_vo32_vm(o, vreinterpret_vm_vf(vcast_vf_f(-0.0)));
  s.x = vreinterpret_vf_vm(vxor_vm_vm_vm(vreinterpret_vm_vf(s.x), n));
  s.y = vreinterpret_vf_vm(vxor_vm_vm_vm(vreinterpret_vm_vf(s.y), n));

  t = s;
  s = dfsqu_vf2_vf2(s);
  s = dfnormalize_vf2_vf2(s);

  u = vcast_vf_f(0.00446636462584137916564941f);
  u = vmla_vf_vf_vf_vf(u, s.x, vcast_vf_f(-8.3920182078145444393158e-05f));
  u = vmla_vf_vf_vf_vf(u, s.x, vcast_vf_f(0.0109639242291450500488281f));
  u = vmla_vf_vf_vf_vf(u, s.x, vcast_vf_f(0.0212360303848981857299805f));
  u = vmla_vf_vf_vf_vf(u, s.x, vcast_vf_f(0.0540687143802642822265625f));

  x = dfadd_vf2_vf_vf(vcast_vf_f(0.133325666189193725585938f), vmul_vf_vf_vf(u, s.x));
  x = dfadd_vf2_vf_vf2(vcast_vf_f(1), dfmul_vf2_vf2_vf2(dfadd_vf2_vf_vf2(vcast_vf_f(0.33333361148834228515625f), dfmul_vf2_vf2_vf2(s, x)), s));
  x = dfmul_vf2_vf2_vf2(t, x);

  x = vsel_vf2_vo_vf2_vf2(o, dfrec_vf2_vf2(x), x);

  u = vadd_vf_vf_vf(x.x, x.y);

  u = vsel_vf_vo_vf_vf(vandnot_vo_vo_vo(visinf_vo_vf(d),
					vor_vo_vo_vo(visnegzero_vo_vf(d),
						     vgt_vo_vf_vf(vabs_vf_vf(d), vcast_vf_f(TRIGRANGEMAXf)))),
		       vcast_vf_f(-0.0f), u);

  return u;
}

EXPORT CONST vfloat xatanf(vfloat d) {
  vfloat s, t, u;
  vint2 q;

  q = vsel_vi2_vf_vi2(d, vcast_vi2_i(2));
  s = vabs_vf_vf(d);

  q = vsel_vi2_vf_vf_vi2_vi2(vcast_vf_f(1.0f), s, vadd_vi2_vi2_vi2(q, vcast_vi2_i(1)), q);
  s = vsel_vf_vo_vf_vf(vlt_vo_vf_vf(vcast_vf_f(1.0f), s), vrec_vf_vf(s), s);

  t = vmul_vf_vf_vf(s, s);

  u = vcast_vf_f(0.00282363896258175373077393f);
  u = vmla_vf_vf_vf_vf(u, t, vcast_vf_f(-0.0159569028764963150024414f));
  u = vmla_vf_vf_vf_vf(u, t, vcast_vf_f(0.0425049886107444763183594f));
  u = vmla_vf_vf_vf_vf(u, t, vcast_vf_f(-0.0748900920152664184570312f));
  u = vmla_vf_vf_vf_vf(u, t, vcast_vf_f(0.106347933411598205566406f));
  u = vmla_vf_vf_vf_vf(u, t, vcast_vf_f(-0.142027363181114196777344f));
  u = vmla_vf_vf_vf_vf(u, t, vcast_vf_f(0.199926957488059997558594f));
  u = vmla_vf_vf_vf_vf(u, t, vcast_vf_f(-0.333331018686294555664062f));

  t = vmla_vf_vf_vf_vf(s, vmul_vf_vf_vf(t, u), s);

  t = vsel_vf_vo_vf_vf(veq_vo_vi2_vi2(vand_vi2_vi2_vi2(q, vcast_vi2_i(1)), vcast_vi2_i(1)), vsub_vf_vf_vf(vcast_vf_f((float)(M_PI/2)), t), t);

  t = vreinterpret_vf_vm(vxor_vm_vm_vm(vand_vm_vo32_vm(veq_vo_vi2_vi2(vand_vi2_vi2_vi2(q, vcast_vi2_i(2)), vcast_vi2_i(2)), vreinterpret_vm_vf(vcast_vf_f(-0.0f))), vreinterpret_vm_vf(t)));

#ifdef ENABLE_NEON32
  t = vsel_vf_vo_vf_vf(visinf_vo_vf(d), vmulsign_vf_vf_vf(vcast_vf_f(1.5874010519681994747517056f), d), t);
#endif

  return t;
}

static INLINE CONST vfloat atan2kf(vfloat y, vfloat x) {
  vfloat s, t, u;
  vint2 q;
  vopmask p;

  q = vsel_vi2_vf_vi2(x, vcast_vi2_i(-2));
  x = vabs_vf_vf(x);

  q = vsel_vi2_vf_vf_vi2_vi2(x, y, vadd_vi2_vi2_vi2(q, vcast_vi2_i(1)), q);
  p = vlt_vo_vf_vf(x, y);
  s = vsel_vf_vo_vf_vf(p, vneg_vf_vf(x), y);
  t = vmax_vf_vf_vf(x, y);

  s = vdiv_vf_vf_vf(s, t);
  t = vmul_vf_vf_vf(s, s);

  u = vcast_vf_f(0.00282363896258175373077393f);
  u = vmla_vf_vf_vf_vf(u, t, vcast_vf_f(-0.0159569028764963150024414f));
  u = vmla_vf_vf_vf_vf(u, t, vcast_vf_f(0.0425049886107444763183594f));
  u = vmla_vf_vf_vf_vf(u, t, vcast_vf_f(-0.0748900920152664184570312f));
  u = vmla_vf_vf_vf_vf(u, t, vcast_vf_f(0.106347933411598205566406f));
  u = vmla_vf_vf_vf_vf(u, t, vcast_vf_f(-0.142027363181114196777344f));
  u = vmla_vf_vf_vf_vf(u, t, vcast_vf_f(0.199926957488059997558594f));
  u = vmla_vf_vf_vf_vf(u, t, vcast_vf_f(-0.333331018686294555664062f));

  t = vmla_vf_vf_vf_vf(s, vmul_vf_vf_vf(t, u), s);
  t = vmla_vf_vf_vf_vf(vcast_vf_vi2(q), vcast_vf_f((float)(M_PI/2)), t);

  return t;
}

static INLINE CONST vfloat visinf2_vf_vf_vf(vfloat d, vfloat m) {
  return vreinterpret_vf_vm(vand_vm_vo32_vm(visinf_vo_vf(d), vor_vm_vm_vm(vsignbit_vm_vf(d), vreinterpret_vm_vf(m))));
}

EXPORT CONST vfloat xatan2f(vfloat y, vfloat x) {
  vfloat r = atan2kf(vabs_vf_vf(y), x);

  r = vmulsign_vf_vf_vf(r, x);
  r = vsel_vf_vo_vf_vf(vor_vo_vo_vo(visinf_vo_vf(x), veq_vo_vf_vf(x, vcast_vf_f(0.0f))), vsub_vf_vf_vf(vcast_vf_f((float)(M_PI/2)), visinf2_vf_vf_vf(x, vmulsign_vf_vf_vf(vcast_vf_f((float)(M_PI/2)), x))), r);
  r = vsel_vf_vo_vf_vf(visinf_vo_vf(y), vsub_vf_vf_vf(vcast_vf_f((float)(M_PI/2)), visinf2_vf_vf_vf(x, vmulsign_vf_vf_vf(vcast_vf_f((float)(M_PI/4)), x))), r);

  r = vsel_vf_vo_vf_vf(veq_vo_vf_vf(y, vcast_vf_f(0.0f)), vreinterpret_vf_vm(vand_vm_vo32_vm(vsignbit_vo_vf(x), vreinterpret_vm_vf(vcast_vf_f((float)M_PI)))), r);

  r = vreinterpret_vf_vm(vor_vm_vo32_vm(vor_vo_vo_vo(visnan_vo_vf(x), visnan_vo_vf(y)), vreinterpret_vm_vf(vmulsign_vf_vf_vf(r, y))));
  return r;
}

EXPORT CONST vfloat xasinf(vfloat d) {
  vopmask o = vlt_vo_vf_vf(vabs_vf_vf(d), vcast_vf_f(0.707f));
  vfloat x2 = vsel_vf_vo_vf_vf(o, vmul_vf_vf_vf(d, d), vmul_vf_vf_vf(vsub_vf_vf_vf(vcast_vf_f(1), vabs_vf_vf(d)), vcast_vf_f(0.5f)));
  vfloat x = vsel_vf_vo_vf_vf(o, vabs_vf_vf(d), vsqrt_vf_vf(x2)), u;

  u = vcast_vf_f(+0.1083120629e+0);
  u = vmla_vf_vf_vf_vf(u, x2, vcast_vf_f(-0.8821133524e-1));
  u = vmla_vf_vf_vf_vf(u, x2, vcast_vf_f(+0.7465086877e-1));
  u = vmla_vf_vf_vf_vf(u, x2, vcast_vf_f(+0.1687940583e-1));
  u = vmla_vf_vf_vf_vf(u, x2, vcast_vf_f(+0.4648575932e-1));
  u = vmla_vf_vf_vf_vf(u, x2, vcast_vf_f(+0.7487771660e-1));
  u = vmla_vf_vf_vf_vf(u, x2, vcast_vf_f(+0.1666697413e+0));
  u = vmla_vf_vf_vf_vf(vmul_vf_vf_vf(u, x), x2, x);

  vfloat r = vsel_vf_vo_vf_vf(o, u, vmla_vf_vf_vf_vf(u, vcast_vf_f(-2), vcast_vf_f(M_PIf/2)));
  return vmulsign_vf_vf_vf(r, d);
}

EXPORT CONST vfloat xacosf(vfloat d) {
  vfloat x2 = vmul_vf_vf_vf(vsub_vf_vf_vf(vcast_vf_f(1), vabs_vf_vf(d)), vcast_vf_f(0.5f));
  vfloat x = vsqrt_vf_vf(x2), u;

  u = vcast_vf_f(+0.1083120629e+0);
  u = vmla_vf_vf_vf_vf(u, x2, vcast_vf_f(-0.8821133524e-1));
  u = vmla_vf_vf_vf_vf(u, x2, vcast_vf_f(+0.7465086877e-1));
  u = vmla_vf_vf_vf_vf(u, x2, vcast_vf_f(+0.1687940583e-1));
  u = vmla_vf_vf_vf_vf(u, x2, vcast_vf_f(+0.4648575932e-1));
  u = vmla_vf_vf_vf_vf(u, x2, vcast_vf_f(+0.7487771660e-1));
  u = vmla_vf_vf_vf_vf(u, x2, vcast_vf_f(+0.1666697413e+0));
  u = vmla_vf_vf_vf_vf(vmul_vf_vf_vf(u, x), x2, x);

  vfloat r = vmul_vf_vf_vf(vcast_vf_f(2), u);

  return vsel_vf_vo_vf_vf(vsignbit_vo_vf(d), vsub_vf_vf_vf(vcast_vf_f(M_PIf), r), r);
}

//

static INLINE CONST vfloat2 atan2kf_u1(vfloat2 y, vfloat2 x) {
  vfloat u;
  vfloat2 s, t;
  vint2 q;
  vopmask p;
  vmask r;

  q = vsel_vi2_vf_vf_vi2_vi2(x.x, vcast_vf_f(0), vcast_vi2_i(-2), vcast_vi2_i(0));
  p = vlt_vo_vf_vf(x.x, vcast_vf_f(0));
  r = vand_vm_vo32_vm(p, vreinterpret_vm_vf(vcast_vf_f(-0.0)));
  x.x = vreinterpret_vf_vm(vxor_vm_vm_vm(vreinterpret_vm_vf(x.x), r));
  x.y = vreinterpret_vf_vm(vxor_vm_vm_vm(vreinterpret_vm_vf(x.y), r));

  q = vsel_vi2_vf_vf_vi2_vi2(x.x, y.x, vadd_vi2_vi2_vi2(q, vcast_vi2_i(1)), q);
  p = vlt_vo_vf_vf(x.x, y.x);
  s = vsel_vf2_vo_vf2_vf2(p, dfneg_vf2_vf2(x), y);
  t = vsel_vf2_vo_vf2_vf2(p, y, x);

  s = dfdiv_vf2_vf2_vf2(s, t);
  t = dfsqu_vf2_vf2(s);
  t = dfnormalize_vf2_vf2(t);

  u = vcast_vf_f(-0.00176397908944636583328247f);
  u = vmla_vf_vf_vf_vf(u, t.x, vcast_vf_f(0.0107900900766253471374512f));
  u = vmla_vf_vf_vf_vf(u, t.x, vcast_vf_f(-0.0309564601629972457885742f));
  u = vmla_vf_vf_vf_vf(u, t.x, vcast_vf_f(0.0577365085482597351074219f));
  u = vmla_vf_vf_vf_vf(u, t.x, vcast_vf_f(-0.0838950723409652709960938f));
  u = vmla_vf_vf_vf_vf(u, t.x, vcast_vf_f(0.109463557600975036621094f));
  u = vmla_vf_vf_vf_vf(u, t.x, vcast_vf_f(-0.142626821994781494140625f));
  u = vmla_vf_vf_vf_vf(u, t.x, vcast_vf_f(0.199983194470405578613281f));

  t = dfmul_vf2_vf2_vf2(t, dfadd_vf2_vf_vf(vcast_vf_f(-0.333332866430282592773438f), vmul_vf_vf_vf(u, t.x)));
  t = dfmul_vf2_vf2_vf2(s, dfadd_vf2_vf_vf2(vcast_vf_f(1), t));
  t = dfadd2_vf2_vf2_vf2(dfmul_vf2_vf2_vf(vcast_vf2_f_f(1.5707963705062866211f, -4.3711388286737928865e-08f), vcast_vf_vi2(q)), t);

  return t;
}

EXPORT CONST vfloat xatan2f_u1(vfloat y, vfloat x) {
  vopmask o = vlt_vo_vf_vf(vabs_vf_vf(x), vcast_vf_f(2.9387372783541830947e-39f)); // nexttowardf((1.0 / FLT_MAX), 1)
  x = vsel_vf_vo_vf_vf(o, vmul_vf_vf_vf(x, vcast_vf_f(1 << 24)), x);
  y = vsel_vf_vo_vf_vf(o, vmul_vf_vf_vf(y, vcast_vf_f(1 << 24)), y);

  vfloat2 d = atan2kf_u1(vcast_vf2_vf_vf(vabs_vf_vf(y), vcast_vf_f(0)), vcast_vf2_vf_vf(x, vcast_vf_f(0)));
  vfloat r = vadd_vf_vf_vf(d.x, d.y);

  r = vmulsign_vf_vf_vf(r, x);
  r = vsel_vf_vo_vf_vf(vor_vo_vo_vo(visinf_vo_vf(x), veq_vo_vf_vf(x, vcast_vf_f(0))), vsub_vf_vf_vf(vcast_vf_f(M_PI/2), visinf2_vf_vf_vf(x, vmulsign_vf_vf_vf(vcast_vf_f(M_PI/2), x))), r);
  r = vsel_vf_vo_vf_vf(visinf_vo_vf(y), vsub_vf_vf_vf(vcast_vf_f(M_PI/2), visinf2_vf_vf_vf(x, vmulsign_vf_vf_vf(vcast_vf_f(M_PI/4), x))), r);
  r = vsel_vf_vo_vf_vf(veq_vo_vf_vf(y, vcast_vf_f(0.0f)), vreinterpret_vf_vm(vand_vm_vo32_vm(vsignbit_vo_vf(x), vreinterpret_vm_vf(vcast_vf_f((float)M_PI)))), r);

  r = vreinterpret_vf_vm(vor_vm_vo32_vm(vor_vo_vo_vo(visnan_vo_vf(x), visnan_vo_vf(y)), vreinterpret_vm_vf(vmulsign_vf_vf_vf(r, y))));
  return r;
}

EXPORT CONST vfloat xasinf_u1(vfloat d) {
  vopmask o = vlt_vo_vf_vf(vabs_vf_vf(d), vcast_vf_f(0.707f));
  vfloat x2 = vsel_vf_vo_vf_vf(o, vmul_vf_vf_vf(d, d), vmul_vf_vf_vf(vsub_vf_vf_vf(vcast_vf_f(1), vabs_vf_vf(d)), vcast_vf_f(0.5f))), u;
  vfloat2 x = vsel_vf2_vo_vf2_vf2(o, vcast_vf2_vf_vf(vabs_vf_vf(d), vcast_vf_f(0)), dfsqrt_vf2_vf(x2));
  x = vsel_vf2_vo_vf2_vf2(veq_vo_vf_vf(vabs_vf_vf(d), vcast_vf_f(1.0f)), vcast_vf2_f_f(0, 0), x);

  u = vcast_vf_f(+0.1083120629e+0);
  u = vmla_vf_vf_vf_vf(u, x2, vcast_vf_f(-0.8821133524e-1));
  u = vmla_vf_vf_vf_vf(u, x2, vcast_vf_f(+0.7465086877e-1));
  u = vmla_vf_vf_vf_vf(u, x2, vcast_vf_f(+0.1687940583e-1));
  u = vmla_vf_vf_vf_vf(u, x2, vcast_vf_f(+0.4648575932e-1));
  u = vmla_vf_vf_vf_vf(u, x2, vcast_vf_f(+0.7487771660e-1));
  u = vmla_vf_vf_vf_vf(u, x2, vcast_vf_f(+0.1666697413e+0));
  u = vmul_vf_vf_vf(vmul_vf_vf_vf(u, x2), x.x);

  vfloat2 y = dfsub_vf2_vf2_vf(dfsub_vf2_vf2_vf2(vcast_vf2_f_f(3.1415927410125732422f/4,-8.7422776573475857731e-08f/4), x), u);

  vfloat r = vsel_vf_vo_vf_vf(o, vadd_vf_vf_vf(u, x.x),
			       vmul_vf_vf_vf(vadd_vf_vf_vf(y.x, y.y), vcast_vf_f(2)));
  return vmulsign_vf_vf_vf(r, d);
}

EXPORT CONST vfloat xacosf_u1(vfloat d) {
  vopmask o = vlt_vo_vf_vf(vabs_vf_vf(d), vcast_vf_f(0.707f));
  vfloat x2 = vsel_vf_vo_vf_vf(o, vmul_vf_vf_vf(d, d), vmul_vf_vf_vf(vsub_vf_vf_vf(vcast_vf_f(1), vabs_vf_vf(d)), vcast_vf_f(0.5f))), u;
  vfloat2 x = vsel_vf2_vo_vf2_vf2(o, vcast_vf2_vf_vf(vabs_vf_vf(d), vcast_vf_f(0)), dfsqrt_vf2_vf(x2));
  x = vsel_vf2_vo_vf2_vf2(veq_vo_vf_vf(vabs_vf_vf(d), vcast_vf_f(1.0f)), vcast_vf2_f_f(0, 0), x);

  u = vcast_vf_f(+0.1083120629e+0);
  u = vmla_vf_vf_vf_vf(u, x2, vcast_vf_f(-0.8821133524e-1));
  u = vmla_vf_vf_vf_vf(u, x2, vcast_vf_f(+0.7465086877e-1));
  u = vmla_vf_vf_vf_vf(u, x2, vcast_vf_f(+0.1687940583e-1));
  u = vmla_vf_vf_vf_vf(u, x2, vcast_vf_f(+0.4648575932e-1));
  u = vmla_vf_vf_vf_vf(u, x2, vcast_vf_f(+0.7487771660e-1));
  u = vmla_vf_vf_vf_vf(u, x2, vcast_vf_f(+0.1666697413e+0));
  u = vmul_vf_vf_vf(vmul_vf_vf_vf(u, x.x), x2);

  vfloat2 y = dfsub_vf2_vf2_vf2(vcast_vf2_f_f(3.1415927410125732422f/2,-8.7422776573475857731e-08f/2),
				 dfadd_vf2_vf_vf(vmulsign_vf_vf_vf(x.x, d), vmulsign_vf_vf_vf(u, d)));
  x = dfadd_vf2_vf2_vf(x, u);

  vfloat r = vsel_vf_vo_vf_vf(o, vadd_vf_vf_vf(y.x, y.y),
			       vmul_vf_vf_vf(vadd_vf_vf_vf(x.x, x.y), vcast_vf_f(2)));

  return vsel_vf_vo_vf_vf(vandnot_vo_vo_vo(o, vsignbit_vo_vf(d)), vsub_vf_vf_vf(vcast_vf_f(M_PIf), r), r);
}

EXPORT CONST vfloat xatanf_u1(vfloat d) {
  vfloat2 d2 = atan2kf_u1(vcast_vf2_vf_vf(vabs_vf_vf(d), vcast_vf_f(0)), vcast_vf2_f_f(1, 0));
  vfloat r = vadd_vf_vf_vf(d2.x, d2.y);
  r = vsel_vf_vo_vf_vf(visinf_vo_vf(d), vcast_vf_f(1.570796326794896557998982), r);
  return vmulsign_vf_vf_vf(r, d);
}

//

EXPORT CONST vfloat xlogf(vfloat d) {
  vfloat x, x2, t, m;

#ifndef ENABLE_AVX512F
  vopmask o = vlt_vo_vf_vf(d, vcast_vf_f(FLT_MIN));
  d = vsel_vf_vo_vf_vf(o, vmul_vf_vf_vf(d, vcast_vf_f((float)(1LL << 32) * (float)(1LL << 32))), d);
  vint2 e = vilogb2k_vi2_vf(vmul_vf_vf_vf(d, vcast_vf_f(1.0f/0.75f)));
  m = vldexp3_vf_vf_vi2(d, vneg_vi2_vi2(e));
  e = vsel_vi2_vo_vi2_vi2(o, vsub_vi2_vi2_vi2(e, vcast_vi2_i(64)), e);
#else
  vfloat e = vgetexp_vf_vf(vmul_vf_vf_vf(d, vcast_vf_f(1.0f/0.75f)));
  e = vsel_vf_vo_vf_vf(vispinf_vo_vf(e), vcast_vf_f(128.0f), e);
  m = vgetmant_vf_vf(d);
#endif

  x = vdiv_vf_vf_vf(vadd_vf_vf_vf(vcast_vf_f(-1.0f), m), vadd_vf_vf_vf(vcast_vf_f(1.0f), m));
  x2 = vmul_vf_vf_vf(x, x);

  t = vcast_vf_f(0.2392828464508056640625f);
  t = vmla_vf_vf_vf_vf(t, x2, vcast_vf_f(0.28518211841583251953125f));
  t = vmla_vf_vf_vf_vf(t, x2, vcast_vf_f(0.400005877017974853515625f));
  t = vmla_vf_vf_vf_vf(t, x2, vcast_vf_f(0.666666686534881591796875f));
  t = vmla_vf_vf_vf_vf(t, x2, vcast_vf_f(2.0f));

#ifndef ENABLE_AVX512F
  x = vmla_vf_vf_vf_vf(x, t, vmul_vf_vf_vf(vcast_vf_f(0.693147180559945286226764f), vcast_vf_vi2(e)));
  x = vsel_vf_vo_vf_vf(vispinf_vo_vf(d), vcast_vf_f(INFINITYf), x);
  x = vsel_vf_vo_vf_vf(vor_vo_vo_vo(vlt_vo_vf_vf(d, vcast_vf_f(0)), visnan_vo_vf(d)), vcast_vf_f(NANf), x);
  x = vsel_vf_vo_vf_vf(veq_vo_vf_vf(d, vcast_vf_f(0)), vcast_vf_f(-INFINITYf), x);
#else
  x = vmla_vf_vf_vf_vf(x, t, vmul_vf_vf_vf(vcast_vf_f(0.693147180559945286226764f), e));
  x = vfixup_vf_vf_vf_vi2_i(x, d, vcast_vi2_i((5 << (5*4))), 0);
#endif

  return x;
}

EXPORT CONST vfloat xexpf(vfloat d) {
  vint2 q = vrint_vi2_vf(vmul_vf_vf_vf(d, vcast_vf_f(R_LN2f)));
  vfloat s, u;

  s = vmla_vf_vf_vf_vf(vcast_vf_vi2(q), vcast_vf_f(-L2Uf), d);
  s = vmla_vf_vf_vf_vf(vcast_vf_vi2(q), vcast_vf_f(-L2Lf), s);

  u = vcast_vf_f(0.000198527617612853646278381);
  u = vmla_vf_vf_vf_vf(u, s, vcast_vf_f(0.00139304355252534151077271));
  u = vmla_vf_vf_vf_vf(u, s, vcast_vf_f(0.00833336077630519866943359));
  u = vmla_vf_vf_vf_vf(u, s, vcast_vf_f(0.0416664853692054748535156));
  u = vmla_vf_vf_vf_vf(u, s, vcast_vf_f(0.166666671633720397949219));
  u = vmla_vf_vf_vf_vf(u, s, vcast_vf_f(0.5));

  u = vadd_vf_vf_vf(vcast_vf_f(1.0f), vmla_vf_vf_vf_vf(vmul_vf_vf_vf(s, s), u, s));

  u = vldexp2_vf_vf_vi2(u, q);

  u = vreinterpret_vf_vm(vandnot_vm_vo32_vm(vlt_vo_vf_vf(d, vcast_vf_f(-104)), vreinterpret_vm_vf(u)));
  u = vsel_vf_vo_vf_vf(vlt_vo_vf_vf(vcast_vf_f(100), d), vcast_vf_f(INFINITYf), u);

  return u;
}

#ifdef ENABLE_NEON32
EXPORT CONST vfloat xsqrtf_u35(vfloat d) {
  vfloat e = vreinterpret_vf_vi2(vadd_vi2_vi2_vi2(vcast_vi2_i(0x20000000), vand_vi2_vi2_vi2(vcast_vi2_i(0x7f000000), vsrl_vi2_vi2_i(vreinterpret_vi2_vf(d), 1))));
  vfloat m = vreinterpret_vf_vi2(vadd_vi2_vi2_vi2(vcast_vi2_i(0x3f000000), vand_vi2_vi2_vi2(vcast_vi2_i(0x01ffffff), vreinterpret_vi2_vf(d))));
  float32x4_t x = vrsqrteq_f32(m);
  x = vmulq_f32(x, vrsqrtsq_f32(m, vmulq_f32(x, x)));
  float32x4_t u = vmulq_f32(x, m);
  u = vmlaq_f32(u, vmlsq_f32(m, u, u), vmulq_f32(x, vdupq_n_f32(0.5)));
  e = vreinterpret_vf_vm(vandnot_vm_vo32_vm(veq_vo_vf_vf(d, vcast_vf_f(0)), vreinterpret_vm_vf(e)));
  u = vmul_vf_vf_vf(e, u);

  u = vsel_vf_vo_vf_vf(visinf_vo_vf(d), vcast_vf_f(INFINITYf), u);
  u = vreinterpret_vf_vm(vor_vm_vo32_vm(vor_vo_vo_vo(visnan_vo_vf(d), vlt_vo_vf_vf(d, vcast_vf_f(0))), vreinterpret_vm_vf(u)));
  u = vmulsign_vf_vf_vf(u, d);

  return u;
}
#elif defined(ENABLE_CLANGVEC)
EXPORT CONST vfloat xsqrtf_u35(vfloat d) {
  vfloat q = vsqrt_vf_vf(d);
  q = vsel_vf_vo_vf_vf(visnegzero_vo_vf(d), vcast_vf_f(-0.0), q);
  return vsel_vf_vo_vf_vf(vispinf_vo_vf(d), INFINITYf, q);
}
#else
EXPORT CONST vfloat xsqrtf_u35(vfloat d) { return vsqrt_vf_vf(d); }
#endif

EXPORT CONST vfloat xcbrtf(vfloat d) {
  vfloat x, y, q = vcast_vf_f(1.0), t;
  vint2 e, qu, re;

#ifdef ENABLE_AVX512F
  vfloat s = d;
#endif
  e = vadd_vi2_vi2_vi2(vilogbk_vi2_vf(vabs_vf_vf(d)), vcast_vi2_i(1));
  d = vldexp2_vf_vf_vi2(d, vneg_vi2_vi2(e));

  t = vadd_vf_vf_vf(vcast_vf_vi2(e), vcast_vf_f(6144));
  qu = vtruncate_vi2_vf(vmul_vf_vf_vf(t, vcast_vf_f(1.0f/3.0f)));
  re = vtruncate_vi2_vf(vsub_vf_vf_vf(t, vmul_vf_vf_vf(vcast_vf_vi2(qu), vcast_vf_f(3))));

  q = vsel_vf_vo_vf_vf(veq_vo_vi2_vi2(re, vcast_vi2_i(1)), vcast_vf_f(1.2599210498948731647672106f), q);
  q = vsel_vf_vo_vf_vf(veq_vo_vi2_vi2(re, vcast_vi2_i(2)), vcast_vf_f(1.5874010519681994747517056f), q);
  q = vldexp2_vf_vf_vi2(q, vsub_vi2_vi2_vi2(qu, vcast_vi2_i(2048)));

  q = vmulsign_vf_vf_vf(q, d);
  d = vabs_vf_vf(d);

  x = vcast_vf_f(-0.601564466953277587890625f);
  x = vmla_vf_vf_vf_vf(x, d, vcast_vf_f(2.8208892345428466796875f));
  x = vmla_vf_vf_vf_vf(x, d, vcast_vf_f(-5.532182216644287109375f));
  x = vmla_vf_vf_vf_vf(x, d, vcast_vf_f(5.898262500762939453125f));
  x = vmla_vf_vf_vf_vf(x, d, vcast_vf_f(-3.8095417022705078125f));
  x = vmla_vf_vf_vf_vf(x, d, vcast_vf_f(2.2241256237030029296875f));

  y = vmul_vf_vf_vf(vmul_vf_vf_vf(d, x), x);
  y = vmul_vf_vf_vf(vsub_vf_vf_vf(y, vmul_vf_vf_vf(vmul_vf_vf_vf(vcast_vf_f(2.0f / 3.0f), y), vmla_vf_vf_vf_vf(y, x, vcast_vf_f(-1.0f)))), q);

#ifdef ENABLE_AVX512F
  y = vsel_vf_vo_vf_vf(visinf_vo_vf(s), vmulsign_vf_vf_vf(vcast_vf_f(INFINITYf), s), y);
  y = vsel_vf_vo_vf_vf(veq_vo_vf_vf(s, vcast_vf_f(0)), vmulsign_vf_vf_vf(vcast_vf_f(0), s), y);
#endif

  return y;
}

EXPORT CONST vfloat xcbrtf_u1(vfloat d) {
  vfloat x, y, z, t;
  vfloat2 q2 = vcast_vf2_f_f(1, 0), u, v;
  vint2 e, qu, re;

#ifdef ENABLE_AVX512F
  vfloat s = d;
#endif
  e = vadd_vi2_vi2_vi2(vilogbk_vi2_vf(vabs_vf_vf(d)), vcast_vi2_i(1));
  d = vldexp2_vf_vf_vi2(d, vneg_vi2_vi2(e));

  t = vadd_vf_vf_vf(vcast_vf_vi2(e), vcast_vf_f(6144));
  qu = vtruncate_vi2_vf(vmul_vf_vf_vf(t, vcast_vf_f(1.0/3.0)));
  re = vtruncate_vi2_vf(vsub_vf_vf_vf(t, vmul_vf_vf_vf(vcast_vf_vi2(qu), vcast_vf_f(3))));

  q2 = vsel_vf2_vo_vf2_vf2(veq_vo_vi2_vi2(re, vcast_vi2_i(1)), vcast_vf2_f_f(1.2599210739135742188f, -2.4018701694217270415e-08), q2);
  q2 = vsel_vf2_vo_vf2_vf2(veq_vo_vi2_vi2(re, vcast_vi2_i(2)), vcast_vf2_f_f(1.5874010324478149414f,  1.9520385308169352356e-08), q2);

  q2.x = vmulsign_vf_vf_vf(q2.x, d); q2.y = vmulsign_vf_vf_vf(q2.y, d);
  d = vabs_vf_vf(d);

  x = vcast_vf_f(-0.601564466953277587890625f);
  x = vmla_vf_vf_vf_vf(x, d, vcast_vf_f(2.8208892345428466796875f));
  x = vmla_vf_vf_vf_vf(x, d, vcast_vf_f(-5.532182216644287109375f));
  x = vmla_vf_vf_vf_vf(x, d, vcast_vf_f(5.898262500762939453125f));
  x = vmla_vf_vf_vf_vf(x, d, vcast_vf_f(-3.8095417022705078125f));
  x = vmla_vf_vf_vf_vf(x, d, vcast_vf_f(2.2241256237030029296875f));

  y = vmul_vf_vf_vf(x, x); y = vmul_vf_vf_vf(y, y); x = vsub_vf_vf_vf(x, vmul_vf_vf_vf(vmlanp_vf_vf_vf_vf(d, y, x), vcast_vf_f(-1.0 / 3.0)));

  z = x;

  u = dfmul_vf2_vf_vf(x, x);
  u = dfmul_vf2_vf2_vf2(u, u);
  u = dfmul_vf2_vf2_vf(u, d);
  u = dfadd2_vf2_vf2_vf(u, vneg_vf_vf(x));
  y = vadd_vf_vf_vf(u.x, u.y);

  y = vmul_vf_vf_vf(vmul_vf_vf_vf(vcast_vf_f(-2.0 / 3.0), y), z);
  v = dfadd2_vf2_vf2_vf(dfmul_vf2_vf_vf(z, z), y);
  v = dfmul_vf2_vf2_vf(v, d);
  v = dfmul_vf2_vf2_vf2(v, q2);
  z = vldexp2_vf_vf_vi2(vadd_vf_vf_vf(v.x, v.y), vsub_vi2_vi2_vi2(qu, vcast_vi2_i(2048)));

  z = vsel_vf_vo_vf_vf(visinf_vo_vf(d), vmulsign_vf_vf_vf(vcast_vf_f(INFINITY), q2.x), z);
  z = vsel_vf_vo_vf_vf(veq_vo_vf_vf(d, vcast_vf_f(0)), vreinterpret_vf_vm(vsignbit_vm_vf(q2.x)), z);

#ifdef ENABLE_AVX512F
  z = vsel_vf_vo_vf_vf(visinf_vo_vf(s), vmulsign_vf_vf_vf(vcast_vf_f(INFINITYf), s), z);
  z = vsel_vf_vo_vf_vf(veq_vo_vf_vf(s, vcast_vf_f(0)), vmulsign_vf_vf_vf(vcast_vf_f(0), s), z);
#endif

  return z;
}

static INLINE CONST vfloat2 logkf(vfloat d) {
  vfloat2 x, x2;
  vfloat t, m;

#ifndef ENABLE_AVX512F
  vopmask o = vlt_vo_vf_vf(d, vcast_vf_f(FLT_MIN));
  d = vsel_vf_vo_vf_vf(o, vmul_vf_vf_vf(d, vcast_vf_f((float)(1LL << 32) * (float)(1LL << 32))), d);
  vint2 e = vilogb2k_vi2_vf(vmul_vf_vf_vf(d, vcast_vf_f(1.0f/0.75f)));
  m = vldexp3_vf_vf_vi2(d, vneg_vi2_vi2(e));
  e = vsel_vi2_vo_vi2_vi2(o, vsub_vi2_vi2_vi2(e, vcast_vi2_i(64)), e);
#else
  vfloat e = vgetexp_vf_vf(vmul_vf_vf_vf(d, vcast_vf_f(1.0f/0.75f)));
  e = vsel_vf_vo_vf_vf(vispinf_vo_vf(e), vcast_vf_f(128.0f), e);
  m = vgetmant_vf_vf(d);
#endif

  x = dfdiv_vf2_vf2_vf2(dfadd2_vf2_vf_vf(vcast_vf_f(-1), m), dfadd2_vf2_vf_vf(vcast_vf_f(1), m));
  x2 = dfsqu_vf2_vf2(x);

  t = vcast_vf_f(0.240320354700088500976562);
  t = vmla_vf_vf_vf_vf(t, x2.x, vcast_vf_f(0.285112679004669189453125));
  t = vmla_vf_vf_vf_vf(t, x2.x, vcast_vf_f(0.400007992982864379882812));
  vfloat2 c = vcast_vf2_f_f(0.66666662693023681640625f, 3.69183861259614332084311e-09f);

#ifndef ENABLE_AVX512F
  return dfadd2_vf2_vf2_vf2(dfmul_vf2_vf2_vf(vcast_vf2_vf_vf(vcast_vf_f(0.69314718246459960938f), vcast_vf_f(-1.904654323148236017e-09f)),
					     vcast_vf_vi2(e)),
			    dfadd2_vf2_vf2_vf2(dfscale_vf2_vf2_vf(x, vcast_vf_f(2)),
					       dfmul_vf2_vf2_vf2(dfmul_vf2_vf2_vf2(x2, x),
								 dfadd2_vf2_vf2_vf2(dfmul_vf2_vf2_vf(x2, t), c))));
#else
  return dfadd2_vf2_vf2_vf2(dfmul_vf2_vf2_vf(vcast_vf2_vf_vf(vcast_vf_f(0.69314718246459960938f), vcast_vf_f(-1.904654323148236017e-09f)), e),
			    dfadd2_vf2_vf2_vf2(dfscale_vf2_vf2_vf(x, vcast_vf_f(2)),
					       dfmul_vf2_vf2_vf2(dfmul_vf2_vf2_vf2(x2, x),
								 dfadd2_vf2_vf2_vf2(dfmul_vf2_vf2_vf(x2, t), c))));
#endif
}

EXPORT CONST vfloat xlogf_u1(vfloat d) {
  vfloat2 x;
  vfloat t, m, x2;

#ifndef ENABLE_AVX512F
  vopmask o = vlt_vo_vf_vf(d, vcast_vf_f(FLT_MIN));
  d = vsel_vf_vo_vf_vf(o, vmul_vf_vf_vf(d, vcast_vf_f((float)(1LL << 32) * (float)(1LL << 32))), d);
  vint2 e = vilogb2k_vi2_vf(vmul_vf_vf_vf(d, vcast_vf_f(1.0f/0.75f)));
  m = vldexp3_vf_vf_vi2(d, vneg_vi2_vi2(e));
  e = vsel_vi2_vo_vi2_vi2(o, vsub_vi2_vi2_vi2(e, vcast_vi2_i(64)), e);
#else
  vfloat e = vgetexp_vf_vf(vmul_vf_vf_vf(d, vcast_vf_f(1.0f/0.75f)));
  e = vsel_vf_vo_vf_vf(vispinf_vo_vf(e), vcast_vf_f(128.0f), e);
  m = vgetmant_vf_vf(d);
#endif

  x = dfdiv_vf2_vf2_vf2(dfadd2_vf2_vf_vf(vcast_vf_f(-1), m), dfadd2_vf2_vf_vf(vcast_vf_f(1), m));
  x2 = vmul_vf_vf_vf(x.x, x.x);

  t = vcast_vf_f(+0.3027294874e+0f);
  t = vmla_vf_vf_vf_vf(t, x2, vcast_vf_f(+0.3996108174e+0f));
  t = vmla_vf_vf_vf_vf(t, x2, vcast_vf_f(+0.6666694880e+0f));

#ifndef ENABLE_AVX512F
  vfloat2 s = dfadd2_vf2_vf2_vf2(dfmul_vf2_vf2_vf(vcast_vf2_f_f(0.69314718246459960938f, -1.904654323148236017e-09f), vcast_vf_vi2(e)),
				 dfadd2_vf2_vf2_vf(dfscale_vf2_vf2_vf(x, vcast_vf_f(2)), vmul_vf_vf_vf(vmul_vf_vf_vf(x2, x.x), t)));
#else
  vfloat2 s = dfadd2_vf2_vf2_vf2(dfmul_vf2_vf2_vf(vcast_vf2_f_f(0.69314718246459960938f, -1.904654323148236017e-09f), e),
				 dfadd2_vf2_vf2_vf(dfscale_vf2_vf2_vf(x, vcast_vf_f(2)), vmul_vf_vf_vf(vmul_vf_vf_vf(x2, x.x), t)));
#endif

  vfloat r = vadd_vf_vf_vf(s.x, s.y);

#ifndef ENABLE_AVX512F
  r = vsel_vf_vo_vf_vf(vispinf_vo_vf(d), vcast_vf_f(INFINITYf), r);
  r = vsel_vf_vo_vf_vf(vor_vo_vo_vo(vlt_vo_vf_vf(d, vcast_vf_f(0)), visnan_vo_vf(d)), vcast_vf_f(NANf), r);
  r = vsel_vf_vo_vf_vf(veq_vo_vf_vf(d, vcast_vf_f(0)), vcast_vf_f(-INFINITYf), r);
#else
  r = vfixup_vf_vf_vf_vi2_i(r, d, vcast_vi2_i((4 << (2*4)) | (3 << (4*4)) | (5 << (5*4)) | (2 << (6*4))), 0);
#endif

  return r;
}

static INLINE CONST vfloat expkf(vfloat2 d) {
  vfloat u = vmul_vf_vf_vf(vadd_vf_vf_vf(d.x, d.y), vcast_vf_f(R_LN2f));
  vint2 q = vrint_vi2_vf(u);
  vfloat2 s, t;

  s = dfadd2_vf2_vf2_vf(d, vmul_vf_vf_vf(vcast_vf_vi2(q), vcast_vf_f(-L2Uf)));
  s = dfadd2_vf2_vf2_vf(s, vmul_vf_vf_vf(vcast_vf_vi2(q), vcast_vf_f(-L2Lf)));

  s = dfnormalize_vf2_vf2(s);

  u = vcast_vf_f(0.00136324646882712841033936f);
  u = vmla_vf_vf_vf_vf(u, s.x, vcast_vf_f(0.00836596917361021041870117f));
  u = vmla_vf_vf_vf_vf(u, s.x, vcast_vf_f(0.0416710823774337768554688f));
  u = vmla_vf_vf_vf_vf(u, s.x, vcast_vf_f(0.166665524244308471679688f));
  u = vmla_vf_vf_vf_vf(u, s.x, vcast_vf_f(0.499999850988388061523438f));

  t = dfadd_vf2_vf2_vf2(s, dfmul_vf2_vf2_vf(dfsqu_vf2_vf2(s), u));

  t = dfadd_vf2_vf_vf2(vcast_vf_f(1), t);
  u = vadd_vf_vf_vf(t.x, t.y);
  u = vldexp_vf_vf_vi2(u, q);

  u = vreinterpret_vf_vm(vandnot_vm_vo32_vm(vlt_vo_vf_vf(d.x, vcast_vf_f(-104)), vreinterpret_vm_vf(u)));

  return u;
}

EXPORT CONST vfloat xpowf(vfloat x, vfloat y) {
#if 1
  vopmask yisint = vor_vo_vo_vo(veq_vo_vf_vf(vtruncate_vf_vf(y), y), vgt_vo_vf_vf(vabs_vf_vf(y), vcast_vf_f(1 << 24)));
  vopmask yisodd = vand_vo_vo_vo(vand_vo_vo_vo(veq_vo_vi2_vi2(vand_vi2_vi2_vi2(vtruncate_vi2_vf(y), vcast_vi2_i(1)), vcast_vi2_i(1)), yisint),
				 vlt_vo_vf_vf(vabs_vf_vf(y), vcast_vf_f(1 << 24)));

#ifdef ENABLE_NEON32
  yisodd = vandnot_vm_vo32_vm(visinf_vo_vf(y), yisodd);
#endif

  vfloat result = expkf(dfmul_vf2_vf2_vf(logkf(vabs_vf_vf(x)), y));

  result = vsel_vf_vo_vf_vf(visnan_vo_vf(result), vcast_vf_f(INFINITYf), result);

  result = vmul_vf_vf_vf(result,
			 vsel_vf_vo_vf_vf(vgt_vo_vf_vf(x, vcast_vf_f(0)),
					  vcast_vf_f(1),
					  vsel_vf_vo_vf_vf(yisint, vsel_vf_vo_vf_vf(yisodd, vcast_vf_f(-1.0f), vcast_vf_f(1)), vcast_vf_f(NANf))));

  vfloat efx = vmulsign_vf_vf_vf(vsub_vf_vf_vf(vabs_vf_vf(x), vcast_vf_f(1)), y);

  result = vsel_vf_vo_vf_vf(visinf_vo_vf(y),
			    vreinterpret_vf_vm(vandnot_vm_vo32_vm(vlt_vo_vf_vf(efx, vcast_vf_f(0.0f)),
								  vreinterpret_vm_vf(vsel_vf_vo_vf_vf(veq_vo_vf_vf(efx, vcast_vf_f(0.0f)),
												      vcast_vf_f(1.0f),
												      vcast_vf_f(INFINITYf))))),
			    result);

  result = vsel_vf_vo_vf_vf(vor_vo_vo_vo(visinf_vo_vf(x), veq_vo_vf_vf(x, vcast_vf_f(0))),
			    vmul_vf_vf_vf(vsel_vf_vo_vf_vf(yisodd, vsign_vf_vf(x), vcast_vf_f(1)),
					  vreinterpret_vf_vm(vandnot_vm_vo32_vm(vlt_vo_vf_vf(vsel_vf_vo_vf_vf(veq_vo_vf_vf(x, vcast_vf_f(0)), vneg_vf_vf(y), y), vcast_vf_f(0)),
										vreinterpret_vm_vf(vcast_vf_f(INFINITYf))))),
			    result);

  result = vreinterpret_vf_vm(vor_vm_vo32_vm(vor_vo_vo_vo(visnan_vo_vf(x), visnan_vo_vf(y)), vreinterpret_vm_vf(result)));

  result = vsel_vf_vo_vf_vf(vor_vo_vo_vo(veq_vo_vf_vf(y, vcast_vf_f(0)), veq_vo_vf_vf(x, vcast_vf_f(1))), vcast_vf_f(1), result);

  return result;
#else
  return expkf(dfmul_vf2_vf2_vf(logkf(x), y));
#endif
}

static INLINE CONST vfloat2 expk2f(vfloat2 d) {
  vfloat u = vmul_vf_vf_vf(vadd_vf_vf_vf(d.x, d.y), vcast_vf_f(R_LN2f));
  vint2 q = vrint_vi2_vf(u);
  vfloat2 s, t;

  s = dfadd2_vf2_vf2_vf(d, vmul_vf_vf_vf(vcast_vf_vi2(q), vcast_vf_f(-L2Uf)));
  s = dfadd2_vf2_vf2_vf(s, vmul_vf_vf_vf(vcast_vf_vi2(q), vcast_vf_f(-L2Lf)));

  u = vcast_vf_f(0.00136324646882712841033936f);
  u = vmla_vf_vf_vf_vf(u, s.x, vcast_vf_f(0.00836596917361021041870117f));
  u = vmla_vf_vf_vf_vf(u, s.x, vcast_vf_f(0.0416710823774337768554688f));
  u = vmla_vf_vf_vf_vf(u, s.x, vcast_vf_f(0.166665524244308471679688f));
  u = vmla_vf_vf_vf_vf(u, s.x, vcast_vf_f(0.499999850988388061523438f));

  t = dfadd_vf2_vf2_vf2(s, dfmul_vf2_vf2_vf(dfsqu_vf2_vf2(s), u));

  t = dfadd_vf2_vf_vf2(vcast_vf_f(1), t);
  return dfscale_vf2_vf2_vf(dfscale_vf2_vf2_vf(t, vcast_vf_f(2)),
			    vpow2i_vf_vi2(vsub_vi2_vi2_vi2(q, vcast_vi2_i(1))));
}

EXPORT CONST vfloat xsinhf(vfloat x) {
  vfloat y = vabs_vf_vf(x);
  vfloat2 d = expk2f(vcast_vf2_vf_vf(y, vcast_vf_f(0)));
  d = dfsub_vf2_vf2_vf2(d, dfrec_vf2_vf2(d));
  y = vmul_vf_vf_vf(vadd_vf_vf_vf(d.x, d.y), vcast_vf_f(0.5));

  y = vsel_vf_vo_vf_vf(vor_vo_vo_vo(vgt_vo_vf_vf(vabs_vf_vf(x), vcast_vf_f(89)),
				    visnan_vo_vf(y)), vcast_vf_f(INFINITYf), y);
  y = vmulsign_vf_vf_vf(y, x);
  y = vreinterpret_vf_vm(vor_vm_vo32_vm(visnan_vo_vf(x), vreinterpret_vm_vf(y)));

  return y;
}

EXPORT CONST vfloat xcoshf(vfloat x) {
  vfloat y = vabs_vf_vf(x);
  vfloat2 d = expk2f(vcast_vf2_vf_vf(y, vcast_vf_f(0)));
  d = dfadd_vf2_vf2_vf2(d, dfrec_vf2_vf2(d));
  y = vmul_vf_vf_vf(vadd_vf_vf_vf(d.x, d.y), vcast_vf_f(0.5));

  y = vsel_vf_vo_vf_vf(vor_vo_vo_vo(vgt_vo_vf_vf(vabs_vf_vf(x), vcast_vf_f(89)),
				    visnan_vo_vf(y)), vcast_vf_f(INFINITYf), y);
  y = vreinterpret_vf_vm(vor_vm_vo32_vm(visnan_vo_vf(x), vreinterpret_vm_vf(y)));

  return y;
}

EXPORT CONST vfloat xtanhf(vfloat x) {
  vfloat y = vabs_vf_vf(x);
  vfloat2 d = expk2f(vcast_vf2_vf_vf(y, vcast_vf_f(0)));
  vfloat2 e = dfrec_vf2_vf2(d);
  d = dfdiv_vf2_vf2_vf2(dfadd_vf2_vf2_vf2(d, dfneg_vf2_vf2(e)), dfadd_vf2_vf2_vf2(d, e));
  y = vadd_vf_vf_vf(d.x, d.y);

  y = vsel_vf_vo_vf_vf(vor_vo_vo_vo(vgt_vo_vf_vf(vabs_vf_vf(x), vcast_vf_f(8.664339742f)),
				    visnan_vo_vf(y)), vcast_vf_f(1.0f), y);
  y = vmulsign_vf_vf_vf(y, x);
  y = vreinterpret_vf_vm(vor_vm_vo32_vm(visnan_vo_vf(x), vreinterpret_vm_vf(y)));

  return y;
}

static INLINE CONST vfloat2 logk2f(vfloat2 d) {
  vfloat2 x, x2, m;
  vfloat t;
  vint2 e;

#ifndef ENABLE_AVX512F
  e = vilogbk_vi2_vf(vmul_vf_vf_vf(d.x, vcast_vf_f(1.0f/0.75f)));
#else
  e = vrint_vi2_vf(vgetexp_vf_vf(vmul_vf_vf_vf(d.x, vcast_vf_f(1.0f/0.75f))));
#endif
  m = dfscale_vf2_vf2_vf(d, vpow2i_vf_vi2(vneg_vi2_vi2(e)));

  x = dfdiv_vf2_vf2_vf2(dfadd2_vf2_vf2_vf(m, vcast_vf_f(-1)), dfadd2_vf2_vf2_vf(m, vcast_vf_f(1)));
  x2 = dfsqu_vf2_vf2(x);

  t = vcast_vf_f(0.2392828464508056640625f);
  t = vmla_vf_vf_vf_vf(t, x2.x, vcast_vf_f(0.28518211841583251953125f));
  t = vmla_vf_vf_vf_vf(t, x2.x, vcast_vf_f(0.400005877017974853515625f));
  t = vmla_vf_vf_vf_vf(t, x2.x, vcast_vf_f(0.666666686534881591796875f));

  return dfadd2_vf2_vf2_vf2(dfmul_vf2_vf2_vf(vcast_vf2_vf_vf(vcast_vf_f(0.69314718246459960938f), vcast_vf_f(-1.904654323148236017e-09f)),
					     vcast_vf_vi2(e)),
			    dfadd2_vf2_vf2_vf2(dfscale_vf2_vf2_vf(x, vcast_vf_f(2)), dfmul_vf2_vf2_vf(dfmul_vf2_vf2_vf2(x2, x), t)));
}

EXPORT CONST vfloat xasinhf(vfloat x) {
  vfloat y = vabs_vf_vf(x);
  vopmask o = vgt_vo_vf_vf(y, vcast_vf_f(1));
  vfloat2 d;

  d = vsel_vf2_vo_vf2_vf2(o, dfrec_vf2_vf(x), vcast_vf2_vf_vf(y, vcast_vf_f(0)));
  d = dfsqrt_vf2_vf2(dfadd2_vf2_vf2_vf(dfsqu_vf2_vf2(d), vcast_vf_f(1)));
  d = vsel_vf2_vo_vf2_vf2(o, dfmul_vf2_vf2_vf(d, y), d);

  d = logk2f(dfnormalize_vf2_vf2(dfadd2_vf2_vf2_vf(d, x)));
  y = vadd_vf_vf_vf(d.x, d.y);

  y = vsel_vf_vo_vf_vf(vor_vo_vo_vo(vgt_vo_vf_vf(vabs_vf_vf(x), vcast_vf_f(SQRT_FLT_MAX)),
				    visnan_vo_vf(y)),
		       vmulsign_vf_vf_vf(vcast_vf_f(INFINITYf), x), y);
  y = vreinterpret_vf_vm(vor_vm_vo32_vm(visnan_vo_vf(x), vreinterpret_vm_vf(y)));
  y = vsel_vf_vo_vf_vf(visnegzero_vo_vf(x), vcast_vf_f(-0.0), y);

  return y;
}

EXPORT CONST vfloat xacoshf(vfloat x) {
  vfloat2 d = logk2f(dfadd2_vf2_vf2_vf(dfmul_vf2_vf2_vf2(dfsqrt_vf2_vf2(dfadd2_vf2_vf_vf(x, vcast_vf_f(1))), dfsqrt_vf2_vf2(dfadd2_vf2_vf_vf(x, vcast_vf_f(-1)))), x));
  vfloat y = vadd_vf_vf_vf(d.x, d.y);

  y = vsel_vf_vo_vf_vf(vor_vo_vo_vo(vgt_vo_vf_vf(vabs_vf_vf(x), vcast_vf_f(SQRT_FLT_MAX)),
				    visnan_vo_vf(y)),
		       vcast_vf_f(INFINITYf), y);

  y = vreinterpret_vf_vm(vandnot_vm_vo32_vm(veq_vo_vf_vf(x, vcast_vf_f(1.0f)), vreinterpret_vm_vf(y)));

  y = vreinterpret_vf_vm(vor_vm_vo32_vm(vlt_vo_vf_vf(x, vcast_vf_f(1.0f)), vreinterpret_vm_vf(y)));
  y = vreinterpret_vf_vm(vor_vm_vo32_vm(visnan_vo_vf(x), vreinterpret_vm_vf(y)));

  return y;
}

EXPORT CONST vfloat xatanhf(vfloat x) {
  vfloat y = vabs_vf_vf(x);
  vfloat2 d = logk2f(dfdiv_vf2_vf2_vf2(dfadd2_vf2_vf_vf(vcast_vf_f(1), y), dfadd2_vf2_vf_vf(vcast_vf_f(1), vneg_vf_vf(y))));
  y = vreinterpret_vf_vm(vor_vm_vo32_vm(vgt_vo_vf_vf(y, vcast_vf_f(1.0)), vreinterpret_vm_vf(vsel_vf_vo_vf_vf(veq_vo_vf_vf(y, vcast_vf_f(1.0)), vcast_vf_f(INFINITYf), vmul_vf_vf_vf(vadd_vf_vf_vf(d.x, d.y), vcast_vf_f(0.5))))));

  y = vreinterpret_vf_vm(vor_vm_vo32_vm(vor_vo_vo_vo(visinf_vo_vf(x), visnan_vo_vf(y)), vreinterpret_vm_vf(y)));
  y = vmulsign_vf_vf_vf(y, x);
  y = vreinterpret_vf_vm(vor_vm_vo32_vm(visnan_vo_vf(x), vreinterpret_vm_vf(y)));

  return y;
}

EXPORT CONST vfloat xexp2f(vfloat a) {
  vfloat u = expkf(dfmul_vf2_vf2_vf(vcast_vf2_vf_vf(vcast_vf_f(0.69314718246459960938f), vcast_vf_f(-1.904654323148236017e-09f)), a));
  u = vsel_vf_vo_vf_vf(vgt_vo_vf_vf(a, vcast_vf_f(128.0f)), vcast_vf_f(INFINITYf), u);
  u = vreinterpret_vf_vm(vandnot_vm_vo32_vm(visminf_vo_vf(a), vreinterpret_vm_vf(u)));
  return u;
}

EXPORT CONST vfloat xexp10f(vfloat a) {
  vfloat u = expkf(dfmul_vf2_vf2_vf(vcast_vf2_vf_vf(vcast_vf_f(2.3025851249694824219f), vcast_vf_f(-3.1975436520781386207e-08f)), a));
  u = vsel_vf_vo_vf_vf(vgt_vo_vf_vf(a, vcast_vf_f(38.531839419103626f)), vcast_vf_f(INFINITYf), u);
  u = vreinterpret_vf_vm(vandnot_vm_vo32_vm(visminf_vo_vf(a), vreinterpret_vm_vf(u)));
  return u;
}

EXPORT CONST vfloat xexpm1f(vfloat a) {
  vfloat2 d = dfadd2_vf2_vf2_vf(expk2f(vcast_vf2_vf_vf(a, vcast_vf_f(0))), vcast_vf_f(-1.0));
  vfloat x = vadd_vf_vf_vf(d.x, d.y);
  x = vsel_vf_vo_vf_vf(vgt_vo_vf_vf(a, vcast_vf_f(88.72283172607421875f)), vcast_vf_f(INFINITYf), x);
  x = vsel_vf_vo_vf_vf(vlt_vo_vf_vf(a, vcast_vf_f(-16.635532333438687426013570f)), vcast_vf_f(-1), x);
  x = vsel_vf_vo_vf_vf(visnegzero_vo_vf(a), vcast_vf_f(-0.0f), x);
  return x;
}

EXPORT CONST vfloat xlog10f(vfloat a) {
  vfloat2 d = dfmul_vf2_vf2_vf2(logkf(a), vcast_vf2_vf_vf(vcast_vf_f(0.43429449200630187988f), vcast_vf_f(-1.0103050118726031315e-08f)));
  vfloat x = vadd_vf_vf_vf(d.x, d.y);

#ifndef ENABLE_AVX512F
  x = vsel_vf_vo_vf_vf(vispinf_vo_vf(a), vcast_vf_f(INFINITYf), x);
  x = vsel_vf_vo_vf_vf(vor_vo_vo_vo(vlt_vo_vf_vf(a, vcast_vf_f(0)), visnan_vo_vf(a)), vcast_vf_f(NANf), x);
  x = vsel_vf_vo_vf_vf(veq_vo_vf_vf(a, vcast_vf_f(0)), vcast_vf_f(-INFINITYf), x);
#else
  x = vfixup_vf_vf_vf_vi2_i(x, a, vcast_vi2_i((4 << (2*4)) | (3 << (4*4)) | (5 << (5*4)) | (2 << (6*4))), 0);
#endif

  return x;
}

EXPORT CONST vfloat xlog1pf(vfloat a) {
  vfloat2 d = logk2f(dfadd2_vf2_vf_vf(a, vcast_vf_f(1)));
  vfloat x = vadd_vf_vf_vf(d.x, d.y);

  x = vsel_vf_vo_vf_vf(vgt_vo_vf_vf(a, vcast_vf_f(1e+38)), vcast_vf_f(INFINITYf), x);
  x = vreinterpret_vf_vm(vor_vm_vo32_vm(vgt_vo_vf_vf(vcast_vf_f(-1), a), vreinterpret_vm_vf(x)));
  x = vsel_vf_vo_vf_vf(veq_vo_vf_vf(a, vcast_vf_f(-1)), vcast_vf_f(-INFINITYf), x);
  x = vsel_vf_vo_vf_vf(visnegzero_vo_vf(a), vcast_vf_f(-0.0f), x);

  return x;
}

//

EXPORT CONST vfloat xfabsf(vfloat x) { return vabs_vf_vf(x); }

EXPORT CONST vfloat xcopysignf(vfloat x, vfloat y) { return vcopysign_vf_vf_vf(x, y); }

EXPORT CONST vfloat xfmaxf(vfloat x, vfloat y) {
  return vsel_vf_vo_vf_vf(visnan_vo_vf(y), x, vsel_vf_vo_vf_vf(vgt_vo_vf_vf(x, y), x, y));
}

EXPORT CONST vfloat xfminf(vfloat x, vfloat y) {
  return vsel_vf_vo_vf_vf(visnan_vo_vf(y), x, vsel_vf_vo_vf_vf(vgt_vo_vf_vf(y, x), x, y));
}

EXPORT CONST vfloat xfdimf(vfloat x, vfloat y) {
  vfloat ret = vsub_vf_vf_vf(x, y);
  ret = vsel_vf_vo_vf_vf(vor_vo_vo_vo(vlt_vo_vf_vf(ret, vcast_vf_f(0)), veq_vo_vf_vf(x, y)), vcast_vf_f(0), ret);
  return ret;
}

EXPORT CONST vfloat xtruncf(vfloat x) {
  vfloat fr = vsub_vf_vf_vf(x, vcast_vf_vi2(vtruncate_vi2_vf(x)));
  return vsel_vf_vo_vf_vf(vor_vo_vo_vo(visinf_vo_vf(x), vge_vo_vf_vf(vabs_vf_vf(x), vcast_vf_f(1LL << 23))), x, vcopysign_vf_vf_vf(vsub_vf_vf_vf(x, fr), x));
}

EXPORT CONST vfloat xfloorf(vfloat x) {
  vfloat fr = vsub_vf_vf_vf(x, vcast_vf_vi2(vtruncate_vi2_vf(x)));
  fr = vsel_vf_vo_vf_vf(vlt_vo_vf_vf(fr, vcast_vf_f(0)), vadd_vf_vf_vf(fr, vcast_vf_f(1.0f)), fr);
  return vsel_vf_vo_vf_vf(vor_vo_vo_vo(visinf_vo_vf(x), vge_vo_vf_vf(vabs_vf_vf(x), vcast_vf_f(1LL << 23))), x, vcopysign_vf_vf_vf(vsub_vf_vf_vf(x, fr), x));
}

EXPORT CONST vfloat xceilf(vfloat x) {
  vfloat fr = vsub_vf_vf_vf(x, vcast_vf_vi2(vtruncate_vi2_vf(x)));
  fr = vsel_vf_vo_vf_vf(vle_vo_vf_vf(fr, vcast_vf_f(0)), fr, vsub_vf_vf_vf(fr, vcast_vf_f(1.0f)));
  return vsel_vf_vo_vf_vf(vor_vo_vo_vo(visinf_vo_vf(x), vge_vo_vf_vf(vabs_vf_vf(x), vcast_vf_f(1LL << 23))), x, vcopysign_vf_vf_vf(vsub_vf_vf_vf(x, fr), x));
}

EXPORT CONST vfloat xroundf(vfloat d) {
  vfloat x = vadd_vf_vf_vf(d, vcast_vf_f(0.5f));
  vfloat fr = vsub_vf_vf_vf(x, vcast_vf_vi2(vtruncate_vi2_vf(x)));
  x = vsel_vf_vo_vf_vf(vand_vo_vo_vo(vle_vo_vf_vf(x, vcast_vf_f(0)), veq_vo_vf_vf(fr, vcast_vf_f(0))), vsub_vf_vf_vf(x, vcast_vf_f(1.0f)), x);
  fr = vsel_vf_vo_vf_vf(vlt_vo_vf_vf(fr, vcast_vf_f(0)), vadd_vf_vf_vf(fr, vcast_vf_f(1.0f)), fr);
  return vsel_vf_vo_vf_vf(vor_vo_vo_vo(visinf_vo_vf(d), vge_vo_vf_vf(vabs_vf_vf(d), vcast_vf_f(1LL << 23))), d, vcopysign_vf_vf_vf(vsub_vf_vf_vf(x, fr), d));
}

EXPORT CONST vfloat xrintf(vfloat d) {
  vfloat x = vadd_vf_vf_vf(d, vcast_vf_f(0.5f));
  vopmask isodd = veq_vo_vi2_vi2(vand_vi2_vi2_vi2(vcast_vi2_i(1), vtruncate_vi2_vf(x)), vcast_vi2_i(1));
  vfloat fr = vsub_vf_vf_vf(x, vcast_vf_vi2(vtruncate_vi2_vf(x)));
  fr = vsel_vf_vo_vf_vf(vor_vo_vo_vo(vlt_vo_vf_vf(fr, vcast_vf_f(0)), vand_vo_vo_vo(veq_vo_vf_vf(fr, vcast_vf_f(0)), isodd)), vadd_vf_vf_vf(fr, vcast_vf_f(1.0f)), fr);
  vfloat ret = vsel_vf_vo_vf_vf(vor_vo_vo_vo(visinf_vo_vf(d), vge_vo_vf_vf(vabs_vf_vf(d), vcast_vf_f(1LL << 23))), d, vcopysign_vf_vf_vf(vsub_vf_vf_vf(x, fr), d));
  return ret;
}

EXPORT CONST vfloat2 xmodff(vfloat x) {
  vfloat fr = vsub_vf_vf_vf(x, vcast_vf_vi2(vtruncate_vi2_vf(x)));
  fr = vsel_vf_vo_vf_vf(vgt_vo_vf_vf(vabs_vf_vf(x), vcast_vf_f(1LL << 23)), vcast_vf_f(0), fr);

  vfloat2 ret;

  ret.x = vcopysign_vf_vf_vf(fr, x);
  ret.y = vcopysign_vf_vf_vf(vsub_vf_vf_vf(x, fr), x);

  return ret;
}

EXPORT CONST vfloat xfmaf(vfloat x, vfloat y, vfloat z) {
  vfloat h2 = vadd_vf_vf_vf(vmul_vf_vf_vf(x, y), z), q = vcast_vf_f(1);
  vopmask o = vlt_vo_vf_vf(vabs_vf_vf(h2), vcast_vf_f(1e-38f));
  {
    const float c0 = 1ULL << 25, c1 = c0 * c0, c2 = c1 * c1;
    x = vsel_vf_vo_vf_vf(o, vmul_vf_vf_vf(x, vcast_vf_f(c1)), x);
    y = vsel_vf_vo_vf_vf(o, vmul_vf_vf_vf(y, vcast_vf_f(c1)), y);
    z = vsel_vf_vo_vf_vf(o, vmul_vf_vf_vf(z, vcast_vf_f(c2)), z);
    q = vsel_vf_vo_vf_vf(o, vcast_vf_f(1.0f / c2), q);
  }
  o = vgt_vo_vf_vf(vabs_vf_vf(h2), vcast_vf_f(1e+38f));
  {
    const float c0 = 1ULL << 25, c1 = c0 * c0, c2 = c1 * c1;
    x = vsel_vf_vo_vf_vf(o, vmul_vf_vf_vf(x, vcast_vf_f(1.0f / c1)), x);
    y = vsel_vf_vo_vf_vf(o, vmul_vf_vf_vf(y, vcast_vf_f(1.0f / c1)), y);
    z = vsel_vf_vo_vf_vf(o, vmul_vf_vf_vf(z, vcast_vf_f(1.0f / c2)), z);
    q = vsel_vf_vo_vf_vf(o, vcast_vf_f(c2), q);
  }
  vfloat2 d = dfmul_vf2_vf_vf(x, y);
  d = dfadd2_vf2_vf2_vf(d, z);
  vfloat ret = vsel_vf_vo_vf_vf(vor_vo_vo_vo(veq_vo_vf_vf(x, vcast_vf_f(0)), veq_vo_vf_vf(y, vcast_vf_f(0))), z, vadd_vf_vf_vf(d.x, d.y));
  o = visinf_vo_vf(z);
  o = vandnot_vo_vo_vo(visinf_vo_vf(x), o);
  o = vandnot_vo_vo_vo(visnan_vo_vf(x), o);
  o = vandnot_vo_vo_vo(visinf_vo_vf(y), o);
  o = vandnot_vo_vo_vo(visnan_vo_vf(y), o);
  h2 = vsel_vf_vo_vf_vf(o, z, h2);

  o = vor_vo_vo_vo(visinf_vo_vf(h2), visnan_vo_vf(h2));

  return vsel_vf_vo_vf_vf(o, h2, vmul_vf_vf_vf(ret, q));
}

static INLINE CONST vint2 vcast_vi2_i_i(int i0, int i1) { return vcast_vi2_vm(vcast_vm_i_i(i0, i1)); }

EXPORT CONST vfloat xsqrtf_u05(vfloat d) {
  vfloat q;
  vopmask o;

  d = vsel_vf_vo_vf_vf(vlt_vo_vf_vf(d, vcast_vf_f(0)), vcast_vf_f(NANf), d);

  o = vlt_vo_vf_vf(d, vcast_vf_f(5.2939559203393770e-23f));
  d = vsel_vf_vo_vf_vf(o, vmul_vf_vf_vf(d, vcast_vf_f(1.8889465931478580e+22f)), d);
  q = vsel_vf_vo_vf_vf(o, vcast_vf_f(7.2759576141834260e-12f*0.5f), vcast_vf_f(0.5f));

  o = vgt_vo_vf_vf(d, vcast_vf_f(1.8446744073709552e+19f));
  d = vsel_vf_vo_vf_vf(o, vmul_vf_vf_vf(d, vcast_vf_f(5.4210108624275220e-20f)), d);
  q = vsel_vf_vo_vf_vf(o, vcast_vf_f(4294967296.0f * 0.5f), q);

  vfloat x = vreinterpret_vf_vi2(vsub_vi2_vi2_vi2(vcast_vi2_i(0x5f375a86), vsrl_vi2_vi2_i(vreinterpret_vi2_vf(vadd_vf_vf_vf(d, vcast_vf_f(1e-45f))), 1)));

  x = vmul_vf_vf_vf(x, vsub_vf_vf_vf(vcast_vf_f(1.5f), vmul_vf_vf_vf(vmul_vf_vf_vf(vmul_vf_vf_vf(vcast_vf_f(0.5f), d), x), x)));
  x = vmul_vf_vf_vf(x, vsub_vf_vf_vf(vcast_vf_f(1.5f), vmul_vf_vf_vf(vmul_vf_vf_vf(vmul_vf_vf_vf(vcast_vf_f(0.5f), d), x), x)));
  x = vmul_vf_vf_vf(x, vsub_vf_vf_vf(vcast_vf_f(1.5f), vmul_vf_vf_vf(vmul_vf_vf_vf(vmul_vf_vf_vf(vcast_vf_f(0.5f), d), x), x)));
  x = vmul_vf_vf_vf(x, d);

  vfloat2 d2 = dfmul_vf2_vf2_vf2(dfadd2_vf2_vf_vf2(d, dfmul_vf2_vf_vf(x, x)), dfrec_vf2_vf(x));

  x = vmul_vf_vf_vf(vadd_vf_vf_vf(d2.x, d2.y), q);

  x = vsel_vf_vo_vf_vf(vispinf_vo_vf(d), vcast_vf_f(INFINITYf), x);
  x = vsel_vf_vo_vf_vf(veq_vo_vf_vf(d, vcast_vf_f(0)), d, x);

  return x;
}

EXPORT CONST vfloat xhypotf_u05(vfloat x, vfloat y) {
#if 1
  vfloat B = vmul_vf_vf_vf(y, y);
  vfloat AB = vmla_vf_vf_vf_vf(x, x , B);
  return vsqrt_vf_vf(AB);
#else
  x = vabs_vf_vf(x);
  y = vabs_vf_vf(y);
  vfloat min = vmin_vf_vf_vf(x, y), n = min;
  vfloat max = vmax_vf_vf_vf(x, y), d = max;

  vopmask o = vlt_vo_vf_vf(max, vcast_vf_f(FLT_MIN));
  n = vsel_vf_vo_vf_vf(o, vmul_vf_vf_vf(n, vcast_vf_f(1ULL << 24)), n);
  d = vsel_vf_vo_vf_vf(o, vmul_vf_vf_vf(d, vcast_vf_f(1ULL << 24)), d);

  vfloat2 t = dfdiv_vf2_vf2_vf2(vcast_vf2_vf_vf(n, vcast_vf_f(0)), vcast_vf2_vf_vf(d, vcast_vf_f(0)));
  t = dfmul_vf2_vf2_vf(dfsqrt_vf2_vf2(dfadd2_vf2_vf2_vf(dfsqu_vf2_vf2(t), vcast_vf_f(1))), max);
  vfloat ret = vadd_vf_vf_vf(t.x, t.y);
  ret = vsel_vf_vo_vf_vf(visnan_vo_vf(ret), vcast_vf_f(INFINITYf), ret);
  ret = vsel_vf_vo_vf_vf(veq_vo_vf_vf(min, vcast_vf_f(0)), max, ret);
  ret = vsel_vf_vo_vf_vf(vor_vo_vo_vo(visnan_vo_vf(x), visnan_vo_vf(y)), vcast_vf_f(NANf), ret);
  ret = vsel_vf_vo_vf_vf(vor_vo_vo_vo(veq_vo_vf_vf(x, vcast_vf_f(INFINITYf)), veq_vo_vf_vf(y, vcast_vf_f(INFINITYf))), vcast_vf_f(INFINITYf), ret);

  return ret;
#endif
}

EXPORT CONST vfloat xhypotf_u35(vfloat x, vfloat y) {
  x = vabs_vf_vf(x);
  y = vabs_vf_vf(y);
  vfloat min = vmin_vf_vf_vf(x, y), n = min;
  vfloat max = vmax_vf_vf_vf(x, y), d = max;

  vfloat t = vdiv_vf_vf_vf(min, max);
  vfloat ret = vmul_vf_vf_vf(max, vsqrt_vf_vf(vmla_vf_vf_vf_vf(t, t, vcast_vf_f(1))));
  ret = vsel_vf_vo_vf_vf(veq_vo_vf_vf(min, vcast_vf_f(0)), max, ret);
  ret = vsel_vf_vo_vf_vf(vor_vo_vo_vo(visnan_vo_vf(x), visnan_vo_vf(y)), vcast_vf_f(NANf), ret);
  ret = vsel_vf_vo_vf_vf(vor_vo_vo_vo(veq_vo_vf_vf(x, vcast_vf_f(INFINITYf)), veq_vo_vf_vf(y, vcast_vf_f(INFINITYf))), vcast_vf_f(INFINITYf), ret);

  return ret;
}

EXPORT CONST vfloat xnextafterf(vfloat x, vfloat y) {
  x = vsel_vf_vo_vf_vf(veq_vo_vf_vf(x, vcast_vf_f(0)), vmulsign_vf_vf_vf(vcast_vf_f(0), y), x);
  vint2 t, xi2 = vreinterpret_vi2_vf(x);
  vopmask c = vxor_vo_vo_vo(vsignbit_vo_vf(x), vge_vo_vf_vf(y, x));

  xi2 = vsel_vi2_vo_vi2_vi2(c, vsub_vi2_vi2_vi2(vcast_vi2_i(0), vxor_vi2_vi2_vi2(xi2, vcast_vi2_i(1 << 31))), xi2);

  xi2 = vsel_vi2_vo_vi2_vi2(vneq_vo_vf_vf(x, y), vsub_vi2_vi2_vi2(xi2, vcast_vi2_i(1)), xi2);

  xi2 = vsel_vi2_vo_vi2_vi2(c, vsub_vi2_vi2_vi2(vcast_vi2_i(0), vxor_vi2_vi2_vi2(xi2, vcast_vi2_i(1 << 31))), xi2);

  vfloat ret = vreinterpret_vf_vi2(xi2);

  ret = vsel_vf_vo_vf_vf(vand_vo_vo_vo(veq_vo_vf_vf(ret, vcast_vf_f(0)), vneq_vo_vf_vf(x, vcast_vf_f(0))),
			 vmulsign_vf_vf_vf(vcast_vf_f(0), x), ret);

  ret = vsel_vf_vo_vf_vf(vand_vo_vo_vo(veq_vo_vf_vf(x, vcast_vf_f(0)), veq_vo_vf_vf(y, vcast_vf_f(0))), y, ret);

  ret = vsel_vf_vo_vf_vf(vor_vo_vo_vo(visnan_vo_vf(x), visnan_vo_vf(y)), vcast_vf_f(NANf), ret);

  return ret;
}

EXPORT CONST vfloat xfrfrexpf(vfloat x) {
  x = vsel_vf_vo_vf_vf(vlt_vo_vf_vf(vabs_vf_vf(x), vcast_vf_f(FLT_MIN)), vmul_vf_vf_vf(x, vcast_vf_f(1ULL << 30)), x);

  vmask xm = vreinterpret_vm_vf(x);
  xm = vand_vm_vm_vm(xm, vcast_vm_i_i(~0x7f800000U, ~0x7f800000U));
  xm = vor_vm_vm_vm (xm, vcast_vm_i_i( 0x3f000000U,  0x3f000000U));

  vfloat ret = vreinterpret_vf_vm(xm);

  ret = vsel_vf_vo_vf_vf(visinf_vo_vf(x), vmulsign_vf_vf_vf(vcast_vf_f(INFINITYf), x), ret);
  ret = vsel_vf_vo_vf_vf(veq_vo_vf_vf(x, vcast_vf_f(0)), x, ret);

  return ret;
}

EXPORT CONST vint2 xexpfrexpf(vfloat x) {
  /*
  x = vsel_vf_vo_vf_vf(vlt_vo_vf_vf(vabs_vf_vf(x), vcast_vf_f(FLT_MIN)), vmul_vf_vf_vf(x, vcast_vf_f(1ULL << 63)), x);

  vint ret = vcastu_vi_vi2(vreinterpret_vi2_vf(x));
  ret = vsub_vi_vi_vi(vand_vi_vi_vi(vsrl_vi_vi_i(ret, 20), vcast_vi_i(0x7ff)), vcast_vi_i(0x3fe));

  ret = vsel_vi_vo_vi_vi(vor_vo_vo_vo(vor_vo_vo_vo(veq_vo_vf_vf(x, vcast_vf_f(0)), visnan_vo_vf(x)), visinf_vo_vf(x)), vcast_vi_i(0), ret);

  return ret;
  */
  return vcast_vi2_i(0);
}

static INLINE CONST vfloat vnexttoward0f(vfloat x) {
  vint2 xi2 = vreinterpret_vi2_vf(x);

  xi2 = vsub_vi2_vi2_vi2(xi2, vcast_vi2_i(1));

  vfloat ret = vreinterpret_vf_vi2(xi2);
  ret = vsel_vf_vo_vf_vf(veq_vo_vf_vf(x, vcast_vf_f(0)), vcast_vf_f(0), ret);

  return ret;
}

EXPORT CONST vfloat xfmodf(vfloat x, vfloat y) {
  vfloat nu = vabs_vf_vf(x), de = vabs_vf_vf(y), s = vcast_vf_f(1);
  vopmask o = vlt_vo_vf_vf(de, vcast_vf_f(FLT_MIN));
  nu = vsel_vf_vo_vf_vf(o, vmul_vf_vf_vf(nu, vcast_vf_f(1ULL << 25)), nu);
  de = vsel_vf_vo_vf_vf(o, vmul_vf_vf_vf(de, vcast_vf_f(1ULL << 25)), de);
  s  = vsel_vf_vo_vf_vf(o, vmul_vf_vf_vf(s , vcast_vf_f(1.0f / (1ULL << 25))), s);

  vfloat2 q, r = vcast_vf2_vf_vf(nu, vcast_vf_f(0));

  for(int i=0;i<6;i++) { // ceil(log2(FLT_MAX) / 23)
    q = dfnormalize_vf2_vf2(dfdiv_vf2_vf2_vf2(r, vcast_vf2_vf_vf(de, vcast_vf_f(0))));
    r = dfnormalize_vf2_vf2(dfadd2_vf2_vf2_vf2(r, dfmul_vf2_vf_vf(xtruncf(vsel_vf_vo_vf_vf(vlt_vo_vf_vf(q.y, vcast_vf_f(0)), vnexttoward0f(q.x), q.x)), vmul_vf_vf_vf(de, vcast_vf_f(-1)))));
    if (vtestallones_i_vo32(vlt_vo_vf_vf(r.x, y))) break;
  }

  vfloat ret = vmul_vf_vf_vf(vadd_vf_vf_vf(r.x, r.y), s);
  ret = vsel_vf_vo_vf_vf(veq_vo_vf_vf(vadd_vf_vf_vf(r.x, r.y), de), vcast_vf_f(0), ret);

  ret = vmulsign_vf_vf_vf(ret, x);

  ret = vsel_vf_vo_vf_vf(vlt_vo_vf_vf(vabs_vf_vf(x), vabs_vf_vf(y)), x, ret);

  return ret;
}

#ifdef ENABLE_MAIN
// gcc -DENABLE_MAIN -Wno-attributes -I../common -I../arch -DENABLE_AVX2 -mavx2 -mfma sleefsimdsp.c ../common/common.c -lm
#include <stdio.h>
#include <stdlib.h>
int main(int argc, char **argv) {
  vfloat vf1 = vcast_vf_f(atof(argv[1]));
  //vfloat vf2 = vcast_vf_f(atof(argv[2]));

  //vfloat r = xpowf(vf1, vf2);
  //vfloat r = xsqrtf_u05(vf1);
  //printf("%g\n", xnextafterf(vf1, vf2)[0]);
  //printf("%g\n", nextafterf(atof(argv[1]), atof(argv[2])));
  printf("t = %.20g\n", xlogf_u1(vf1)[0]);
  printf("c = %.20g\n", logf(atof(argv[1])));

}
#endif