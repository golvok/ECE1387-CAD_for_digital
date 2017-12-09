#include "graphics_wrapper.hpp"

#include <graphics/graphics.hpp>
#include <util/thread_utils.hpp>

#include <thread>

namespace graphics {

Graphics::Graphics()
	: enabled(false)
	, impl(nullptr)
{ }

Graphics::~Graphics() { }

class Graphics::Impl {
private:
	bool initialized;
	bool close_requested;
	std::thread gui_thread;

	util::SafeWaitForNotify wait_for_proceed;

	Graphics* graphics_interface;
public:
	Impl(Graphics* interface)
		: initialized(false)
		, close_requested(false)
		, gui_thread()
		, wait_for_proceed()
		, graphics_interface(interface)
	{
		startThreadsAndOpenWindow();
	}

	Impl(const Impl&) = delete;
	Impl(Impl&&) = delete;
	Impl& operator=(const Impl&) = delete;
	Impl& operator=(Impl&&) = delete;

	void startThreadsAndOpenWindow() {
		if (initialized) return;
		initialized = true;
		// XInitThreads();

		gui_thread = std::thread([&]() noexcept {
			// std::string title;
			// for (int i = 0; i < argc; ++i) {
			// 	title += argv[i];
			// 	if (i != argc-1) {
			// 		title += ' ';
			// 	}
			// }
			graphics::init_graphics("", graphics::WHITE);

			while (!close_requested) {
				graphics::event_loop(
					[this](float x, float y, graphics::t_event_buttonPressed button_info) noexcept {
						// mousebutton
						(void)x; (void)y; (void)button_info;
					},
					[this](float x, float y) noexcept {
						// mousemove
						(void)x; (void)y;
					},
					[this](char key_pressed, int keysym) noexcept {
						// keypress
						(void)key_pressed; (void)keysym;
					},
					[this]() noexcept {
						// drawscreen
						graphics::clearscreen();
						graphics_interface->drawAll();
					}
				);
				wait_for_proceed.notify_all();
			}

			graphics::close_graphics();
		});
	}

	void waitForPress() {
		if (initialized) {
			wait_for_proceed.wait();
		}
	}

	void refresh() {
		graphics::refresh_graphics();
	}

	void close() {
		close_requested = true;
		graphics::simulate_proceed();
	}

	void join() {		
		if (gui_thread.joinable()) {
			gui_thread.join();
		}
	}

};

void Graphics::waitForPress() {
	if (impl) impl->waitForPress();
}

void Graphics::refresh() {
	if (impl) impl->refresh();
}

void Graphics::close() {
	if (impl) impl->close();
}

void Graphics::join() {
	if (impl) impl->join();
}

void Graphics::startThreadsAndOpenWindow() {
	if (enabled && !impl) {
		impl.reset(new Impl(this));
	}
}

} // end namespace graphics
