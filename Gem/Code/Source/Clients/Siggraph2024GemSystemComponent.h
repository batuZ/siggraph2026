/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>

#include <Atom/RPI.Public/Pass/PassSystemInterface.h>

#include <Siggraph2024Gem/Siggraph2024GemBus.h>

namespace Siggraph2024Gem
{
    class Siggraph2024GemSystemComponent
        : public AZ::Component
        , protected Siggraph2024GemRequestBus::Handler
    {
    public:
        AZ_COMPONENT_DECL(Siggraph2024GemSystemComponent);

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        Siggraph2024GemSystemComponent();
        ~Siggraph2024GemSystemComponent();

    protected:
        ////////////////////////////////////////////////////////////////////////
        // Siggraph2024GemRequestBus interface implementation

        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        //! ATTENTION: Begin
        //! The following code was commented out because since Dec 14th, 2024 it is possible
        //! to register Pass Template Mapping assets without C++. See details in this wiki:
        //! https://github.com/o3de/o3de/wiki/%5BAtom%5D-Work-With-Passes-In-Gems
        // // Load pass template mappings for this Gem.
        // void LoadPassTemplateMappings();
        // 
        // // We use this event handler to add our Pass Templates to the RPI::PassSystem.
        // // The callback will invoke this->LoadPassTemplateMappings()
        // AZ::RPI::PassSystemInterface::OnReadyLoadTemplatesEvent::Handler m_loadTemplatesHandler;
        //! ATTENTION: End
        ////////////////////////////////////////////////////////////////////////


    };

} // namespace Siggraph2024Gem
