#include "stdafx.h"
#include "engine_fps_counter.h"

namespace engine {
	bool should_show_fps_counter = false;

    float _smoothed_dt = 0.f;
    float _smoothed_fps = 0.f;
    float _dt_buffer[256] = { 0.f };
    float _fps_buffer[256] = { 0.f };
    int _buffer_offset = 0;

	void update_fps_counter(float dt) {
        _dt_buffer[_buffer_offset] = dt;
        _fps_buffer[_buffer_offset] = 1.f / dt;
        _buffer_offset = (_buffer_offset + 1) % 256;
        constexpr float SMOOTHING_FACTOR = 0.99f;
        _smoothed_dt = SMOOTHING_FACTOR * _smoothed_dt + (1.f - SMOOTHING_FACTOR) * dt;
        _smoothed_fps = SMOOTHING_FACTOR * _smoothed_fps + (1.f - SMOOTHING_FACTOR) / dt;
	}

	void show_fps_counter_imgui() {
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::Begin("Stats", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);
        char overlay_text[64];
        sprintf(overlay_text, "%.f us", _smoothed_dt * 1'000'000.f);
        ImGui::PlotLines("##dt", _dt_buffer, 256, _buffer_offset, overlay_text, 0.f, 0.01f, ImVec2(0, 80));
        sprintf(overlay_text, "%.f FPS", _smoothed_fps);
        ImGui::PlotLines("##fps", _fps_buffer, 256, _buffer_offset, overlay_text, 0.f, 600.f, ImVec2(0, 80));
        ImGui::End();
	}
}