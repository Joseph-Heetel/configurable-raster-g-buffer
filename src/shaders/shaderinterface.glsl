#if WORLDPOS
layout(location = 0) INTERFACEMODE vec3 WorldPos;
#endif
#if WORLDPOSOLD
layout(location = 1) INTERFACEMODE vec3 WorldPosOld;
#endif
#if DEVICEPOS
layout(location = 2) INTERFACEMODE vec4 DevicePos;
#endif
#if DEVICEPOSOLD
layout(location = 3) INTERFACEMODE vec4 DevicePosOld;
#endif
#if NORMAL
layout(location = 4) INTERFACEMODE vec3 Normal;
#endif
#if TANGENT
layout(location = 5) INTERFACEMODE vec3 Tangent;
#endif
#if UV
layout(location = 6) INTERFACEMODE vec2 UV;
#endif
#if MESHID
layout(location = 7) flat INTERFACEMODE uint MeshInstanceId;
#endif
