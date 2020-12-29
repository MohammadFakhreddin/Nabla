#ifndef __IRR_I_GPU_QUEUE_H_INCLUDED__
#define __IRR_I_GPU_QUEUE_H_INCLUDED__

#include <nbl/core/IReferenceCounted.h>
#include <nbl/video/IGPUPrimaryCommandBuffer.h>
#include <nbl/video/IGPUSemaphore.h>
#include <nbl/video/IGPUFence.h>
#include <nbl/asset/IRenderpass.h>

namespace nbl {
namespace video
{

class IGPUQueue : public core::IReferenceCounted
{
public:
    enum E_CREATE_FLAGS : uint32_t
    {
        ECF_PROTECTED_BIT = 0x01
    };

    struct SSubmitInfo
    {
        uint32_t waitSemaphoreCount;
        IGPUSemaphore** pWaitSemaphores;
        const asset::E_PIPELINE_STAGE_FLAGS* pWaitDstStageMask;
        uint32_t signalSemaphoreCount;
        IGPUSemaphore** pSignalSemaphores;
        uint32_t commandBufferCount;
        IGPUPrimaryCommandBuffer** commandBuffers;
    };

    //! `flags` takes bits from E_CREATE_FLAGS
    IGPUQueue(uint32_t _famIx, E_CREATE_FLAGS _flags, float _priority)
        : m_flags(_flags), m_familyIndex(_famIx), m_priority(_priority)
    {

    }

    virtual void submit(uint32_t _count, const SSubmitInfo* _submits, IGPUFence* _fence) = 0;

    float getPriority() const { return m_priority; }
    uint32_t getFamilyIndex() const { return m_familyIndex; }
    E_CREATE_FLAGS getFlags() const { return m_flags; }

protected:
    const uint32_t m_familyIndex;
    const E_CREATE_FLAGS m_flags;
    const float m_priority;
};

}}

#endif