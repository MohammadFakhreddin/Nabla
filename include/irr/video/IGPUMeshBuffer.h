// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_GPU_MESH_BUFFER_H_INCLUDED__
#define __I_GPU_MESH_BUFFER_H_INCLUDED__

#include <algorithm>

#include "irr/asset/asset.h"
#include "IGPUBuffer.h"
#include "IGPUDescriptorSet.h"
#include "IGPURenderpassIndependentPipeline.h"

namespace irr
{
namespace video
{
	class IGPUMeshBuffer final : public asset::IMeshBuffer<IGPUBuffer,IGPUDescriptorSet,IGPURenderpassIndependentPipeline>
	{
        using base_t = asset::IMeshBuffer<IGPUBuffer, IGPUDescriptorSet, IGPURenderpassIndependentPipeline>;

    public:
        using base_t::base_t;
	};

} // end namespace video
} // end namespace irr



#endif


