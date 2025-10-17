#include "stdafx.h"
#include "steam.h"
#include "steam_server.h"
#include <steam/steam_api.h>
#include "console.h"

#pragma comment(lib, "steam_api64")

namespace steam {
	const uint32_t _APP_ID = 0; // Invalid ID for now
	bool _is_initialized = false;
	bool _is_overlay_active = false;

	bool restart_app_if_necessary() {
		return SteamAPI_RestartAppIfNecessary(_APP_ID);
	}

	bool is_steam_running() {
		return SteamAPI_IsSteamRunning();
	}

	bool startup() {
		if (_is_initialized) return false;
		if (!SteamAPI_Init()) return false;
		SteamAPI_ManualDispatch_Init();
		_is_initialized = true;
		return true;
	}

	void shutdown() {
		server_shutdown(); // Make sure any sockets are closed
		if (!_is_initialized) return;
		SteamAPI_Shutdown();
		_is_initialized = false;
	}

	void _process_callback(HSteamPipe steam_pipe, int identifier, void* callback);

	void _process_steam_api_call_completed_callback(HSteamPipe steam_pipe, SteamAPICallCompleted_t* callback) {
		void* call_result = malloc(callback->m_cubParam);
		bool failed = true;
		if (SteamAPI_ManualDispatch_GetAPICallResult(
			steam_pipe,
			callback->m_hAsyncCall,
			call_result,
			callback->m_cubParam,
			callback->m_iCallback,
			&failed)) {
			_process_callback(steam_pipe, callback->m_iCallback, call_result);
		}
		free(call_result);
	}

	void _process_game_overlay_activated_callback(GameOverlayActivated_t* callback) {
		_is_overlay_active = callback->m_bActive;
	}

	void _process_steam_net_connection_status_changed_callback(SteamNetConnectionStatusChangedCallback_t* callback) {
		switch (callback->m_info.m_eState) {
			case k_ESteamNetworkingConnectionState_None: {
				console::log("Connection doesn't exist or has already been closed");
			} break;
			case k_ESteamNetworkingConnectionState_Connecting: {
				console::log("Connection is connecting");
			} break;
			case k_ESteamNetworkingConnectionState_FindingRoute: {
				console::log("Connection is finding the route");
			} break;
			case k_ESteamNetworkingConnectionState_Connected: {
				console::log("Connection is connected");
			} break;
			case k_ESteamNetworkingConnectionState_ClosedByPeer: {
				console::log("Connection was closed by the peer");
			} break;
			case k_ESteamNetworkingConnectionState_ProblemDetectedLocally: {
				console::log("Connection has a problem detected locally");
			} break;
			case k_ESteamNetworkingConnectionState_FinWait: {
				console::log("Connection is in the fin wait state");
			} break;
			case k_ESteamNetworkingConnectionState_Linger: {
				console::log("Connection is in the linger state");
			} break;
			case k_ESteamNetworkingConnectionState_Dead: {
				console::log("Connection is dead");
			} break;
		}
	}

	void _process_callback(HSteamPipe steam_pipe, int identifier, void* callback) {
		switch (identifier) {
			case SteamAPICallCompleted_t::k_iCallback: {
				_process_steam_api_call_completed_callback(steam_pipe, (SteamAPICallCompleted_t*)callback);
			} break;
			case GameOverlayActivated_t::k_iCallback: {
				_process_game_overlay_activated_callback((GameOverlayActivated_t*)callback);
			} break;
													/*case GameConnectedFriendChatMsg_t::k_iCallback: {
														GameConnectedFriendChatMsg_t* chat_msg = (GameConnectedFriendChatMsg_t*)callback.m_pubParam;
														std::string log_msg;
														log_msg += SteamFriends()->GetFriendPersonaName(chat_msg->m_steamIDUser);
														log_msg += ": ";
														log_msg += SteamFriends()->GetFriendMessage(chat_msg->m_steamIDUser, chat_msg->m_iMessageID);

														log_error("Received chat message from friend " + std::to_string(chat_msg->m_steamIDUser.ConvertToUint64()) + ": " + chat_msg->m_data);
													} break;*/
			case SteamNetConnectionStatusChangedCallback_t::k_iCallback: {
				_process_steam_net_connection_status_changed_callback((SteamNetConnectionStatusChangedCallback_t*)callback);
			} break;
			case SteamServerConnectFailure_t::k_iCallback: {
				console::log_error("Failed to connect to Steam server");
			} break;
		}
	}

	void run_message_loop() {
		if (!_is_initialized) return;
		HSteamPipe steam_pipe = SteamAPI_GetHSteamPipe();
		SteamAPI_ManualDispatch_RunFrame(steam_pipe);
		CallbackMsg_t callback{};
		while (SteamAPI_ManualDispatch_GetNextCallback(steam_pipe, &callback)) {
			_process_callback(steam_pipe, callback.m_iCallback, callback.m_pubParam);
			SteamAPI_ManualDispatch_FreeLastCallback(steam_pipe);
		}
	}

	bool is_overlay_active() {
		return _is_overlay_active;
	}

	std::string get_steam_id() {
		if (!SteamUser()) return {};
		CSteamID steam_id = SteamUser()->GetSteamID();
		return std::to_string(steam_id.ConvertToUint64());
	}

	std::vector<std::string> get_friends_persona_names() {
		if (!SteamFriends()) return {};
		std::vector<std::string> names;
		int friends_count = SteamFriends()->GetFriendCount(k_EFriendFlagImmediate);
		for (int i = 0; i < friends_count; i++) {
			CSteamID friend_id = SteamFriends()->GetFriendByIndex(i, k_EFriendFlagImmediate);
			names.push_back(SteamFriends()->GetFriendPersonaName(friend_id));
		}
		return names;
	}
}