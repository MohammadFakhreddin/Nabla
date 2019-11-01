#include "IrrCompileConfig.h"

#ifdef _IRR_COMPILE_WITH_OPENGL_

#include "irr/core/Types.h"
#include "COpenGL2DTexture.h"
#include "COpenGLDriver.h"




namespace irr
{
namespace video
{

/**
    if (mipmapData)
    {
        uint8_t* tmpMipmapDataPTr = (uint8_t*)mipmapData;
        for (uint32_t i=1; i<MipLevelsStored; i++)
        {
            core::dimension2d<uint32_t> tmpSize = size;
            tmpSize.Width = core::max(tmpSize.Width/(0x1u<<i),0x1u);
            tmpSize.Height = core::max(tmpSize.Height/(0x1u<<i),0x1u);
            size_t levelByteSize;
            if (compressed)
            {
                levelByteSize = (((tmpSize.Width+3)&0xfffffc)*((tmpSize.Height+3)&0xfffffc)*bpp)/8;
                COpenGLExtensionHandler::extGlCompressedTextureSubImage2D(TextureName,GL_TEXTURE_2D,i,0,0, tmpSize.Width,tmpSize.Height, InternalFormat,levelByteSize,tmpMipmapDataPTr);
            }
            else
            {
                levelByteSize = (tmpSize.Width*tmpSize.Height*bpp)/8;
                COpenGLExtensionHandler::setPixelUnpackAlignment((tmpSize.Width*bpp)/8,tmpMipmapDataPTr);
                COpenGLExtensionHandler::extGlTextureSubImage2D(TextureName,GL_TEXTURE_2D,i,0,0, tmpSize.Width,tmpSize.Height, inDataFmt, inDataTpe, (void*)tmpMipmapDataPTr);
            }
            tmpMipmapDataPTr += levelByteSize;
        }
    }
**/

bool COpenGL2DTexture::updateSubRegion(const asset::E_FORMAT &inDataColorFormat, const void* data, const uint32_t* minimum, const uint32_t* maximum, int32_t mipmap, const uint32_t& unpackRowByteAlignment)
{
    bool sourceCompressed = isBlockCompressionFormat(inDataColorFormat);

    bool destinationCompressed = COpenGLTexture::isInternalFormatCompressed(InternalFormat);
    if ((!destinationCompressed)&&sourceCompressed)
        return false;

    if (destinationCompressed)
    {
        if (minimum[0]||minimum[1])
            return false;

        uint32_t adjustedTexSize[2] = {TextureSize[0],TextureSize[1]};
        adjustedTexSize[0] /= 0x1u<<mipmap;
        adjustedTexSize[1] /= 0x1u<<mipmap;
        /*
        adjustedTexSize[0] += 3u;
        adjustedTexSize[1] += 3u;
        adjustedTexSize[0] &= 0xfffffc;
        adjustedTexSize[1] &= 0xfffffc;
        */
        if (maximum[0]!=adjustedTexSize[0]||maximum[1]!=adjustedTexSize[1])
            return false;
    }

    if (sourceCompressed)
    {
		// should really use blockk size querying functions to round up properly and not assume 4x4
        size_t levelByteSize = (((maximum[0]-minimum[0]+3)&0xfffffc)*((maximum[1]-minimum[1]+3)&0xfffffc)*asset::getBytesPerPixel(ColorFormat)).getIntegerApprox();

        COpenGLExtensionHandler::extGlCompressedTextureSubImage2D(TextureName,GL_TEXTURE_2D, mipmap, minimum[0],minimum[1],maximum[0]-minimum[0],maximum[1]-minimum[1], InternalFormat, levelByteSize, data);
    }
    else
    {
        GLenum pixFmt,pixType;
        getOpenGLFormatAndParametersFromColorFormat(inDataColorFormat, pixFmt, pixType);
        //! replace with
        ///COpenGLExtensionHandler::extGlGetInternalFormativ(GL_TEXTURE_2D,InternalFormat,GL_TEXTURE_IMAGE_FORMAT,1,&pixFmt);
        ///COpenGLExtensionHandler::extGlGetInternalFormativ(GL_TEXTURE_2D,InternalFormat,GL_TEXTURE_IMAGE_FORMAT,1,&pixType);

        //! we're going to have problems with uploading lower mip levels ?
        uint32_t pitchInBits = ((maximum[0]-minimum[0])*asset::getBytesPerPixel(inDataColorFormat)).getIntegerApprox();

        COpenGLExtensionHandler::setPixelUnpackAlignment(pitchInBits,const_cast<void*>(data),unpackRowByteAlignment);
        COpenGLExtensionHandler::extGlTextureSubImage2D(TextureName, GL_TEXTURE_2D, mipmap, minimum[0],minimum[1], maximum[0]-minimum[0],maximum[1]-minimum[1], pixFmt, pixType, data);
    }
    return true;
}

bool COpenGL2DTexture::resize(const uint32_t* size, const uint32_t &mipLevels)
{
    if (TextureSize[0]==size[0]&&TextureSize[1]==size[1])
        return true;

    recreateName(getOpenGLTextureType());

    uint32_t defaultMipMapCount = 1u+uint32_t(floorf(log2(float(core::max(size[0],size[1])))));
    if (MipLevelsStored>1)
    {
        if (mipLevels==0)
            MipLevelsStored = defaultMipMapCount;
        else
            MipLevelsStored = core::min(mipLevels,defaultMipMapCount);
    }

    TextureSize[0] = size[0];
    TextureSize[1] = size[1];
	COpenGLExtensionHandler::extGlTextureStorage2D(TextureName,getOpenGLTextureType(), MipLevelsStored, InternalFormat, TextureSize[0], TextureSize[1]);
	return true;
}

}
}
#endif

#if 0


#include "irr/core/Types.h"
#include "os.h"


		//! constructor for basic setup (only for derived classes)
		COpenGLTexture::COpenGLTexture(const GLenum& textureType_Target)
			: TextureName(0), TextureNameHasChanged(0)
		{
		}



		void COpenGLTexture::recreateName(const GLenum& textureType_Target)
		{
			if (TextureName)
				glDeleteTextures(1, &TextureName);
			COpenGLExtensionHandler::extGlCreateTextures(textureType_Target, 1, &TextureName);
			TextureNameHasChanged = CNullDriver::incrementAndFetchReallocCounter();
		}

		//! Get opengl values for the GPU texture storage
		void COpenGLTexture::getOpenGLFormatAndParametersFromColorFormat(const asset::E_FORMAT& format,
			GLenum& colorformat,
			GLenum& type)
		{
			using namespace asset;
			// default
			colorformat = GL_INVALID_ENUM;
			type = GL_INVALID_ENUM;

			switch (format)
			{
			case asset::EF_A1R5G5B5_UNORM_PACK16:
				colorformat = GL_BGRA_EXT;
				type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
				break;
			case asset::EF_R5G6B5_UNORM_PACK16:
				colorformat = GL_RGB;
				type = GL_UNSIGNED_SHORT_5_6_5;
				break;
				// Floating Point texture formats. Thanks to Patryk "Nadro" Nadrowski.
			case asset::EF_B10G11R11_UFLOAT_PACK32:
			{
				colorformat = GL_RGB;
				type = GL_R11F_G11F_B10F;
			}
			break;
			case asset::EF_R16_SFLOAT:
			{
				colorformat = GL_RED;
				type = GL_HALF_FLOAT;
			}
			break;
			case asset::EF_R16G16_SFLOAT:
			{
				colorformat = GL_RG;
				type = GL_HALF_FLOAT;
			}
			break;
			case asset::EF_R16G16B16A16_SFLOAT:
			{
				colorformat = GL_RGBA;
				type = GL_HALF_FLOAT;
			}
			break;
			case asset::EF_R32_SFLOAT:
			{
				colorformat = GL_RED;
				type = GL_FLOAT;
			}
			break;
			case asset::EF_R32G32_SFLOAT:
			{
				colorformat = GL_RG;
				type = GL_FLOAT;
			}
			break;
			case asset::EF_R32G32B32A32_SFLOAT:
			{
				colorformat = GL_RGBA;
				type = GL_FLOAT;
			}
			break;
			case asset::EF_R8_SNORM:
			{
				colorformat = GL_RED;
				type = GL_BYTE;
			}
			break;
			case asset::EF_R8_UNORM:
			{
				colorformat = GL_RED;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case asset::EF_R8_SRGB:
			{
				if (!COpenGLExtensionHandler::FeatureAvailable[COpenGLExtensionHandler::IRR_EXT_texture_sRGB_R8])
					break;
				colorformat = GL_RED;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case asset::EF_R8G8_SNORM:
			{
				colorformat = GL_RG;
				type = GL_BYTE;
			}
			break;
			case asset::EF_R8G8_UNORM:
			{
				colorformat = GL_RG;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case asset::EF_R8G8_SRGB:
			{
				if (!COpenGLExtensionHandler::FeatureAvailable[COpenGLExtensionHandler::IRR_EXT_texture_sRGB_RG8])
					break;
				colorformat = GL_RG;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case asset::EF_R8G8B8_SNORM:
				colorformat = GL_RGB;
				type = GL_BYTE;
				break;
			case asset::EF_R8G8B8_UNORM:
				colorformat = GL_RGB;
				type = GL_UNSIGNED_BYTE;
				break;
			case asset::EF_B8G8R8A8_SNORM:
				colorformat = GL_BGRA_EXT;
				type = GL_BYTE;
				break;
			case asset::EF_B8G8R8A8_UNORM:
				colorformat = GL_BGRA_EXT;
				type = GL_UNSIGNED_INT_8_8_8_8_REV;
				break;
			case asset::EF_B8G8R8A8_SRGB:
				colorformat = GL_BGRA_EXT;
				type = GL_UNSIGNED_INT_8_8_8_8_REV;
				break;
			case asset::EF_R8G8B8A8_SNORM:
				colorformat = GL_RGBA;
				type = GL_BYTE;
				break;
			case asset::EF_R8G8B8A8_UNORM:
				colorformat = GL_RGBA;
				type = GL_UNSIGNED_BYTE;
				break;
			case asset::EF_R8_UINT:
			{
				colorformat = GL_RED_INTEGER;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case asset::EF_R8G8_UINT:
			{
				colorformat = GL_RG_INTEGER;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case asset::EF_R8G8B8_UINT:
			{
				colorformat = GL_RGB_INTEGER;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case asset::EF_R8G8B8A8_UINT:
			{
				colorformat = GL_RGBA_INTEGER;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case asset::EF_R8_SINT:
			{
				colorformat = GL_RED_INTEGER;
				type = GL_BYTE;
			}
			break;
			case asset::EF_R8G8_SINT:
			{
				colorformat = GL_RG_INTEGER;
				type = GL_BYTE;
			}
			break;
			case asset::EF_R8G8B8_SINT:
			{
				colorformat = GL_RGB_INTEGER;
				type = GL_BYTE;
			}
			break;
			case asset::EF_R8G8B8_SRGB:
			{
				colorformat = GL_RGB;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case asset::EF_R8G8B8A8_SRGB:
			{
				colorformat = GL_RGBA;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case asset::EF_R8G8B8A8_SINT:
			{
				colorformat = GL_RGBA_INTEGER;
				type = GL_BYTE;
			}
			break;
			case asset::EF_R16_SNORM:
			{
				colorformat = GL_RED;
				type = GL_SHORT;
			}
			break;
			case asset::EF_R16_UNORM:
			{
				colorformat = GL_RED;
				type = GL_UNSIGNED_SHORT;
			}
			break;
			case asset::EF_R16G16_SNORM:
			{
				colorformat = GL_RG;
				type = GL_SHORT;
			}
			break;
			case asset::EF_R16G16_UNORM:
			{
				colorformat = GL_RG;
				type = GL_UNSIGNED_SHORT;
			}
			break;
			case asset::EF_R16G16B16_SNORM:
			{
				colorformat = GL_RGB;
				type = GL_SHORT;
			}
			break;
			case asset::EF_R16G16B16_UNORM:
			{
				colorformat = GL_RGB;
				type = GL_UNSIGNED_SHORT;
			}
			break;
			case asset::EF_R16G16B16A16_SNORM:
			{
				colorformat = GL_RGBA;
				type = GL_SHORT;
			}
			break;
			case asset::EF_R16G16B16A16_UNORM:
			{
				colorformat = GL_RGBA;
				type = GL_UNSIGNED_SHORT;
			}
			break;
			case asset::EF_R16_UINT:
			{
				colorformat = GL_RED_INTEGER;
				type = GL_UNSIGNED_SHORT;
			}
			break;
			case asset::EF_R16G16_UINT:
			{
				colorformat = GL_RG_INTEGER;
				type = GL_UNSIGNED_SHORT;
			}
			break;
			case asset::EF_R16G16B16_UINT:
			{
				colorformat = GL_RGB_INTEGER;
				type = GL_UNSIGNED_SHORT;
			}
			break;
			case asset::EF_R16G16B16A16_UINT:
			{
				colorformat = GL_RGBA_INTEGER;
				type = GL_UNSIGNED_SHORT;
			}
			break;
			case asset::EF_R16_SINT:
			{
				colorformat = GL_RED_INTEGER;
				type = GL_SHORT;
			}
			break;
			case asset::EF_R16G16_SINT:
			{
				colorformat = GL_RG_INTEGER;
				type = GL_SHORT;
			}
			break;
			case asset::EF_R16G16B16_SINT:
			{
				colorformat = GL_RGB_INTEGER;
				type = GL_SHORT;
			}
			break;
			case asset::EF_R16G16B16A16_SINT:
			{
				colorformat = GL_RGBA_INTEGER;
				type = GL_SHORT;
			}
			break;
			case asset::EF_R32_UINT:
			{
				colorformat = GL_RED_INTEGER;
				type = GL_UNSIGNED_INT;
			}
			break;
			case asset::EF_R32G32_UINT:
			{
				colorformat = GL_RG_INTEGER;
				type = GL_UNSIGNED_INT;
			}
			break;
			case asset::EF_R32G32B32_UINT:
			{
				colorformat = GL_RGB_INTEGER;
				type = GL_UNSIGNED_INT;
			}
			break;
			case asset::EF_R32G32B32A32_UINT:
			{
				colorformat = GL_RGBA_INTEGER;
				type = GL_UNSIGNED_INT;
			}
			break;
			case asset::EF_R32_SINT:
			{
				colorformat = GL_RED_INTEGER;
				type = GL_INT;
			}
			break;
			case asset::EF_R32G32_SINT:
			{
				colorformat = GL_RG_INTEGER;
				type = GL_INT;
			}
			break;
			case asset::EF_R32G32B32_SINT:
			{
				colorformat = GL_RGB_INTEGER;
				type = GL_INT;
			}
			break;
			case asset::EF_R32G32B32A32_SINT:
			{
				colorformat = GL_RGBA_INTEGER;
				type = GL_INT;
			}
			break;
			case asset::EF_BC1_RGB_UNORM_BLOCK:
			{
				colorformat = GL_RGB;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case asset::EF_BC1_RGBA_UNORM_BLOCK:
			{
				colorformat = GL_RGBA;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case asset::EF_BC2_UNORM_BLOCK:
			{
				colorformat = GL_RGBA;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case asset::EF_BC3_UNORM_BLOCK:
			{
				colorformat = GL_RGBA;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case asset::EF_BC1_RGB_SRGB_BLOCK:
			{
				colorformat = GL_RGBA;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case asset::EF_BC1_RGBA_SRGB_BLOCK:
			{
				colorformat = GL_RGBA;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case asset::EF_BC2_SRGB_BLOCK:
			{
				colorformat = GL_RGBA;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case asset::EF_BC3_SRGB_BLOCK:
			{
				colorformat = GL_RGBA;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case asset::EF_BC7_UNORM_BLOCK:
			{
				colorformat = GL_RGBA;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case asset::EF_BC7_SRGB_BLOCK:
			{
				colorformat = GL_RGBA;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case asset::EF_BC6H_SFLOAT_BLOCK:
			{
				colorformat = GL_RGB;
				type = GL_HALF_FLOAT;
			}
			break;
			case asset::EF_BC6H_UFLOAT_BLOCK:
			{
				colorformat = GL_RGB;
				type = GL_HALF_FLOAT;
			}
			break;
			case asset::EF_ETC2_R8G8B8_UNORM_BLOCK:
			{
				colorformat = GL_RGB;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case asset::EF_ETC2_R8G8B8_SRGB_BLOCK:
			{
				colorformat = GL_RGB;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case asset::EF_ETC2_R8G8B8A1_UNORM_BLOCK:
			{
				colorformat = GL_RGBA;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case asset::EF_ETC2_R8G8B8A1_SRGB_BLOCK:
			{
				colorformat = GL_RGBA;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case asset::EF_ETC2_R8G8B8A8_UNORM_BLOCK:
			{
				colorformat = GL_RGBA;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case asset::EF_ETC2_R8G8B8A8_SRGB_BLOCK:
			{
				colorformat = GL_RGBA;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case asset::EF_EAC_R11G11_UNORM_BLOCK:
			{
				colorformat = GL_RG;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case asset::EF_EAC_R11G11_SNORM_BLOCK:
			{
				colorformat = GL_RG;
				type = GL_BYTE;
			}
			break;
			case asset::EF_EAC_R11_UNORM_BLOCK:
			{
				colorformat = GL_RED;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case asset::EF_EAC_R11_SNORM_BLOCK:
			{
				colorformat = GL_RED;
				type = GL_BYTE;
			}
			break;
			case EF_ASTC_4x4_UNORM_BLOCK:
			{
				colorformat = GL_RGBA;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case EF_ASTC_5x4_UNORM_BLOCK:
			{
				colorformat = GL_RGBA;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case EF_ASTC_5x5_UNORM_BLOCK:
			{
				colorformat = GL_RGBA;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case EF_ASTC_6x5_UNORM_BLOCK:
			{
				colorformat = GL_RGBA;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case EF_ASTC_6x6_UNORM_BLOCK:
			{
				colorformat = GL_RGBA;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case EF_ASTC_8x5_UNORM_BLOCK:
			{
				colorformat = GL_RGBA;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case EF_ASTC_8x6_UNORM_BLOCK:
			{
				colorformat = GL_RGBA;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case EF_ASTC_8x8_UNORM_BLOCK:
			{
				colorformat = GL_RGBA;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case EF_ASTC_10x5_UNORM_BLOCK:
			{
				colorformat = GL_RGBA;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case EF_ASTC_10x6_UNORM_BLOCK:
			{
				colorformat = GL_RGBA;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case EF_ASTC_10x8_UNORM_BLOCK:
			{
				colorformat = GL_RGBA;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case EF_ASTC_10x10_UNORM_BLOCK:
			{
				colorformat = GL_RGBA;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case EF_ASTC_12x10_UNORM_BLOCK:
			{
				colorformat = GL_RGBA;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case EF_ASTC_12x12_UNORM_BLOCK:
			{
				colorformat = GL_RGBA;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case EF_ASTC_4x4_SRGB_BLOCK:
			{
				colorformat = GL_RGBA;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case EF_ASTC_5x4_SRGB_BLOCK:
			{
				colorformat = GL_RGBA;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case EF_ASTC_5x5_SRGB_BLOCK:
			{
				colorformat = GL_RGBA;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case EF_ASTC_6x5_SRGB_BLOCK:
			{
				colorformat = GL_RGBA;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case EF_ASTC_6x6_SRGB_BLOCK:
			{
				colorformat = GL_RGBA;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case EF_ASTC_8x5_SRGB_BLOCK:
			{
				colorformat = GL_RGBA;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case EF_ASTC_8x6_SRGB_BLOCK:
			{
				colorformat = GL_RGBA;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case EF_ASTC_8x8_SRGB_BLOCK:
			{
				colorformat = GL_RGBA;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case EF_ASTC_10x5_SRGB_BLOCK:
			{
				colorformat = GL_RGBA;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case EF_ASTC_10x6_SRGB_BLOCK:
			{
				colorformat = GL_RGBA;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case EF_ASTC_10x8_SRGB_BLOCK:
			{
				colorformat = GL_RGBA;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case EF_ASTC_10x10_SRGB_BLOCK:
			{
				colorformat = GL_RGBA;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case EF_ASTC_12x10_SRGB_BLOCK:
			{
				colorformat = GL_RGBA;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case EF_ASTC_12x12_SRGB_BLOCK:
			{
				colorformat = GL_RGBA;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			case asset::EF_E5B9G9R9_UFLOAT_PACK32:
			{
				colorformat = GL_RGB;
				type = GL_UNSIGNED_INT_5_9_9_9_REV;
			}
			break;
			/// this is totally wrong but safe - most probs have to reupload
			case asset::EF_D16_UNORM:
			{
				colorformat = GL_DEPTH;
				type = GL_UNSIGNED_SHORT;
			}
			break;
			case asset::EF_X8_D24_UNORM_PACK32:
			{
				colorformat = GL_DEPTH;
				type = GL_UNSIGNED_SHORT;
			}
			break;
			case asset::EF_D24_UNORM_S8_UINT:
			{
				colorformat = GL_DEPTH_STENCIL;
				type = GL_UNSIGNED_INT_24_8;
			}
			break;
			case asset::EF_D32_SFLOAT:
			{
				colorformat = GL_DEPTH;
				type = GL_FLOAT;
			}
			break;
			case asset::EF_D32_SFLOAT_S8_UINT:
			{
				colorformat = GL_DEPTH_STENCIL;
				type = GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
			}
			break;
			case asset::EF_S8_UINT:
			{
				colorformat = GL_STENCIL;
				type = GL_UNSIGNED_BYTE;
			}
			break;
			default:
				break;
			}

			if (colorformat == GL_INVALID_ENUM || type == GL_INVALID_ENUM)
				os::Printer::log("Unsupported upload format", ELL_ERROR);
		}


		asset::E_FORMAT COpenGLTexture::getColorFormatFromSizedOpenGLFormat(const GLenum& sizedFormat)
		{
			using namespace asset;
			switch (sizedFormat)
			{
			case GL_COMPRESSED_RGBA_ASTC_4x4_KHR:
				return EF_ASTC_4x4_UNORM_BLOCK;
			case GL_COMPRESSED_RGBA_ASTC_5x4_KHR:
				return EF_ASTC_5x4_UNORM_BLOCK;
			case GL_COMPRESSED_RGBA_ASTC_5x5_KHR:
				return EF_ASTC_5x5_UNORM_BLOCK;
			case GL_COMPRESSED_RGBA_ASTC_6x5_KHR:
				return EF_ASTC_6x5_UNORM_BLOCK;
			case GL_COMPRESSED_RGBA_ASTC_6x6_KHR:
				return EF_ASTC_6x6_UNORM_BLOCK;
			case GL_COMPRESSED_RGBA_ASTC_8x5_KHR:
				return EF_ASTC_8x5_UNORM_BLOCK;
			case GL_COMPRESSED_RGBA_ASTC_8x6_KHR:
				return EF_ASTC_8x6_UNORM_BLOCK;
			case GL_COMPRESSED_RGBA_ASTC_8x8_KHR:
				return EF_ASTC_8x8_UNORM_BLOCK;
			case GL_COMPRESSED_RGBA_ASTC_10x5_KHR:
				return EF_ASTC_10x5_UNORM_BLOCK;
			case GL_COMPRESSED_RGBA_ASTC_10x6_KHR:
				return EF_ASTC_10x6_UNORM_BLOCK;
			case GL_COMPRESSED_RGBA_ASTC_10x8_KHR:
				return EF_ASTC_10x8_UNORM_BLOCK;
			case GL_COMPRESSED_RGBA_ASTC_10x10_KHR:
				return EF_ASTC_10x10_UNORM_BLOCK;
			case GL_COMPRESSED_RGBA_ASTC_12x10_KHR:
				return EF_ASTC_12x10_UNORM_BLOCK;
			case GL_COMPRESSED_RGBA_ASTC_12x12_KHR:
				return EF_ASTC_12x12_UNORM_BLOCK;

			case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR:
				return EF_ASTC_4x4_SRGB_BLOCK;
			case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR:
				return EF_ASTC_5x4_SRGB_BLOCK;
			case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR:
				return EF_ASTC_5x5_SRGB_BLOCK;
			case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR:
				return EF_ASTC_6x5_SRGB_BLOCK;
			case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR:
				return EF_ASTC_6x6_SRGB_BLOCK;
			case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR:
				return EF_ASTC_8x5_SRGB_BLOCK;
			case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR:
				return EF_ASTC_8x6_SRGB_BLOCK;
			case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR:
				return EF_ASTC_8x8_SRGB_BLOCK;
			case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR:
				return EF_ASTC_10x5_SRGB_BLOCK;
			case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR:
				return EF_ASTC_10x6_SRGB_BLOCK;
			case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR:
				return EF_ASTC_10x8_SRGB_BLOCK;
			case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR:
				return EF_ASTC_10x10_SRGB_BLOCK;
			case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR:
				return EF_ASTC_12x10_SRGB_BLOCK;
			case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR:
				return EF_ASTC_12x12_SRGB_BLOCK;

				/*case asset::EF_BC1_RGB_SRGB_BLOCK:
					return GL_COMPRESSED_SRGB_S3TC_DXT1_EXT;
					break;
				case asset::EF_BC1_RGBA_SRGB_BLOCK:
					return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;
					break;
				case asset::EF_BC2_SRGB_BLOCK:
					return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT;
					break;
				case asset::EF_BC3_SRGB_BLOCK:
					return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
					break;*/
			case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
				return asset::EF_BC1_RGB_SRGB_BLOCK;
			case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
				return asset::EF_BC1_RGBA_SRGB_BLOCK;
			case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
				return asset::EF_BC2_SRGB_BLOCK;
			case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
				return asset::EF_BC3_SRGB_BLOCK;
			case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
				return asset::EF_BC1_RGB_UNORM_BLOCK;
				break;
			case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
				return asset::EF_BC1_RGBA_UNORM_BLOCK;
				break;
			case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
				return asset::EF_BC2_UNORM_BLOCK;
				break;
			case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
				return asset::EF_BC3_UNORM_BLOCK;
				break;
			case GL_COMPRESSED_RGBA_BPTC_UNORM:
				return asset::EF_BC7_UNORM_BLOCK;
			case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM:
				return asset::EF_BC7_SRGB_BLOCK;
			case GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT:
				return asset::EF_BC6H_SFLOAT_BLOCK;
			case GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT:
				return asset::EF_BC6H_UFLOAT_BLOCK;
			case GL_COMPRESSED_RGB8_ETC2:
				return asset::EF_ETC2_R8G8B8_UNORM_BLOCK;
			case GL_COMPRESSED_SRGB8_ETC2:
				return asset::EF_ETC2_R8G8B8_SRGB_BLOCK;
			case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:
				return asset::EF_ETC2_R8G8B8A1_UNORM_BLOCK;
			case GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:
				return asset::EF_ETC2_R8G8B8A1_SRGB_BLOCK;
			case GL_COMPRESSED_RGBA8_ETC2_EAC:
				return asset::EF_ETC2_R8G8B8A8_UNORM_BLOCK;
			case GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC:
				return asset::EF_ETC2_R8G8B8A8_SRGB_BLOCK;
			case GL_COMPRESSED_RG11_EAC:
				return asset::EF_EAC_R11G11_UNORM_BLOCK;
			case GL_COMPRESSED_SIGNED_RG11_EAC:
				return asset::EF_EAC_R11G11_SNORM_BLOCK;
			case GL_COMPRESSED_R11_EAC:
				return asset::EF_EAC_R11_UNORM_BLOCK;
			case GL_COMPRESSED_SIGNED_R11_EAC:
				return asset::EF_EAC_R11_SNORM_BLOCK;
			case GL_STENCIL_INDEX8:
				return asset::EF_S8_UINT;
				break;
			case GL_RGBA2:
				///return asset::EF_8BIT_PIX;
				break;
			case GL_R3_G3_B2:
				///return asset::EF_8BIT_PIX;
				break;
			case GL_R8:
				return asset::EF_R8_UNORM;
				break;
			case GL_R8I:
				return asset::EF_R8_SINT;
				break;
			case GL_R8UI:
				return asset::EF_R8_UINT;
				break;
			case GL_R8_SNORM:
				return asset::EF_R8_SNORM;
				break;
			case GL_RGB4:
				///return asset::EF_16BIT_PIX;
				break;
			case GL_RGB5:
				///return asset::EF_;
				break;
			case GL_DEPTH_COMPONENT16:
				return asset::EF_D16_UNORM;
				break;
			case GL_RGBA4:
				return asset::EF_R4G4B4A4_UNORM_PACK16;
				break;
			case GL_RGB5_A1:
				return asset::EF_R5G5B5A1_UNORM_PACK16;
				break;
			case GL_RG8:
				return asset::EF_R8G8_UNORM;
				break;
			case GL_SR8_EXT:
				return asset::EF_R8_SRGB;
				break;
			case GL_RG8I:
				return asset::EF_R8G8_SINT;
				break;
			case GL_RG8UI:
				return asset::EF_R8G8_UINT;
				break;
			case GL_RG8_SNORM:
				return asset::EF_R8G8_SNORM;
				break;
			case GL_R16:
				return asset::EF_R16_UNORM;
				break;
			case GL_R16I:
				return asset::EF_R16_SINT;
				break;
			case GL_R16UI:
				return asset::EF_R16_UINT;
				break;
			case GL_R16_SNORM:
				return asset::EF_R16_SNORM;
				break;
			case GL_R16F:
				return asset::EF_R16_SFLOAT;
				break;
			case GL_DEPTH_COMPONENT24:
				return asset::EF_X8_D24_UNORM_PACK32;
				break;
			case GL_RGB8:
				return asset::EF_R8G8B8_UNORM;
				break;
			case GL_SRG8_EXT:
				return asset::EF_R8G8_SRGB;
				break;
			case GL_RGB8I:
				return asset::EF_R8G8B8_SINT;
				break;
			case GL_RGB8UI:
				return asset::EF_R8G8B8_UINT;
				break;
			case GL_RGB8_SNORM:
				return asset::EF_R8G8B8_SNORM;
				break;
			case GL_SRGB8:
				return asset::EF_R8G8B8_SRGB;
				break;
			case GL_RGB10:
				///return asset::EF_;
				break;
			case GL_DEPTH24_STENCIL8:
				return asset::EF_D24_UNORM_S8_UINT;
				break;
			case GL_DEPTH_COMPONENT32:
				///return asset::EF_DEPTH32;
				break;
			case GL_DEPTH_COMPONENT32F:
				return asset::EF_D32_SFLOAT;
				break;
			case GL_RGBA8:
				return asset::EF_R8G8B8A8_UNORM;
				break;
			case GL_RGBA8I:
				return asset::EF_R8G8B8A8_SINT;
				break;
			case GL_RGBA8UI:
				return asset::EF_R8G8B8A8_UINT;
				break;
			case GL_RGBA8_SNORM:
				return asset::EF_R8G8B8A8_SNORM;
				break;
			case GL_SRGB8_ALPHA8:
				return asset::EF_R8G8B8A8_SRGB;
				break;
			case GL_RGB10_A2:
				return asset::EF_A2B10G10R10_UNORM_PACK32;
				break;
			case GL_RGB10_A2UI:
				return asset::EF_A2B10G10R10_UINT_PACK32;
				break;
			case GL_R11F_G11F_B10F:
				return asset::EF_B10G11R11_UFLOAT_PACK32;
				break;
			case GL_RGB9_E5:
				return asset::EF_E5B9G9R9_UFLOAT_PACK32;
				break;
			case GL_RG16:
				return asset::EF_R16G16_UNORM;
				break;
			case GL_RG16I:
				return asset::EF_R16G16_SINT;
				break;
			case GL_RG16UI:
				return asset::EF_R16G16_UINT;
				break;
			case GL_RG16F:
				return asset::EF_R16G16_SFLOAT;
				break;
			case GL_R32I:
				return asset::EF_R32G32_SINT;
				break;
			case GL_R32UI:
				return asset::EF_R32G32_UINT;
				break;
			case GL_R32F:
				return asset::EF_R32_SFLOAT;
				break;
			case GL_RGB12:
				///return asset::EF_;
				break;
			case GL_DEPTH32F_STENCIL8:
				return asset::EF_D32_SFLOAT_S8_UINT;
				break;
			case GL_RGBA12:
				///return asset::EF_;
				break;
			case GL_RGB16:
				return asset::EF_R16G16B16_UNORM;
				break;
			case GL_RGB16I:
				return asset::EF_R16G16B16_SINT;
				break;
			case GL_RGB16UI:
				return asset::EF_R16G16B16_UINT;
				break;
			case GL_RGB16_SNORM:
				return asset::EF_R16G16B16_SNORM;
				break;
			case GL_RGB16F:
				return asset::EF_R16G16B16_SFLOAT;
				break;
			case GL_RGBA16:
				return asset::EF_R16G16B16A16_UNORM;
				break;
			case GL_RGBA16I:
				return asset::EF_R16G16B16A16_SINT;
				break;
			case GL_RGBA16UI:
				return asset::EF_R16G16B16A16_UINT;
				break;
			case GL_RGBA16F:
				return asset::EF_R16G16B16A16_SFLOAT;
				break;
			case GL_RG32I:
				return asset::EF_R32G32_SINT;
				break;
			case GL_RG32UI:
				return asset::EF_R32G32_UINT;
				break;
			case GL_RG32F:
				return asset::EF_R32G32_SFLOAT;
				break;
			case GL_RGB32I:
				return asset::EF_R32G32B32_SINT;
				break;
			case GL_RGB32UI:
				return asset::EF_R32G32B32_UINT;
				break;
			case GL_RGB32F:
				return asset::EF_R32G32B32_SFLOAT;
				break;
			case GL_RGBA32I:
				return asset::EF_R32G32B32A32_SINT;
				break;
			case GL_RGBA32UI:
				return asset::EF_R32G32B32A32_UINT;
				break;
			case GL_RGBA32F:
				return asset::EF_R32G32B32A32_SFLOAT;
				break;
			default:
				break;
			}
			return asset::EF_UNKNOWN;
		}


	} // end namespace video
} // end namespace irr

#endif // _IRR_COMPILE_WITH_OPENGL_


#endif