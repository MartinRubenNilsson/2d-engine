#pragma once

namespace platform {
	bool is_debugger_present();
	void debug_break();
	void output_debug_string(const char* string);
}