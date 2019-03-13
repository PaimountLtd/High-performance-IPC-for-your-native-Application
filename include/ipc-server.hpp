/******************************************************************************
    Copyright (C) 2016-2019 by Streamlabs (General Workings Inc)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

******************************************************************************/

#pragma once
#include "ipc.hpp"
#include "ipc-class.hpp"
#include "ipc-server-instance.hpp"
#include <list>
#include <map>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include "../source/windows/named-pipe.hpp"

namespace ipc {
	class server_instance;

	typedef bool(*server_connect_handler_t)(void*, int64_t);
	typedef void(*server_disconnect_handler_t)(void*, int64_t);
	typedef void(*server_message_handler_t)(void*, int64_t, const std::vector<char>&);

	class server {
		bool m_isInitialized = false;

		// Functions		
		std::map<std::string, std::shared_ptr<ipc::collection>> m_classes;

		// Socket
		size_t backlog = 40;
		std::mutex m_sockets_mtx;
		std::list<std::shared_ptr<os::windows::named_pipe>> m_sockets;
		std::string m_socketPath = "";

		// Client management.
		std::mutex m_clients_mtx;
		std::map<std::shared_ptr<os::windows::named_pipe>, std::shared_ptr<server_instance>> m_clients;

		// Event Handlers
		std::pair<server_connect_handler_t, void*> m_handlerConnect;
		std::pair<server_disconnect_handler_t, void*> m_handlerDisconnect;
		std::pair<server_message_handler_t, void*> m_handlerMessage;

		// Worker
		struct {
			std::thread worker;
			bool stop = false;
		} m_watcher;

		void watcher();

		void spawn_client(std::shared_ptr<os::windows::named_pipe> socket);
		void kill_client(std::shared_ptr<os::windows::named_pipe> socket);

		public:
		server();
		~server();

		public: // Status
		void initialize(std::string socketPath);
		void finalize();

		public: // Events
		void set_connect_handler(server_connect_handler_t handler, void* data);
		void set_disconnect_handler(server_disconnect_handler_t handler, void* data);
		void set_message_handler(server_message_handler_t handler, void* data);

		public: // Functionality
		bool register_collection(std::shared_ptr<ipc::collection> cls);

		protected: // Client -> Server
		bool client_call_function(int64_t cid, const std::string &cname, const std::string &fname,
			std::vector<ipc::value>& args, std::vector<ipc::value>& rval, std::string& errormsg);

		friend class server_instance;
	};
}
