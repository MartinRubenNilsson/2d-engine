#include "stdafx.h"
#ifdef _DEBUG_IMGUI
#include "imgui_impl.h"
#include "window.h"
#include "graphics.h"
#include "graphics_api.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#ifdef GRAPHICS_API_OPENGL
#include <imgui_impl_opengl3.h>
#endif
#ifdef GRAPHICS_API_VULKAN
#include <vulkan/vulkan.h> //TODO: remove
#include <imgui_impl_vulkan.h>
#endif
#ifdef GRAPHICS_API_D3D11
#include <imgui_impl_dx11.h>
#endif

namespace graphics {
#ifdef GRAPHICS_API_VULKAN
	extern VkInstance _instance;
	extern VkPhysicalDevice _physical_device;
	extern VkDevice _device;
	extern uint32_t _queue_family;
	extern VkQueue _queue;
#endif
}

namespace imgui_impl {

	bool startup() {
		IMGUI_CHECKVERSION();
		if (!ImGui::CreateContext()) {
			return false;
		}
#ifdef GRAPHICS_API_OPENGL
		if (!ImGui_ImplGlfw_InitForOpenGL((GLFWwindow*)window::get_glfw_window(), true)) {
			return false;
		}
		if (!ImGui_ImplOpenGL3_Init()) {
			return false;
		}
#endif
#ifdef GRAPHICS_API_VULKAN
		ImGui_ImplGlfw_InitForVulkan((GLFWwindow*)window::get_glfw_window(), true);
		ImGui_ImplVulkan_InitInfo init_info{};
		init_info.Instance = graphics::_instance;
		init_info.PhysicalDevice = graphics::_physical_device;
		init_info.Device = graphics::_device;
		init_info.QueueFamily = graphics::_queue_family;
		init_info.Queue = graphics::_queue;
		//TODO: rest
		ImGui_ImplVulkan_Init(&init_info);
#endif
#ifdef GRAPHICS_API_D3D11
		if (!ImGui_ImplGlfw_InitForOther((GLFWwindow*)window::get_glfw_window(), true)) {
			return false;
		}
		if (!ImGui_ImplDX11_Init(
			graphics::api::get_d3d11_device(),
			graphics::api::get_d3d11_device_context()
		)) {
			return false;
		}
#endif
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
		ImGui::GetIO().Fonts->AddFontFromFileTTF("assets/fonts/Consolas.ttf", 18);
		// https://github.com/ocornut/imgui/issues/707#issuecomment-252413954
		ImGuiStyle& style = ImGui::GetStyle();
		style.Colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 0.90f);
		style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
		style.Colors[ImGuiCol_WindowBg] = ImVec4(0.09f, 0.09f, 0.15f, 1.00f);
		style.Colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		style.Colors[ImGuiCol_PopupBg] = ImVec4(0.05f, 0.05f, 0.10f, 0.85f);
		style.Colors[ImGuiCol_Border] = ImVec4(0.70f, 0.70f, 0.70f, 0.65f);
		style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		style.Colors[ImGuiCol_FrameBg] = ImVec4(0.00f, 0.00f, 0.01f, 1.00f);
		style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.90f, 0.80f, 0.80f, 0.40f);
		style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.90f, 0.65f, 0.65f, 0.45f);
		style.Colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.83f);
		style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.40f, 0.40f, 0.80f, 0.20f);
		style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.00f, 0.00f, 0.00f, 0.87f);
		style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.01f, 0.01f, 0.02f, 0.80f);
		style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.20f, 0.25f, 0.30f, 0.60f);
		style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.55f, 0.53f, 0.55f, 0.51f);
		style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.56f, 0.56f, 0.56f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.91f);
		style.Colors[ImGuiCol_CheckMark] = ImVec4(0.90f, 0.90f, 0.90f, 0.83f);
		style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.70f, 0.70f, 0.70f, 0.62f);
		style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.30f, 0.30f, 0.30f, 0.84f);
		style.Colors[ImGuiCol_Button] = ImVec4(0.48f, 0.72f, 0.89f, 0.49f);
		style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.50f, 0.69f, 0.99f, 0.68f);
		style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.80f, 0.50f, 0.50f, 1.00f);
		style.Colors[ImGuiCol_Header] = ImVec4(0.30f, 0.69f, 1.00f, 0.53f);
		style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.44f, 0.61f, 0.86f, 1.00f);
		style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.38f, 0.62f, 0.83f, 1.00f);
		style.Colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.85f);
		style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.60f);
		style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(1.00f, 1.00f, 1.00f, 0.90f);
		style.Colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.00f, 0.00f, 1.00f, 0.35f);
		style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
		return true;
	}

	void shutdown() {
#ifdef GRAPHICS_API_OPENGL
		ImGui_ImplOpenGL3_Shutdown();
#endif
#ifdef GRAPHICS_API_VULKAN
		ImGui_ImplVulkan_Shutdown();
#endif
#ifdef GRAPHICS_API_D3D11
		ImGui_ImplDX11_Shutdown();
#endif
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void new_frame() {
#ifdef GRAPHICS_API_OPENGL
		ImGui_ImplOpenGL3_NewFrame();
#endif
#ifdef GRAPHICS_API_VULKAN
		ImGui_ImplVulkan_NewFrame();
#endif
#ifdef GRAPHICS_API_D3D11
		ImGui_ImplDX11_NewFrame();
#endif
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	void render() {
		const graphics::ScopedDebugGroup debug_group(__FUNCTION__);
		ImGui::Render();
#ifdef GRAPHICS_API_OPENGL
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#endif
#ifdef GRAPHICS_API_VULKAN
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData()); //TODO
#endif
#ifdef GRAPHICS_API_D3D11
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
#endif
	}
}

#endif // _DEBUG_IMGUI