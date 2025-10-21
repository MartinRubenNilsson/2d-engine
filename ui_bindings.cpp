#include "stdafx.h"
#include "ui_bindings.h"
#include "ui_rmlui.h"

namespace ui {
	namespace bindings {
		std::string textbox_text; // RML
		bool textbox_has_sprite = false;
		std::string textbox_sprite;
		bool textbox_has_options = false;
		std::vector<std::string> textbox_options;
		size_t textbox_selected_option = 0;
	}

	extern Rml::Context* _context;
	Rml::DataModelHandle _data_model_handle;

	bool is_variable_dirty(const std::string& name) {
		return _data_model_handle.IsVariableDirty(name);
	}

	void dirty_all_variables() {
		_data_model_handle.DirtyAllVariables();
	}

	template <void (*Func)()>
	Rml::DataEventFunc _wrap() {
		return [](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { Func(); };
	}

	void create_bindings() {
		Rml::DataModelConstructor data_model = _context->CreateDataModel("data_model");
		if (!data_model) return;
		_data_model_handle = data_model.GetModelHandle();

		// REGISTER TYPES

		data_model.RegisterArray<std::vector<std::string>>();

		// BIND VARIABLES

		data_model.Bind("textbox_text", &bindings::textbox_text);
		data_model.Bind("textbox_has_sprite", &bindings::textbox_has_sprite);
		data_model.Bind("textbox_sprite", &bindings::textbox_sprite);
		data_model.Bind("textbox_has_options", &bindings::textbox_has_options);
		data_model.Bind("textbox_options", &bindings::textbox_options);
		data_model.Bind("textbox_selected_option", &bindings::textbox_selected_option);

		// BIND FUNCTIONS

		data_model.BindEventCallback("on_click_play", _wrap<bindings::on_click_play>());
		data_model.BindEventCallback("on_click_settings", _wrap<bindings::on_click_settings>());
		data_model.BindEventCallback("on_click_credits", _wrap<bindings::on_click_credits>());
		data_model.BindEventCallback("on_click_back", _wrap<bindings::on_click_back>());
		data_model.BindEventCallback("on_click_resume", _wrap<bindings::on_click_resume>());
		data_model.BindEventCallback("on_click_main_menu", _wrap<bindings::on_click_main_menu>());
	}
}
