#include "stdafx.h"
#include "console_commands.h"
#include "ui.h"
#include "ui_textboxes.h"

namespace console {
	void _create_commands_ui() {
		create_command({
			.name = "debug_ui_layout",
			.desc = "Toggle debugging of UI layouts.",
			.execute = [](Args) { ui::debug_layout = !ui::debug_layout; }
			});
		create_command({
			.name = "debug_ui_bounding_boxes",
			.desc = "Toggle debug drawing of UI bounding boxes.",
			.execute = [](Args) { ui::debug_bounding_boxes = !ui::debug_bounding_boxes; }
			});
		create_command({
			.name = "open_textboxes",
			.desc = "Open all textboxes whose path start with the given path.",
			.params = { { ParamType::String, "path", "A textbox path." } },
			.execute = [](Args args) {
				const std::string_view path = get_string(args[0]);
				ui::textboxes::open_next(path);
			}
			});
	}
}