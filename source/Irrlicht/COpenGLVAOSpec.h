#ifndef __C_OPEN_GL_VAO_SPEC_H_INCLUDED__
#define __C_OPEN_GL_VAO_SPEC_H_INCLUDED__

#include "IrrCompileConfig.h"
#include "COpenGLBuffer.h"
#include "IMeshBuffer.h"

#ifdef _IRR_COMPILE_WITH_OPENGL_

namespace irr
{
namespace video
{
    static GLint eComponentsPerAttributeToGLint[scene::ECPA_COUNT] =
    {
        GL_BGRA,
        1,2,3,4
    };

    static GLenum eComponentTypeToGLenum[scene::ECT_COUNT] =
    {
        GL_FLOAT,
        GL_HALF_FLOAT,
        GL_DOUBLE,
        GL_UNSIGNED_INT_10F_11F_11F_REV,
        //normalized ints
        GL_INT_2_10_10_10_REV,
        GL_UNSIGNED_INT_2_10_10_10_REV,
        GL_BYTE,
        GL_UNSIGNED_BYTE,
        GL_SHORT,
        GL_UNSIGNED_SHORT,
        GL_INT,
        GL_UNSIGNED_INT,
        //unnorm ints
        GL_INT_2_10_10_10_REV,
        GL_UNSIGNED_INT_2_10_10_10_REV,
        GL_BYTE,
        GL_UNSIGNED_BYTE,
        GL_SHORT,
        GL_UNSIGNED_SHORT,
        GL_INT,
        GL_UNSIGNED_INT,
        //native ints
        GL_INT_2_10_10_10_REV,
        GL_UNSIGNED_INT_2_10_10_10_REV,
        GL_BYTE,
        GL_UNSIGNED_BYTE,
        GL_SHORT,
        GL_UNSIGNED_SHORT,
        GL_INT,
        GL_UNSIGNED_INT,
        //native double
        GL_DOUBLE
    };


    //! Convert E_PRIMITIVE_TYPE to OpenGL equivalent
    inline GLenum primitiveTypeToGL(scene::E_PRIMITIVE_TYPE type)
    {
        switch (type)
        {
            case scene::EPT_POINTS:
                return GL_POINTS;
            case scene::EPT_LINE_STRIP:
                return GL_LINE_STRIP;
            case scene::EPT_LINE_LOOP:
                return GL_LINE_LOOP;
            case scene::EPT_LINES:
                return GL_LINES;
            case scene::EPT_TRIANGLE_STRIP:
                return GL_TRIANGLE_STRIP;
            case scene::EPT_TRIANGLE_FAN:
                return GL_TRIANGLE_FAN;
            case scene::EPT_TRIANGLES:
                return GL_TRIANGLES;
        }
        return GL_TRIANGLES;
    }


    class COpenGLVAOSpec : public scene::IGPUMeshDataFormatDesc
    {
        public:
            struct HashAttribs
            {
                HashAttribs()
                {
                    static_assert(scene::ECPA_COUNT==5, "scene::ECPA_COUNT != 5"); //otherwise our hashing system falls apart
                    static_assert(scene::EVAI_COUNT==16, "scene::EVAI_COUNT != 16"); //otherwise our hashing system falls apart
                    static_assert(sizeof(HashAttribs)/sizeof(uint64_t)==(scene::EVAI_COUNT+2+_IRR_VAO_MAX_ATTRIB_DIVISOR_BITS*2+sizeof(uint64_t)-1)/sizeof(uint64_t), ""); //otherwise our hashing system falls apart

                    for (size_t i=0; i<getHashLength(); i++)
                        hashVal[i] = 0;
                }

                constexpr static size_t getHashLength() {return sizeof(hashVal)/sizeof(uint64_t);}

                inline void setAttrFmtAndCompCnt(const scene::E_VERTEX_ATTRIBUTE_ID& attrId, const scene::E_COMPONENTS_PER_ATTRIBUTE& components, const scene::E_COMPONENT_TYPE& type)
                {
                    attribFormatAndComponentCount[attrId] = components|(type<<3);
                }


                inline scene::E_COMPONENTS_PER_ATTRIBUTE getAttribComponentCount(const scene::E_VERTEX_ATTRIBUTE_ID& attrId) const
                {
                    return static_cast<scene::E_COMPONENTS_PER_ATTRIBUTE>(attribFormatAndComponentCount[attrId]&0x7u);
                }

                inline scene::E_COMPONENT_TYPE getAttribType(const scene::E_VERTEX_ATTRIBUTE_ID& attrId) const
                {
                    return static_cast<scene::E_COMPONENT_TYPE>(attribFormatAndComponentCount[attrId]>>3);
                }

                inline uint32_t getAttribDivisor(const scene::E_VERTEX_ATTRIBUTE_ID& attrId) const
                {
                    uint32_t retval = 0;
                    for (size_t i=0; i<_IRR_VAO_MAX_ATTRIB_DIVISOR_BITS; i++)
                    {
                        uint16_t mask = 0x1u<<attrId;
                        if (attributeDivisors[i]&mask)
                            retval |= 0x1u<<i;
                    }
                    return retval;
                }


                inline bool operator<(const HashAttribs& other) const
                {
                    for (size_t i=0; i<getHashLength()-1; i++)
                    {
                        if (hashVal[i]!=other.hashVal[i])
                            return hashVal[i]<other.hashVal[i];
                    }
                    /*
                    static_if(scene::EVAI_COUNT+2+_IRR_VAO_MAX_ATTRIB_DIVISOR_BITS*2==24)
                    {
                        return hashVal[2]<hashVal[2];
                    }
                    static_else
                    {*/
                        return hashVal[getHashLength()-1]<other.hashVal[getHashLength()-1];
                    //}
                }

                inline bool operator!=(const HashAttribs& other) const
                {
                    for (size_t i=0; i<getHashLength()-1; i++)
                    {
                        if (hashVal[i]!=other.hashVal[i])
                            return true;
                    }
                    /*
                    static_if(scene::EVAI_COUNT+2+_IRR_VAO_MAX_ATTRIB_DIVISOR_BITS*2==24)
                    {
                        return hashVal[2]<hashVal[2];
                    }
                    static_else
                    {*/
                        return hashVal[getHashLength()-1]!=other.hashVal[getHashLength()-1];
                    //}
                }

                inline bool operator==(const HashAttribs& other) const
                {
                    return !((*this)!=other);
                }

                union
                {
                    #include "irr/irrpack.h"
                    struct
                    {
                        uint8_t attribFormatAndComponentCount[scene::EVAI_COUNT];
                        uint16_t enabledAttribs;
                        uint16_t attributeDivisors[_IRR_VAO_MAX_ATTRIB_DIVISOR_BITS];
                    } PACK_STRUCT;
                    #include "irr/irrunpack.h"

                    uint64_t hashVal[(scene::EVAI_COUNT+2*_IRR_VAO_MAX_ATTRIB_DIVISOR_BITS+sizeof(uint64_t)-1)/sizeof(uint64_t)];
                };
            };

            COpenGLVAOSpec(core::LeakDebugger* dbgr=NULL);

            virtual void mapVertexAttrBuffer(IGPUBuffer* attrBuf, const scene::E_VERTEX_ATTRIBUTE_ID& attrId, scene::E_COMPONENTS_PER_ATTRIBUTE components, scene::E_COMPONENT_TYPE type, const size_t &stride=0, size_t offset=0, uint32_t divisor=0);

            inline const IGPUBuffer* const* getMappedBuffers() const
            {
                return mappedAttrBuf;
            }

            /**
            The function is virtual because it needs the current OpenGL buffer name of the element buffer.

            We will operate on some assumptions here:

            1) On all GPU's known to me  GPUs MAX_VERTEX_ATTRIB_BINDINGS <= MAX_VERTEX_ATTRIBS,
            so it makes absolutely no sense to support buffer binding mix'n'match as it wouldn't
            get us anything (however if MVAB>MVA then we could have more inputs into a vertex shader).
            Also the VAO Attrib Binding is a VAO state so more VAOs would have to be created in the cache.

            2) Relative byte offset on VAO Attribute spec is capped to 2047 across all GPUs, which makes it
            useful only for specifying the offset from a single interleaved buffer, since we have to specify
            absolute (unbounded) offset and stride when binding a buffer to a VAO bind-point, it makes absolutely
            no sense to use this feature as its redundant.

            So the only things worth tracking for the VAO are:
            1) Element Buffer Binding
            2) Per Attribute (x16)
                A) Enabled (1 bit)
                B) Format (5 bits)
                C) Component Count (3 bits)
                D) Divisors (32bits - no limit)

            Total 16*4+16+16/8+4 = 11 uint64_t

            If we limit divisors artificially to 1 bit

            16/8+16/8+16+4 = 3 uint64_t

            The limit is set at _IRR_VAO_MAX_ATTRIB_DIVISOR_BITS
            **/
            inline const HashAttribs& getHash() const
            {
                return individualHashFields;
            }

        protected:
            HashAttribs individualHashFields;

            virtual ~COpenGLVAOSpec();

        private:
            core::LeakDebugger* leakDebugger;
    };


} // end namespace video
} // end namespace irr


namespace std
{
    template <>
    class hash<irr::video::COpenGLVAOSpec::HashAttribs>
    {
        public :
            size_t operator()(const irr::video::COpenGLVAOSpec::HashAttribs &x ) const;
    };
}

#endif
#endif

