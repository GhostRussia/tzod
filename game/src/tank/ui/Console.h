// Console.h

#pragma once

#include "Base.h"
#include "Window.h"


// forward declarations
class ConsoleBuffer;

namespace UI
{
///////////////////////////////////////////////////////////////////////////////

class Console : public Window
{
	Text  *_arrow;
	Edit  *_input;
	size_t _scrollBack;
	size_t _cmdIndex;

	size_t _font;

	ConsoleBuffer * const _buf;

	bool _echo;

public:
	Console(Window *parent, float x, float y, float w, float h, ConsoleBuffer *buf);
	virtual ~Console();
	void SetEcho(bool echo);
	Delegate<void(const string_t &)> eventOnSendCommand;
	Delegate<bool(const string_t &, string_t &)> eventOnRequestCompleteCommand;

protected:
	virtual void OnChar(int c);
	virtual void OnRawChar(int c);
	virtual bool OnMouseWheel(float x, float y, float z);
	virtual bool OnMouseDown(float x, float y, int button);

	virtual void DrawChildren(float sx, float sy) const;
	virtual void OnSize(float width, float height);
	virtual bool OnFocus(bool focus);
};


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
