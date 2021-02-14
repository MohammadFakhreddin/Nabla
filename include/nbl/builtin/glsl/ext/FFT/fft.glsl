// Copyright (C) 2018-2020 - DevSH Graphics Programming Sp. z O.O.
// This file is part of the "Nabla Engine".
// For conditions of distribution and use, see copyright notice in nabla.h

#ifndef _NBL_GLSL_EXT_FFT_INCLUDED_
#define _NBL_GLSL_EXT_FFT_INCLUDED_

#include <nbl/builtin/glsl/math/complex.glsl>

// Shared Memory
#include <nbl/builtin/glsl/workgroup/shared_arithmetic.glsl>
#include <nbl/builtin/glsl/math/functions.glsl>


#ifndef _NBL_GLSL_EXT_FFT_MAX_DIM_SIZE_
#error "_NBL_GLSL_EXT_FFT_MAX_DIM_SIZE_ should be defined."
#endif

#ifndef _NBL_GLSL_EXT_FFT_MAX_ITEMS_PER_THREAD
#error "_NBL_GLSL_EXT_FFT_MAX_ITEMS_PER_THREAD should be defined."
#endif


// TODO: Investigate why +1 solves all glitches
#define _NBL_GLSL_EXT_FFT_SHARED_SIZE_NEEDED_ _NBL_GLSL_EXT_FFT_MAX_DIM_SIZE_ + 1

#ifdef _NBL_GLSL_SCRATCH_SHARED_DEFINED_
    #if NBL_GLSL_LESS(_NBL_GLSL_SCRATCH_SHARED_SIZE_DEFINED_,_NBL_GLSL_EXT_FFT_SHARED_SIZE_NEEDED_)
        #error "Not enough shared memory declared for ext::FFT !"
    #endif
#else
    #if NBL_GLSL_GREATER(_NBL_GLSL_EXT_FFT_SHARED_SIZE_NEEDED_,0)
        #define _NBL_GLSL_SCRATCH_SHARED_DEFINED_ nbl_glsl_fftScratchShared
        #define _NBL_GLSL_SCRATCH_SHARED_SIZE_DEFINED_ _NBL_GLSL_EXT_FFT_SHARED_SIZE_NEEDED_
        shared uint _NBL_GLSL_SCRATCH_SHARED_DEFINED_[_NBL_GLSL_EXT_FFT_SHARED_SIZE_NEEDED_];
    #endif
#endif

// Push Constants

#define _NBL_GLSL_EXT_FFT_DIRECTION_X_ 0
#define _NBL_GLSL_EXT_FFT_DIRECTION_Y_ 1
#define _NBL_GLSL_EXT_FFT_DIRECTION_Z_ 2

#define _NBL_GLSL_EXT_FFT_CLAMP_TO_EDGE_ 0
#define _NBL_GLSL_EXT_FFT_FILL_WITH_ZERO_ 1

//TODO: investigate why putting this uint between the 2 other uvec3's don't work
#ifndef _NBL_GLSL_EXT_FFT_PUSH_CONSTANTS_DEFINED_
#define _NBL_GLSL_EXT_FFT_PUSH_CONSTANTS_DEFINED_
layout(push_constant) uniform PushConstants
{
    layout (offset = 0) uvec3 dimension;
    layout (offset = 16) uvec3 padded_dimension;
	layout (offset = 32) uint direction_isInverse_paddingType; // packed into a uint
} pc;
#endif

#ifndef _NBL_GLSL_EXT_FFT_GET_DATA_DECLARED_
#define _NBL_GLSL_EXT_FFT_GET_DATA_DECLARED_
vec2 nbl_glsl_ext_FFT_getData(in uvec3 coordinate, in uint channel);
#endif

#ifndef _NBL_GLSL_EXT_FFT_SET_DATA_DECLARED_
#define _NBL_GLSL_EXT_FFT_SET_DATA_DECLARED_
void nbl_glsl_ext_FFT_setData(in uvec3 coordinate, in uint channel, in vec2 complex_value);
#endif

#ifndef _NBL_GLSL_EXT_FFT_GET_PADDED_DATA_DECLARED_
#define _NBL_GLSL_EXT_FFT_GET_PADDED_DATA_DECLARED_
vec2 nbl_glsl_ext_FFT_getPaddedData(in uvec3 coordinate, in uint channel);
#endif

#ifndef _NBL_GLSL_EXT_FFT_GET_DATA_DEFINED_
#error "You need to define `nbl_glsl_ext_FFT_getData` and mark `_NBL_GLSL_EXT_FFT_GET_DATA_DEFINED_`!"
#endif
#ifndef _NBL_GLSL_EXT_FFT_SET_DATA_DEFINED_
#error "You need to define `nbl_glsl_ext_FFT_setData` and mark `_NBL_GLSL_EXT_FFT_SET_DATA_DEFINED_`!"
#endif
#ifndef _NBL_GLSL_EXT_FFT_GET_PADDED_DATA_DEFINED_
#error "You need to define `nbl_glsl_ext_FFT_getPaddedData` and mark `_NBL_GLSL_EXT_FFT_GET_PADDED_DATA_DEFINED_`!"
#endif

uint nbl_glsl_ext_FFT_calculateTwiddlePower(in uint threadId, in uint iteration, in uint logTwoN) 
{
    const uint shiftSuffix = logTwoN - 1u - iteration; // can we assert that iteration<logTwoN always?? yes
    const uint suffixMask = (2u << iteration) - 1u;
    return (threadId & suffixMask) << shiftSuffix;
}

nbl_glsl_complex nbl_glsl_ext_FFT_twiddle(in uint threadId, in uint iteration, in uint logTwoN) 
{
    uint k = nbl_glsl_ext_FFT_calculateTwiddlePower(threadId, iteration, logTwoN);
    return nbl_glsl_expImaginary(-1.0f * 2.0f * nbl_glsl_PI * float(k) / (1 << logTwoN));
}

nbl_glsl_complex nbl_glsl_ext_FFT_twiddleInverse(in uint threadId, in uint iteration, in uint logTwoN) 
{
    return nbl_glsl_complex_conjugate(nbl_glsl_ext_FFT_twiddle(threadId, iteration, logTwoN));
}

uint nbl_glsl_ext_FFT_getDirection() {
    return (pc.direction_isInverse_paddingType >> 16) & 0x000000ff;
}
bool nbl_glsl_ext_FFT_getIsInverse() {
    return bool((pc.direction_isInverse_paddingType >> 8) & 0x000000ff);
}
uint nbl_glsl_ext_FFT_getPaddingType() {
    return (pc.direction_isInverse_paddingType) & 0x000000ff;
}

uint nbl_glsl_ext_FFT_getChannel()
{
    uint direction = nbl_glsl_ext_FFT_getDirection();
    return gl_WorkGroupID[direction];
}

uvec3 nbl_glsl_ext_FFT_getCoordinates(in uint tidx)
{
    uint direction = nbl_glsl_ext_FFT_getDirection();
    uvec3 tmp = gl_WorkGroupID;
    tmp[direction] = tidx;
    return tmp;
}

uvec3 nbl_glsl_ext_FFT_getBitReversedCoordinates(in uvec3 coords, in uint leadingZeroes)
{
    uint direction = nbl_glsl_ext_FFT_getDirection();
    uint bitReversedIndex = bitfieldReverse(coords[direction]) >> leadingZeroes;
    uvec3 tmp = coords;
    tmp[direction] = bitReversedIndex;
    return tmp;
}

uint nbl_glsl_ext_FFT_getDimLength(uvec3 dimension)
{
    uint direction = nbl_glsl_ext_FFT_getDirection();
    return dimension[direction];
}

void nbl_glsl_ext_FFT()
{
    // Virtual Threads Calculation
    uint dataLength = nbl_glsl_ext_FFT_getDimLength(pc.padded_dimension);
    uint num_virtual_threads = (dataLength-1u)/(_NBL_GLSL_EXT_FFT_BLOCK_SIZE_X_DEFINED_)+1u;
    uint thread_offset = gl_LocalInvocationIndex;

	uint channel = nbl_glsl_ext_FFT_getChannel();
    
    bool is_inverse = nbl_glsl_ext_FFT_getIsInverse();

	// Pass 0: Bit Reversal
	uint leadingZeroes = nbl_glsl_clz(dataLength) + 1u;
	uint logTwo = 32u - leadingZeroes;
	
    nbl_glsl_complex current_values[_NBL_GLSL_EXT_FFT_MAX_ITEMS_PER_THREAD];
    nbl_glsl_complex shuffled_values[_NBL_GLSL_EXT_FFT_MAX_ITEMS_PER_THREAD];

    // Load Initial Values into Local Mem (bit reversed indices)
    for(uint t = 0u; t < num_virtual_threads; t++)
    {
        uint tid = thread_offset + t * _NBL_GLSL_EXT_FFT_BLOCK_SIZE_X_DEFINED_;
        uvec3 coords = nbl_glsl_ext_FFT_getCoordinates(tid);
        uvec3 bitReversedCoords = nbl_glsl_ext_FFT_getBitReversedCoordinates(coords, leadingZeroes);

        current_values[t] = nbl_glsl_ext_FFT_getPaddedData(bitReversedCoords, channel);
    }

    // For loop for each stage of the FFT (each virtual thread computes 1 buttefly wing)
	for(uint i = 0u; i < logTwo; ++i) 
    {
		uint mask = 1u << i;

        // Data Exchange for virtual threads :
        // X and Y are seperate to use less shared memory for complex numbers
        // Get Shuffled Values X for virtual threads
        for(uint t = 0u; t < num_virtual_threads; t++)
        {
            uint tid = thread_offset + t * _NBL_GLSL_EXT_FFT_BLOCK_SIZE_X_DEFINED_;
            _NBL_GLSL_SCRATCH_SHARED_DEFINED_[tid] = floatBitsToUint(current_values[t].x);
        }
        barrier();
        memoryBarrierShared();
        for(uint t = 0u; t < num_virtual_threads; t++)
        {
            uint tid = thread_offset + t * _NBL_GLSL_EXT_FFT_BLOCK_SIZE_X_DEFINED_;
            shuffled_values[t].x = uintBitsToFloat(_NBL_GLSL_SCRATCH_SHARED_DEFINED_[tid ^ mask]);
        }

        // Get Shuffled Values Y for virtual threads
        for(uint t = 0u; t < num_virtual_threads; t++)
        {
            uint tid = thread_offset + t * _NBL_GLSL_EXT_FFT_BLOCK_SIZE_X_DEFINED_;
            _NBL_GLSL_SCRATCH_SHARED_DEFINED_[tid] = floatBitsToUint(current_values[t].y);
        }
        barrier();
        memoryBarrierShared();
        for(uint t = 0u; t < num_virtual_threads; t++)
        {
            uint tid = thread_offset + t * _NBL_GLSL_EXT_FFT_BLOCK_SIZE_X_DEFINED_;
            shuffled_values[t].y = uintBitsToFloat(_NBL_GLSL_SCRATCH_SHARED_DEFINED_[tid ^ mask]);
        }

        // Computation of each virtual thread
        for(uint t = 0u; t < num_virtual_threads; t++)
        {
            uint tid = thread_offset + t * _NBL_GLSL_EXT_FFT_BLOCK_SIZE_X_DEFINED_;
            nbl_glsl_complex shuffled_value = shuffled_values[t];

            nbl_glsl_complex twiddle = (is_inverse) 
             ? nbl_glsl_ext_FFT_twiddle(tid, i, logTwo)
             : nbl_glsl_ext_FFT_twiddleInverse(tid, i, logTwo);

            nbl_glsl_complex this_value = current_values[t];

            if(0u < uint(tid & mask)) {
                current_values[t] = shuffled_value + nbl_glsl_complex_mul(twiddle, this_value); 
            } else {
                current_values[t] = this_value + nbl_glsl_complex_mul(twiddle, shuffled_value); 
            }
        }
    }

    for(uint t = 0u; t < num_virtual_threads; t++)
    {
        uint tid = thread_offset + t * _NBL_GLSL_EXT_FFT_BLOCK_SIZE_X_DEFINED_;
	    uvec3 coords = nbl_glsl_ext_FFT_getCoordinates(tid);
        nbl_glsl_complex complex_value = (is_inverse) 
         ? current_values[t]
         : current_values[t] / dataLength;

	    nbl_glsl_ext_FFT_setData(coords, channel, complex_value);
    }
}

#endif