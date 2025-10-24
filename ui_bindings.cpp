#include "stdafx.h"
#include "ui_bindings.h"
#include "ui_rmlui.h"

namespace ui {
	extern Rml::Context* _context;
	Rml::DataModelHandle _data_model_handle;

	template <void (*Func)()>
	Rml::DataEventFunc _wrap() {
		return [](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { Func(); };
	}

	void create_bindings() {
		Rml::DataModelConstructor data_model = _context->CreateDataModel("data_model");
		if (!data_model) return;
		_data_model_handle = data_model.GetModelHandle();

		data_model.BindEventCallback("on_click_back", _wrap<bindings::on_click_back>());
	}
}
