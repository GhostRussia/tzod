// GuiManager.h

#pragma once

#include "Base.h"

namespace UI
{
	// forward declarations
	class LayoutManager;

	struct IWindowFactory
	{
		virtual Window* Create(LayoutManager *pManager) = 0;
	};

///////////////////////////////////////////////////////////////////////////////

class LayoutManager
{
public:
	LayoutManager(IWindowFactory *pDesktopFactory);
	~LayoutManager();

	//using DrawingContext::AttachRender;
	//using DrawingContext::DetachRender;

	void TimeStep(float dt);
	void Render() const;

	// TODO: should it handle WM_SIZE message instead of exposing SetCanvasSize?
	bool ProcessMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	bool ProcessMouse(float x, float y, float z, UINT msg);
	bool ProcessKeys(UINT msg, int c);

	TextureManager* GetTextureManager();
	Window* GetDesktop() const;

	Window* GetCapture() const;
	void SetCapture(Window* wnd);

	bool SetFocusWnd(Window* wnd);
	Window* GetFocusWnd() const;
	bool ResetFocus(Window* wnd);   // remove focus from wnd or any of its children

	Window* GetHotTrackWnd() const;

	bool IsMainWindowActive() const { return _isAppActive; }

private:
	friend class Window;
	void AddTopMost(Window* wnd, bool add);
	void ResetWindow(Window* wnd);
	PtrList<Window>::iterator TimeStepRegister(Window* wnd);
	void TimeStepUnregister(PtrList<Window>::iterator it);

private:
	bool ProcessMouseInternal(Window* wnd, float x, float y, float z, UINT msg);

	Window* _desktop;

	Window* _focusWnd;
	Window* _hotTrackWnd;
	Window* _captureWnd;

	PtrList<Window> _timestep;
	PtrList<Window> _topmost;

	unsigned int _captureCountSystem;
	unsigned int _captureCount;
	bool _isAppActive;
};

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI
// end of file
