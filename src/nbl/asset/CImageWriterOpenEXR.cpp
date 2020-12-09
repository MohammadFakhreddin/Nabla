/*
MIT License
Copyright (c) 2019 AnastaZIuk
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include <algorithm>
#include <iostream>
#include <string>
#include <unordered_map>

#include "nbl/asset/filters/CRegionBlockFunctorFilter.h"

#include "CImageWriterOpenEXR.h"

#ifdef _NBL_COMPILE_WITH_OPENEXR_WRITER_

#include "nbl/asset/COpenEXRImageMetadata.h"

#include "openexr/IlmBase/Imath/ImathBox.h"
#include "openexr/OpenEXR/IlmImf/ImfOutputFile.h"
#include "openexr/OpenEXR/IlmImf/ImfChannelList.h"
#include "openexr/OpenEXR/IlmImf/ImfChannelListAttribute.h"
#include "openexr/OpenEXR/IlmImf/ImfStringAttribute.h"
#include "openexr/OpenEXR/IlmImf/ImfMatrixAttribute.h"
#include "openexr/OpenEXR/IlmImf/ImfArray.h"

#include "openexr/OpenEXR/IlmImf/ImfNamespace.h"
namespace IMF = Imf;
namespace IMATH = Imath;

namespace nbl
{
namespace asset
{
	using namespace IMF;
	using namespace IMATH;

	constexpr uint8_t availableChannels = 4;

	template<typename ilmType>
	bool createAndWriteImage(std::array<ilmType*, availableChannels>& pixelsArrayIlm, const asset::ICPUImage* image, const char* fileName)
	{
		const auto& creationParams = image->getCreationParameters();
		auto getIlmType = [&creationParams]()
		{
			if (creationParams.format == EF_R16G16B16A16_SFLOAT)
				return PixelType::HALF;
			else if (creationParams.format == EF_R32G32B32A32_SFLOAT)
				return PixelType::FLOAT;
			else if (creationParams.format == EF_R32G32B32A32_UINT)
				return PixelType::UINT;
			else
				return PixelType::NUM_PIXELTYPES;
		};

		const auto width = creationParams.extent.width;
		const auto height = creationParams.extent.height;
		Header header(width, height);
		const PixelType pixelType = getIlmType();
		FrameBuffer frameBuffer;

		if (pixelType == PixelType::NUM_PIXELTYPES || creationParams.type != IImage::E_TYPE::ET_2D)
			return false;

		for (auto& channelPixelsPtr : pixelsArrayIlm)
			channelPixelsPtr = _NBL_NEW_ARRAY(ilmType, width * height);

		const auto* data = reinterpret_cast<const uint8_t*>(image->getBuffer()->getPointer());
		auto writeTexel = [&creationParams,&data,&pixelsArrayIlm](uint32_t ptrOffset, const core::vectorSIMDu32& texelCoord) -> void
		{
			assert(texelCoord.w==0u && texelCoord.z==0u);

			const uint8_t* texelPtr = data+ptrOffset;
			const uint64_t ptrStyleIlmShiftToDataChannelPixel = (texelCoord.y*creationParams.extent.width)+texelCoord.x;

			for (uint8_t channelIndex=0; channelIndex<availableChannels; ++channelIndex)
			{
				ilmType channelPixel = *(reinterpret_cast<const ilmType*>(texelPtr) + channelIndex);
				*(pixelsArrayIlm[channelIndex] + ptrStyleIlmShiftToDataChannelPixel) = channelPixel;
			}
		};

		using StreamToEXR = CRegionBlockFunctorFilter<decltype(writeTexel),true>;
		StreamToEXR::state_type state(writeTexel,image,nullptr);
		for (auto rit=image->getRegions().begin(); rit!=image->getRegions().end(); rit++)
		{
			if (rit->imageSubresource.mipLevel || rit->imageSubresource.baseArrayLayer)
				continue;

			state.regionIterator = rit;
			StreamToEXR::execute(&state);
		}

		constexpr std::array<const char*, availableChannels> rgbaSignatureAsText = { "R", "G", "B", "A" };
		for (uint8_t channel = 0; channel < rgbaSignatureAsText.size(); ++channel)
		{
			auto rowPitch = sizeof(*pixelsArrayIlm[channel]) * width;

			header.channels().insert(rgbaSignatureAsText[channel], Channel(pixelType));
			frameBuffer.insert
			(
				rgbaSignatureAsText[channel],                                                                // name
				Slice(pixelType,                                                                             // type
				(char*) pixelsArrayIlm[channel],                                                             // base
				sizeof(*pixelsArrayIlm[channel]) * 1,                                                        // xStride
				rowPitch)																					 // yStride
			);
		}

		OutputFile file(fileName, header);
		file.setFrameBuffer(frameBuffer);
		file.writePixels(height);

		for (auto channelPixelsPtr : pixelsArrayIlm)
			_NBL_DELETE_ARRAY(channelPixelsPtr, width * height);
	}

	bool CImageWriterOpenEXR::writeAsset(io::IWriteFile* _file, const SAssetWriteParams& _params, IAssetWriterOverride* _override)
	{
		if (!_override)
			getDefaultOverride(_override);

		SAssetWriteContext ctx{ _params, _file };

		auto imageSmart = asset::IImageAssetHandlerBase::createImageDataForCommonWriting(IAsset::castDown<const ICPUImageView>(_params.rootAsset));
		const asset::ICPUImage* image = imageSmart.get();

		if (image->getBuffer()->isADummyObjectForCache())
			return false;

		io::IWriteFile* file = _override->getOutputFile(_file, ctx, { image, 0u });

		if (!file)
			return false;

		return writeImageBinary(file, image);
	}

	bool CImageWriterOpenEXR::writeImageBinary(io::IWriteFile* file, const asset::ICPUImage* image)
	{
		const auto& params = image->getCreationParameters();
			
		std::array<half*, availableChannels> halfPixelMapArray = {nullptr, nullptr, nullptr, nullptr};
		std::array<float*, availableChannels> fullFloatPixelMapArray = { nullptr, nullptr, nullptr, nullptr };
		std::array<uint32_t*, availableChannels> uint32_tPixelMapArray = { nullptr, nullptr, nullptr, nullptr };

		if (params.format == EF_R16G16B16A16_SFLOAT)
			createAndWriteImage(halfPixelMapArray, image, file->getFileName().c_str());
		else if (params.format == EF_R32G32B32A32_SFLOAT)
			createAndWriteImage(fullFloatPixelMapArray, image, file->getFileName().c_str());
		else if (params.format == EF_R32G32B32A32_UINT)
			createAndWriteImage(uint32_tPixelMapArray, image, file->getFileName().c_str());

		return true;
	}
}
}

#endif // _NBL_COMPILE_WITH_OPENEXR_WRITER_