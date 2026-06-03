/*
 * Copyright (c) Contributors to the Grayscale Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/string/string.h>

#include <Atom/Bootstrap/BootstrapNotificationBus.h>
#include <Atom/RPI.Public/DynamicDraw/DynamicDrawContext.h>
#include <Atom/RPI.Public/DynamicDraw/DynamicDrawInterface.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/SceneBus.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <Atom/RHI/RHISystemInterface.h>

#include <Grayscale2D/Grayscale2DBus.h>

namespace Grayscale2D
{
    // Vertex layout: matches InitVertexFormat(POSITION R32G32B32, COLOR B8G8R8A8, TEXCOORD0 R32G32)
    struct Gs2dVertex
    {
        float xyz[3];
        AZ::u32 color; // packed 0xAARRGGBB
        float st[2];
    };

    struct ShaderData
    {
        AZ::RHI::ShaderInputImageIndex    m_imageInputIndex;
        AZ::RHI::ShaderInputConstantIndex m_viewProjInputIndex;
        AZ::RPI::ShaderVariantId          m_variantClamp;
        AZ::RPI::ShaderVariantId          m_variantWrap;
    };

    // Pending draw commands queued from the game thread and flushed on the render thread
    struct RectCmd
    {
        AZ::Vector2 pos;
        AZ::Vector2 size;
        AZ::u32     packedColor;
    };

    struct SpriteCmd
    {
        AZ::Data::Instance<AZ::RPI::Image> image;
        AZ::Vector2 pos;
        AZ::Vector2 size; // width = natural width * scale, height = natural height * scale
    };

    class Grayscale2DSystemComponent
        : public AZ::Component
        , protected Grayscale2DRequestBus::Handler
        , public AZ::Render::Bootstrap::NotificationBus::Handler
        , public AZ::RPI::SceneNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(Grayscale2DSystemComponent, "{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        // Bootstrap::NotificationBus — called once the RPI is ready
        void OnBootstrapSceneReady(AZ::RPI::Scene* bootstrapScene) override;

        // SceneNotificationBus — called each frame before views are submitted
        void OnBeginPrepareRender() override;

        // Grayscale2DRequestBus
        void DrawRect(const AZ::Vector2& position, const AZ::Vector2& size, const AZ::Color& color) override;
        void DrawSprite(const char* spritePath, const AZ::Vector2& position, float scale) override;
        void SetCameraOffset(const AZ::Vector2& offset) override;
        AZ::Vector2 GetCameraOffset() const override;

    private:
        // Build 6 vertices (two triangles) for a screen-space quad and submit them
        void SubmitQuad(
            const AZ::Vector2& pos, const AZ::Vector2& size, AZ::u32 packedColor,
            AZ::Data::Instance<AZ::RPI::Image> image, bool clamp,
            const AZ::RHI::Viewport& viewport);

        // Returns the orthographic projection matrix for the given viewport
        static AZ::Matrix4x4 BuildOrthoMatrix(const AZ::RHI::Viewport& vp);

        // Pack AZ::Color -> 0xAARRGGBB uint32
        static AZ::u32 PackColor(const AZ::Color& c);

        AZ::RHI::Ptr<AZ::RPI::DynamicDrawContext> m_dynamicDraw;
        ShaderData m_shaderData;

        // Sprite image cache: asset path -> loaded image
        AZStd::unordered_map<AZStd::string, AZ::Data::Instance<AZ::RPI::Image>> m_imageCache;

        // Pending draw lists — written from game thread, flushed from render thread
        AZStd::mutex m_drawMutex;
        AZStd::vector<RectCmd>   m_pendingRects;
        AZStd::vector<SpriteCmd> m_pendingSprites;

        AZ::Vector2 m_cameraOffset = AZ::Vector2::CreateZero();
    };

} // namespace Grayscale2D
