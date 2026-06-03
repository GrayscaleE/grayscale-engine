/*
 * Copyright (c) Contributors to the Grayscale Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include "Grayscale2DSystemComponent.h"

#include <AzCore/Math/MatrixUtils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RHI/RHIUtils.h>

#include <AzFramework/Asset/AssetCatalogBus.h>

namespace Grayscale2D
{
    // -------------------------------------------------------------------------
    // Static helpers
    // -------------------------------------------------------------------------

    AZ::u32 Grayscale2DSystemComponent::PackColor(const AZ::Color& c)
    {
        return (c.GetA8() << 24) | (c.GetR8() << 16) | (c.GetG8() << 8) | c.GetB8();
    }

    AZ::Matrix4x4 Grayscale2DSystemComponent::BuildOrthoMatrix(const AZ::RHI::Viewport& vp)
    {
        AZ::Matrix4x4 mat;
        AZ::MakeOrthographicMatrixRH(
            mat,
            vp.m_minX,
            vp.m_minX + (vp.m_maxX - vp.m_minX),
            vp.m_minY + (vp.m_maxY - vp.m_minY),
            vp.m_minY,
            vp.m_maxZ,
            vp.m_minZ);
        return mat;
    }

    // -------------------------------------------------------------------------
    // Reflection / services
    // -------------------------------------------------------------------------

    void Grayscale2DSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<Grayscale2DSystemComponent, AZ::Component>()
                ->Version(1)
                ->Field("CameraOffset", &Grayscale2DSystemComponent::m_cameraOffset)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<Grayscale2DSystemComponent>("Grayscale 2D",
                    "Provides 2D rendering and sprite features for the Grayscale Engine.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Grayscale Engine")
                    ->DataElement(AZ::Edit::UIHandlers::Default,
                        &Grayscale2DSystemComponent::m_cameraOffset,
                        "Camera Offset", "2D camera scroll offset in world units.")
                    ;
            }
        }
    }

    void Grayscale2DSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("Grayscale2DService"));
    }

    void Grayscale2DSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("Grayscale2DService"));
    }

    void Grayscale2DSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void Grayscale2DSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    // -------------------------------------------------------------------------
    // Component lifecycle
    // -------------------------------------------------------------------------

    void Grayscale2DSystemComponent::Init()
    {
    }

    void Grayscale2DSystemComponent::Activate()
    {
        Grayscale2DRequestBus::Handler::BusConnect();
        AZ::Render::Bootstrap::NotificationBus::Handler::BusConnect();
    }

    void Grayscale2DSystemComponent::Deactivate()
    {
        AZ::RPI::SceneNotificationBus::Handler::BusDisconnect();
        AZ::Render::Bootstrap::NotificationBus::Handler::BusDisconnect();
        Grayscale2DRequestBus::Handler::BusDisconnect();

        m_dynamicDraw = nullptr;
        m_imageCache.clear();
    }

    // -------------------------------------------------------------------------
    // Bootstrap: set up the DynamicDrawContext once the RPI is ready
    // -------------------------------------------------------------------------

    void Grayscale2DSystemComponent::OnBootstrapSceneReady(AZ::RPI::Scene* bootstrapScene)
    {
        AZ_Assert(bootstrapScene, "Bootstrap scene is null");

        // Reuse the same SimpleTextured shader LyShine uses — it has the "2dpass" draw list tag
        // and supports the shader options o_clamp / o_useColorChannels we need.
        AZ::Data::Instance<AZ::RPI::Shader> shader =
            AZ::RPI::LoadCriticalShader("Shaders/SimpleTextured.azshader");
        if (!shader)
        {
            AZ_Error("Grayscale2D", false, "Failed to load SimpleTextured.azshader");
            return;
        }

        m_dynamicDraw = AZ::RPI::DynamicDrawInterface::Get()->CreateDynamicDrawContext();
        m_dynamicDraw->InitShader(shader);
        m_dynamicDraw->InitVertexFormat({
            {"POSITION",  AZ::RHI::Format::R32G32B32_FLOAT},
            {"COLOR",     AZ::RHI::Format::B8G8R8A8_UNORM},
            {"TEXCOORD0", AZ::RHI::Format::R32G32_FLOAT}
        });
        m_dynamicDraw->AddDrawStateOptions(
            AZ::RPI::DynamicDrawContext::DrawStateOptions::PrimitiveType  |
            AZ::RPI::DynamicDrawContext::DrawStateOptions::BlendMode      |
            AZ::RPI::DynamicDrawContext::DrawStateOptions::DepthState     |
            AZ::RPI::DynamicDrawContext::DrawStateOptions::ShaderVariant);

        // Scope to the bootstrap scene; draw list tag "2dpass" is inherited from the shader
        m_dynamicDraw->SetOutputScope(bootstrapScene);
        m_dynamicDraw->EndInit();

        if (!m_dynamicDraw->IsReady())
        {
            AZ_Error("Grayscale2D", false, "DynamicDrawContext failed to initialize");
            m_dynamicDraw = nullptr;
            return;
        }

        // Cache shader input indices from a throw-away SRG
        {
            AZ::Data::Instance<AZ::RPI::ShaderResourceGroup> srgProbe = m_dynamicDraw->NewDrawSrg();
            AZ_Assert(srgProbe, "Failed to create probe SRG");
            const AZ::RHI::ShaderResourceGroupLayout* layout = srgProbe->GetLayout();

            m_shaderData.m_imageInputIndex =
                layout->FindShaderInputImageIndex(AZ::Name("m_texture"));
            AZ_Error("Grayscale2D", m_shaderData.m_imageInputIndex.IsValid(),
                "Shader input 'm_texture' not found");

            m_shaderData.m_viewProjInputIndex =
                layout->FindShaderInputConstantIndex(AZ::Name("m_worldToProj"));
            AZ_Error("Grayscale2D", m_shaderData.m_viewProjInputIndex.IsValid(),
                "Shader input 'm_worldToProj' not found");
        }

        // Cache shader variants: clamp (sprites) and wrap (tiling)
        AZ::RPI::ShaderOptionList optsClamp = {
            {AZ::Name("o_clamp"),           AZ::Name("true")},
            {AZ::Name("o_useColorChannels"), AZ::Name("true")}
        };
        AZ::RPI::ShaderOptionList optsWrap = {
            {AZ::Name("o_clamp"),           AZ::Name("false")},
            {AZ::Name("o_useColorChannels"), AZ::Name("true")}
        };
        m_shaderData.m_variantClamp = m_dynamicDraw->UseShaderVariant(optsClamp);
        m_shaderData.m_variantWrap  = m_dynamicDraw->UseShaderVariant(optsWrap);

        // Subscribe to per-frame render notifications now that the context is ready
        AZ::RPI::SceneNotificationBus::Handler::BusConnect(bootstrapScene->GetId());
    }

    // -------------------------------------------------------------------------
    // Per-frame render: flush pending draw commands
    // -------------------------------------------------------------------------

    void Grayscale2DSystemComponent::OnBeginPrepareRender()
    {
        if (!m_dynamicDraw || !m_dynamicDraw->IsReady())
        {
            return;
        }

        // Grab the viewport to build the projection matrix
        auto viewportContext =
            AZ::RPI::ViewportContextRequests::Get()->GetDefaultViewportContext();
        if (!viewportContext)
        {
            return;
        }
        const AZ::RHI::Viewport& viewport =
            viewportContext->GetWindowContext()->GetViewport();

        // Swap pending lists under the lock so the game thread can keep queuing
        AZStd::vector<RectCmd>   rects;
        AZStd::vector<SpriteCmd> sprites;
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_drawMutex);
            rects.swap(m_pendingRects);
            sprites.swap(m_pendingSprites);
        }

        // Draw solid-color rects using the system white texture
        for (const RectCmd& cmd : rects)
        {
            AZ::Vector2 pos = cmd.pos - m_cameraOffset;
            SubmitQuad(pos, cmd.size, cmd.packedColor, nullptr, /*clamp=*/true, viewport);
        }

        // Draw sprites
        for (const SpriteCmd& cmd : sprites)
        {
            AZ::Vector2 pos = cmd.pos - m_cameraOffset;
            SubmitQuad(pos, cmd.size, 0xFFFFFFFF, cmd.image, /*clamp=*/true, viewport);
        }
    }

    // -------------------------------------------------------------------------
    // Internal: build and submit a textured quad (2 triangles, 6 vertices)
    // -------------------------------------------------------------------------

    void Grayscale2DSystemComponent::SubmitQuad(
        const AZ::Vector2& pos, const AZ::Vector2& size, AZ::u32 packedColor,
        AZ::Data::Instance<AZ::RPI::Image> image, bool clamp,
        const AZ::RHI::Viewport& viewport)
    {
        const float x0 = pos.GetX();
        const float y0 = pos.GetY();
        const float x1 = x0 + size.GetX();
        const float y1 = y0 + size.GetY();
        const float z  = 1.0f; // far plane; depth test is disabled in SimpleTextured

        // Winding order matches LyShine's DeferredQuad: 0,1,3, 3,1,2
        // Corners: 0=TL, 1=TR, 2=BR, 3=BL
        Gs2dVertex verts[6] = {
            { {x0, y0, z}, packedColor, {0.f, 0.f} }, // TL
            { {x1, y0, z}, packedColor, {1.f, 0.f} }, // TR
            { {x0, y1, z}, packedColor, {0.f, 1.f} }, // BL
            { {x0, y1, z}, packedColor, {0.f, 1.f} }, // BL
            { {x1, y0, z}, packedColor, {1.f, 0.f} }, // TR
            { {x1, y1, z}, packedColor, {1.f, 1.f} }, // BR
        };

        m_dynamicDraw->SetShaderVariant(clamp ? m_shaderData.m_variantClamp : m_shaderData.m_variantWrap);

        AZ::Data::Instance<AZ::RPI::ShaderResourceGroup> drawSrg = m_dynamicDraw->NewDrawSrg();
        AZ_Assert(drawSrg, "Failed to create draw SRG");

        // Bind texture — fall back to white if none provided
        const AZ::RHI::ImageView* imageView = nullptr;
        if (image)
        {
            imageView = image->GetImageView();
        }
        if (!imageView)
        {
            auto white = AZ::RPI::ImageSystemInterface::Get()->GetSystemImage(AZ::RPI::SystemImage::White);
            imageView = white->GetImageView();
        }
        drawSrg->SetImageView(m_shaderData.m_imageInputIndex, imageView, 0);

        // Orthographic projection: maps screen pixels to NDC
        AZ::Matrix4x4 ortho = BuildOrthoMatrix(viewport);
        drawSrg->SetConstant(m_shaderData.m_viewProjInputIndex, ortho);

        drawSrg->Compile();

        m_dynamicDraw->SetPrimitiveType(AZ::RHI::PrimitiveTopology::TriangleList);
        m_dynamicDraw->DrawLinear(verts, AZ_ARRAY_SIZE(verts), drawSrg);
    }

    // -------------------------------------------------------------------------
    // EBus implementations
    // -------------------------------------------------------------------------

    void Grayscale2DSystemComponent::DrawRect(
        const AZ::Vector2& position, const AZ::Vector2& size, const AZ::Color& color)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_drawMutex);
        m_pendingRects.push_back({position, size, PackColor(color)});
    }

    void Grayscale2DSystemComponent::DrawSprite(
        const char* spritePath, const AZ::Vector2& position, float scale)
    {
        if (!spritePath || spritePath[0] == '\0')
        {
            return;
        }

        // Load or retrieve cached image (game thread safe — asset manager is thread-safe)
        AZ::Data::Instance<AZ::RPI::Image> image;
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_drawMutex);
            auto it = m_imageCache.find(spritePath);
            if (it != m_imageCache.end())
            {
                image = it->second;
            }
            else
            {
                // Resolve asset ID from path and load the streaming image
                AZ::Data::AssetId assetId;
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                    assetId,
                    &AZ::Data::AssetCatalogRequestBus::Events::GenerateAssetIdTEMP,
                    spritePath);
                assetId.m_subId = AZ::RPI::StreamingImageAsset::GetImageAssetSubId();

                auto streamingAsset = AZ::Data::AssetManager::Instance()
                    .FindOrCreateAsset<AZ::RPI::StreamingImageAsset>(
                        assetId, AZ::Data::AssetLoadBehavior::PreLoad);

                image = AZ::RPI::StreamingImage::FindOrCreate(streamingAsset);
                if (!image)
                {
                    AZ_Error("Grayscale2D", false,
                        "DrawSprite: failed to load image '%s'", spritePath);
                    return;
                }
                m_imageCache[spritePath] = image;
            }
        }

        // Derive pixel size from the image dimensions and requested scale
        const AZ::RHI::ImageDescriptor& desc = image->GetDescriptor();
        const AZ::Vector2 size(
            static_cast<float>(desc.m_size.m_width)  * scale,
            static_cast<float>(desc.m_size.m_height) * scale);

        AZStd::lock_guard<AZStd::mutex> lock(m_drawMutex);
        m_pendingSprites.push_back({image, position, size});
    }

    void Grayscale2DSystemComponent::SetCameraOffset(const AZ::Vector2& offset)
    {
        m_cameraOffset = offset;
    }

    AZ::Vector2 Grayscale2DSystemComponent::GetCameraOffset() const
    {
        return m_cameraOffset;
    }

} // namespace Grayscale2D
