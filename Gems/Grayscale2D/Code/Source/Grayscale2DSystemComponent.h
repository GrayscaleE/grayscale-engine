/*
 * Copyright (c) Contributors to the Grayscale Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Math/Vector2.h>
#include <Grayscale2D/Grayscale2DBus.h>

namespace Grayscale2D
{
    class Grayscale2DSystemComponent
        : public AZ::Component
        , protected Grayscale2DRequestBus::Handler
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

        // Grayscale2DRequestBus
        void DrawRect(const AZ::Vector2& position, const AZ::Vector2& size, const AZ::Color& color) override;
        void DrawSprite(const char* spritePath, const AZ::Vector2& position, float scale) override;
        void SetCameraOffset(const AZ::Vector2& offset) override;
        AZ::Vector2 GetCameraOffset() const override;

    private:
        AZ::Vector2 m_cameraOffset = AZ::Vector2::CreateZero();
    };

} // namespace Grayscale2D
