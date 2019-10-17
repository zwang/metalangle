//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// UtilsMtl.h:
//    Defines the class interface for UtilsMtl.
//

#ifndef LIBANGLE_RENDERER_METAL_UTILSMTL_H_
#define LIBANGLE_RENDERER_METAL_UTILSMTL_H_

#import <Metal/Metal.h>

#include "libANGLE/angletypes.h"
#include "libANGLE/renderer/metal/StateCacheMtl.h"
#include "libANGLE/renderer/metal/mtl_command_buffer.h"

namespace rx
{

class BufferMtl;
class ContextMtl;
class RendererMtl;

class UtilsMtl : public mtl::Context, angle::NonCopyable
{
  public:
    UtilsMtl(RendererMtl *renderer);
    ~UtilsMtl();

    struct ClearParams : public mtl::ClearOptions
    {
        gl::Rectangle clearArea;

        bool flipY = false;
    };

    struct BlitParams
    {
        gl::Offset dstOffset;
        // Destination texture needs to have viewport Y flipped?
        // The difference between this param and unpackFlipY is that unpackFlipY is from
        // glCopyImageCHROMIUM(), and dstFlipY controls whether the final viewport needs to be
        // flipped when drawing to destination texture.
        bool dstFlipY = false;

        MTLColorWriteMask dstColorMask = MTLColorWriteMaskAll;

        mtl::TextureRef src;
        uint32_t srcLevel = 0;
        gl::Rectangle srcRect;
        bool srcYFlipped            = false;  // source texture has data flipped in Y direction
        bool unpackFlipY            = false;  // flip texture data copying process in Y direction
        bool unpackPremultiplyAlpha = false;
        bool unpackUnmultiplyAlpha  = false;
        bool dstLuminance           = false;
    };

    struct TriFanFromArrayParams
    {
        uint32_t firstVertex;
        uint32_t vertexCount;
        mtl::BufferRef dstBuffer;
        // Must be multiples of kBufferSettingOffsetAlignment
        uint32_t dstOffset;
    };

    struct IndexConversionParams
    {
        gl::DrawElementsType srcType;
        GLsizei indexCount;
        const void *indices;
        mtl::BufferRef dstBuffer;
        uint32_t dstOffset;
    };

    angle::Result initialize();
    void onDestroy();

    // Clear current framebuffer
    void clearWithDraw(const gl::Context *context,
                       mtl::RenderCommandEncoder *cmdEncoder,
                       const ClearParams &params);
    // Blit texture data to current framebuffer
    void blitWithDraw(const gl::Context *context,
                      mtl::RenderCommandEncoder *cmdEncoder,
                      const BlitParams &params);

    angle::Result convertIndexBuffer(const gl::Context *context,
                                     gl::DrawElementsType srcType,
                                     uint32_t indexCount,
                                     mtl::BufferRef srcBuffer,
                                     uint32_t srcOffset,
                                     mtl::BufferRef dstBuffer,
                                     // Must be multiples of kBufferSettingOffsetAlignment
                                     uint32_t dstOffset);
    angle::Result generateTriFanBufferFromArrays(const gl::Context *context,
                                                 const TriFanFromArrayParams &params);
    angle::Result generateTriFanBufferFromElementsArray(const gl::Context *context,
                                                        const IndexConversionParams &params);

    angle::Result generateLineLoopLastSegment(const gl::Context *context,
                                              uint32_t firstVertex,
                                              uint32_t lastVertex,
                                              mtl::BufferRef dstBuffer,
                                              uint32_t dstOffset);
    angle::Result generateLineLoopLastSegmentFromElementsArray(const gl::Context *context,
                                                               const IndexConversionParams &params);

    angle::Result dispatchCompute(const gl::Context *context,
                                  mtl::ComputeCommandEncoder *encoder,
                                  id<MTLComputePipelineState> pipelineState,
                                  size_t numThreads);

  private:
    // override mtl::ErrorHandler
    void handleError(GLenum error,
                     const char *file,
                     const char *function,
                     unsigned int line) override;
    void handleError(NSError *_Nullable error,
                     const char *file,
                     const char *function,
                     unsigned int line) override;

    angle::Result initShaderLibrary();
    void initClearResources();
    void initBlitResources();

    void setupClearWithDraw(const gl::Context *context,
                            mtl::RenderCommandEncoder *cmdEncoder,
                            const ClearParams &params);
    void setupBlitWithDraw(const gl::Context *context,
                           mtl::RenderCommandEncoder *cmdEncoder,
                           const BlitParams &params);
    id<MTLDepthStencilState> getClearDepthStencilState(const gl::Context *context,
                                                       const ClearParams &params);
    id<MTLRenderPipelineState> getClearRenderPipelineState(const gl::Context *context,
                                                           mtl::RenderCommandEncoder *cmdEncoder,
                                                           const ClearParams &params);
    id<MTLRenderPipelineState> getBlitRenderPipelineState(const gl::Context *context,
                                                          mtl::RenderCommandEncoder *cmdEncoder,
                                                          const BlitParams &params);
    void setupBlitWithDrawUniformData(mtl::RenderCommandEncoder *cmdEncoder,
                                      const BlitParams &params);

    void setupDrawCommonStates(mtl::RenderCommandEncoder *cmdEncoder);

    mtl::AutoObjCPtr<id<MTLComputePipelineState>> getIndexConversionPipeline(
        ContextMtl *context,
        gl::DrawElementsType srcType,
        uint32_t srcOffset);
    mtl::AutoObjCPtr<id<MTLComputePipelineState>> getTriFanFromElemArrayGeneratorPipeline(
        ContextMtl *context,
        gl::DrawElementsType srcType,
        uint32_t srcOffset);
    angle::Result ensureTriFanFromArrayGeneratorInitialized(ContextMtl *context);
    angle::Result generateTriFanBufferFromElementsArrayGPU(
        const gl::Context *context,
        gl::DrawElementsType srcType,
        uint32_t indexCount,
        mtl::BufferRef srcBuffer,
        uint32_t srcOffset,
        mtl::BufferRef dstBuffer,
        // Must be multiples of kBufferSettingOffsetAlignment
        uint32_t dstOffset);
    angle::Result generateTriFanBufferFromElementsArrayCPU(const gl::Context *context,
                                                           const IndexConversionParams &params);
    angle::Result generateLineLoopLastSegmentFromElementsArrayCPU(
        const gl::Context *context,
        const IndexConversionParams &params);

    mtl::AutoObjCPtr<id<MTLLibrary>> mDefaultShaders = nil;
    RenderPipelineCacheMtl mClearRenderPipelineCache;
    RenderPipelineCacheMtl mBlitRenderPipelineCache;
    RenderPipelineCacheMtl mBlitPremultiplyAlphaRenderPipelineCache;
    RenderPipelineCacheMtl mBlitUnmultiplyAlphaRenderPipelineCache;

    struct IndexConvesionPipelineCacheKey
    {
        gl::DrawElementsType srcType;
        bool srcBufferOffsetAligned;

        bool operator==(const IndexConvesionPipelineCacheKey &other) const;
        bool operator<(const IndexConvesionPipelineCacheKey &other) const;
    };
    std::map<IndexConvesionPipelineCacheKey, mtl::AutoObjCPtr<id<MTLComputePipelineState>>>
        mIndexConversionPipelineCaches;
    std::map<IndexConvesionPipelineCacheKey, mtl::AutoObjCPtr<id<MTLComputePipelineState>>>
        mTriFanFromElemArrayGeneratorPipelineCaches;
    mtl::AutoObjCPtr<id<MTLComputePipelineState>> mTriFanFromArraysGeneratorPipeline;
};

}  // namespace rx

#endif /* LIBANGLE_RENDERER_METAL_UTILSMTL_H_ */