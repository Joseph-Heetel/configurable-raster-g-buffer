#version 450
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable

#define INTERFACEMODE in
#include "shaderinterface.glsl"

#if OUT_0
layout(location = 0) out OUT_0_TYPE out0;
#endif
#if OUT_1
layout(location = 1) out OUT_1_TYPE out1;
#endif
#if OUT_2
layout(location = 2) out OUT_2_TYPE out2;
#endif
#if OUT_3
layout(location = 3) out OUT_3_TYPE out3;
#endif
#if OUT_4
layout(location = 4) out OUT_4_TYPE out4;
#endif
#if OUT_5
layout(location = 5) out OUT_5_TYPE out5;
#endif
#if OUT_6
layout(location = 6) out OUT_6_TYPE out6;
#endif
#if OUT_7
layout(location = 7) out OUT_7_TYPE out7;
#endif

#include "bindpoints.glsl"
#include "common/gltf_pushc.glsl"
#include "common/materialbuffer.glsl"
#include "common/normaltbn.glsl"

void main()
{
#if MATERIALPROBE || NORMALMAPPING
    MaterialBufferObject material = GetMaterialOrFallback(PushConstant.MaterialIndex);

    MaterialProbe probe = ProbeMaterial(material, UV);
#else
#if MATERIALPROBEALPHA || ALPHATEST
#endif
#endif
#if ALPHATEST
#endif
#if NORMALMAPPING
#endif

#if OUT_0
    OUT_0_CALC
    out0 = OUT_0_TYPE(OUT_0_RESULT);
#endif
#if OUT_1
    OUT_1_CALC
    out1 = OUT_1_TYPE(OUT_1_RESULT);
#endif
#if OUT_2
    OUT_2_CALC
    out2 = OUT_2_TYPE(OUT_2_RESULT);
#endif
#if OUT_3
    OUT_3_CALC
    out3 = OUT_3_TYPE(OUT_3_RESULT);
#endif
#if OUT_4
    OUT_4_CALC
    out4 = OUT_4_TYPE(OUT_4_RESULT);
#endif
#if OUT_5
    OUT_5_CALC
    out5 = OUT_5_TYPE(OUT_5_RESULT);
#endif
#if OUT_6
    OUT_6_CALC
    out6 = OUT_6_TYPE(OUT_6_RESULT);
#endif
#if OUT_7
    OUT_7_CALC
    out7 = OUT_7_TYPE(OUT_7_RESULT);
#endif
}
