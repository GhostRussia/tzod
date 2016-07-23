#pragma once
#include "Window.h"
#include <memory>

namespace UI
{
	class ScrollView
		: public Window
		, private ScrollSink
	{
	public:
		ScrollView(LayoutManager &manager);

		void SetContent(std::shared_ptr<Window> content);

		// Window
		ScrollSink* GetScrollSink() override { return this; }
		FRECT GetChildRect(vec2d size, float scale, const Window &child) const override;

	private:
		std::shared_ptr<Window> _content;
		vec2d _offset = {};

		// ScrollSink
		void OnScroll(UI::InputContext &ic, vec2d size, float scale, vec2d pointerPosition, vec2d offset) override;
	};

}// namespace UI