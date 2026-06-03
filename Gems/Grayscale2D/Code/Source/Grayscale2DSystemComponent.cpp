/*
 * Copyright (c) Contributors to the Grayscale Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include "Grayscale2DSystemComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace Grayscale2D
{
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
                ec->Class<Grayscale2DSystemComponent>("Grayscale 2D", "Provides 2D rendering and sprite features for the Grayscale Engine.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Grayscale Engine")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Grayscale2DSystemComponent::m_cameraOffset,
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

    void Grayscale2DSystemComponent::Init()
    {
    }

    void Grayscale2DSystemComponent::Activate()
    {
        Grayscale2DRequestBus::Handler::BusConnect();
    }

    void Grayscale2DSystemComponent::Deactivate()
    {
        Grayscale2DRequestBus::Handler::BusDisconnect();
    }

    void Grayscale2DSystemComponent::DrawRect(
        [[maybe_unused]] const AZ::Vector2& position,
        [[maybe_unused]] const AZ::Vector2& size,
        [[maybe_unused]] const AZ::Color& color)
    {
        // TODO: submit a 2D quad draw call via Atom RPI
    }

    void Grayscale2DSystemComponent::DrawSprite(
        [[maybe_unused]] const char* spritePath,
        [[maybe_unused]] const AZ::Vector2& position,
        [[maybe_unused]] float scale)
    {
        // TODO: look up sprite asset, bind texture, submit draw call
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
