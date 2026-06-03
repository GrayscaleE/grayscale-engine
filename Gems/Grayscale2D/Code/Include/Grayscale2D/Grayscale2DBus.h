/*
 * Copyright (c) Contributors to the Grayscale Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Color.h>

namespace Grayscale2D
{
    //! Requests for the Grayscale2D system — 2D rendering, sprite management, and tilemap queries.
    class Grayscale2DRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual ~Grayscale2DRequests() = default;

        //! Draw a solid colored rectangle in 2D screen space.
        //! @param position Top-left corner in pixels.
        //! @param size     Width and height in pixels.
        //! @param color    RGBA color.
        virtual void DrawRect(const AZ::Vector2& position, const AZ::Vector2& size, const AZ::Color& color) = 0;

        //! Draw a sprite by asset path at the given screen-space position.
        //! @param spritePath  Asset path to the sprite texture.
        //! @param position    Top-left corner in pixels.
        //! @param scale       Uniform scale applied to the sprite.
        virtual void DrawSprite(const char* spritePath, const AZ::Vector2& position, float scale) = 0;

        //! Set the global 2D camera offset (scroll / parallax).
        //! @param offset World-space offset in 2D units.
        virtual void SetCameraOffset(const AZ::Vector2& offset) = 0;

        //! Get the current 2D camera offset.
        virtual AZ::Vector2 GetCameraOffset() const = 0;
    };

    using Grayscale2DRequestBus = AZ::EBus<Grayscale2DRequests>;

} // namespace Grayscale2D
