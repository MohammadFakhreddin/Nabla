#ifndef _NBL_BUILTIN_GLSL_EXT_MITSUBA_LOADER_INSTANCE_DATA_DESCRIPTOR_INCLUDED_
#define _NBL_BUILTIN_GLSL_EXT_MITSUBA_LOADER_INSTANCE_DATA_DESCRIPTOR_INCLUDED_


#include <nbl/builtin/glsl/ext/MitsubaLoader/instance_data_struct.glsl>


#ifndef _NBL_GLSL_EXT_MITSUBA_LOADER_INSTANCE_DATA_SSBO_
#define _NBL_GLSL_EXT_MITSUBA_LOADER_INSTANCE_DATA_SSBO_

    #ifndef _NBL_GLSL_EXT_MITSUBA_LOADER_INSTANCE_DATA_SET_
    #define _NBL_GLSL_EXT_MITSUBA_LOADER_INSTANCE_DATA_SET_ 0
    #endif
    #ifndef _NBL_GLSL_EXT_MITSUBA_LOADER_INSTANCE_DATA_BINDING_
    #define _NBL_GLSL_EXT_MITSUBA_LOADER_INSTANCE_DATA_BINDING_ 5
    #endif

layout(set = _NBL_GLSL_EXT_MITSUBA_LOADER_INSTANCE_DATA_SET_, binding = _NBL_GLSL_EXT_MITSUBA_LOADER_INSTANCE_DATA_BINDING_, row_major, std430) readonly restrict buffer InstDataBuffer {
    nbl_glsl_ext_Mitsuba_Loader_instance_data_t data[];
} InstData;
#endif


#endif