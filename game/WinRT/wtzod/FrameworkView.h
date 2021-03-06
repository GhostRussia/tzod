﻿#pragma once

#include "pch.h"
#include "StepTimer.h"

namespace DX
{
	class DeviceResources;
	class SwapChainResources;
}

namespace FS
{
	class FileSystem;
}

namespace UI
{
	class ConsoleBuffer;
}

class TzodApp;
class TzodView;
class StoreAppWindow;

namespace wtzod
{
	// Main entry point for our app. Connects the app with the Windows shell and handles application lifecycle events.
	ref class FrameworkView sealed : public Windows::ApplicationModel::Core::IFrameworkView
	{
	internal:
		FrameworkView(FS::FileSystem &fs, UI::ConsoleBuffer &logger, TzodApp &app);

	public:
		virtual ~FrameworkView();

		// IFrameworkView
		virtual void Initialize(Windows::ApplicationModel::Core::CoreApplicationView^ coreApplicationView);
		virtual void SetWindow(Windows::UI::Core::CoreWindow^ coreWindow);
		virtual void Load(Platform::String^ entryPoint);
		virtual void Run();
		virtual void Uninitialize();

	protected:
		// Application lifecycle event handlers.
		void OnAppViewActivated(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView, Windows::ApplicationModel::Activation::IActivatedEventArgs^ args);
		void OnAppSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ args);
		void OnAppResuming(Platform::Object^ sender, Platform::Object^ args);

		// Window event handlers.
		void OnWindowSizeChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::WindowSizeChangedEventArgs^ args);
		void OnVisibilityChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::VisibilityChangedEventArgs^ args);
		void OnWindowClosed(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::CoreWindowEventArgs^ args);

		// DisplayInformation event handlers.
		void OnDpiChanged(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);
		void OnOrientationChanged(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);
		void OnDisplayContentsInvalidated(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);

	private:
		FS::FileSystem &_fs;
		UI::ConsoleBuffer &_logger;
		TzodApp &_app;

		Platform::Agile<Windows::UI::Core::CoreWindow>  m_window;
		std::shared_ptr<DX::DeviceResources> m_deviceResources;
		std::shared_ptr<DX::SwapChainResources> m_swapChainResources;

		std::unique_ptr<StoreAppWindow> _appWindow;
		std::unique_ptr<TzodView> _view;

		// Rendering loop timer.
		DX::StepTimer m_timer;

		bool m_windowClosed;
		bool m_windowVisible;
		
		void HandleDeviceLost();
	};
}
