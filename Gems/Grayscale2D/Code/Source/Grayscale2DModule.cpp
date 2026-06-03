/*
 * Copyright (c) Contributors to the Grayscale Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include "Grayscale2DSystemComponent.h"

#include <IGem.h>

namespace Grayscale2D
{
    class Grayscale2DModule : public CryHooksModule
    {
    public:
        AZ_RTTI(Grayscale2DModule, "{B1C2D3E4-F5A6-7890-BCDE-F01234567891}", CryHooksModule);

        Grayscale2DModule()
            : CryHooksModule()
        {
            m_descriptors.insert(m_descriptors.end(), {
                Grayscale2DSystemComponent::CreateDescriptor(),
            });
        }

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<Grayscale2DSystemComponent>(),
            };
        }
    };

} // namespace Grayscale2D

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), Grayscale2D::Grayscale2DModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_Grayscale2D, Grayscale2D::Grayscale2DModule)
#endif
