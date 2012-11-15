#include "ui/message_box.h"
#include "ui/button.h"
#include "ui/text_box.h"


namespace ui {

	MessageBox::MessageBox(const IRect &rect, const char *message) :Window(rect) {
		int2 bottom(rect.width() / 2, rect.height());

		attach(new TextBox(IRect(0, 0, rect.width(), bottom.y - 5), message));
		attach(new Button(IRect(-100, -30, -5, -10) + bottom, "yes", 1));
		attach(new Button(IRect(5, -30, 100, -10) + bottom, "no", 0));
	}

	bool MessageBox::onEvent(const Event &event) {
		if(event.type == Event::button_clicked) {
			close(event.value);
			return true;
		}

		return false;
	}

	void MessageBox::drawContents() const {
		drawWindow(IRect({0, 0}, rect().size()), Color::gui_dark, 3);
	}

}
