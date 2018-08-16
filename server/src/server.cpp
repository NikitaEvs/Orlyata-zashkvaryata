#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include "json/json.hpp"

#include <iostream>
#include <set>
#include <websocketpp/common/thread.hpp>

using websocketpp::connection_hdl;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;
using websocketpp::lib::lock_guard;
using websocketpp::lib::thread;
using websocketpp::lib::mutex;
using websocketpp::lib::condition_variable;
using websocketpp::lib::unique_lock;

typedef websocketpp::server<websocketpp::config::asio> server;
int port = 8080;

enum action_type {
	SUBSCRIBE,
	UNSUBSCRIBE,
	MESSAGE
};

struct action {
	action(action_type type, connection_hdl handler) : 
		type(type), 
		handler(handler) {}
	action(action_type type, connection_hdl handler, server::message_ptr message) :  
		type(type),
		handler(handler),
		msg(message) {}
	action_type type;
	websocketpp::connection_hdl handler;
	server::message_ptr msg;
};

class pineServer {
	public:
		pineServer() {
			std::cout << "Start init server!" << std::endl;
			currentServer.init_asio();

			currentServer.set_open_handler(bind(&pineServer::on_open, this, ::_1));
			currentServer.set_close_handler(bind(&pineServer::on_close, this, ::_1));
			currentServer.set_message_handler(bind(&pineServer::on_message, this, ::_1, ::_2));
			std::cout << "Open handlers!" << std::endl;
		}

		void run(uint16_t port){
			std::cout << "Configure running" << std::endl;
			currentServer.listen(port);
			currentServer.start_accept();
			
			try{
				std::cout << "Running" << std::endl;
				currentServer.run();
			} catch (websocketpp::exception const & e) {
				std::cout << "Oops. Err code: " << e.what() << std::endl;
			} catch (...) {
				std::cout << "Unknown exception :c " << std::endl;
			}
		}
		
		void on_open(connection_hdl handler){
			{
				std::cout << "Connection open!" << std::endl;
				lock_guard<mutex> guard(action_lock);
				actions.push(action(SUBSCRIBE, handler)); 
			}
			action_cond.notify_one();
		}

		void on_close(connection_hdl handler){
			{
				std::cout << "Connection close!" << std::endl;
				lock_guard<mutex> guard(action_lock);
				actions.push(action(UNSUBSCRIBE, handler));	
			}
			action_cond.notify_one();
		}

		void on_message(connection_hdl handler, server::message_ptr msg){
			{
				std::cout << "Receive message" << std::endl;
				lock_guard<mutex> guard(action_lock);
				actions.push(action(MESSAGE, handler, msg));
			}
			action_cond.notify_one();
		}
		void messages_process(){
			while(true){
				std::cout << "Start messages process" << std::endl;
				unique_lock<mutex> lock(action_lock);
				while(actions.empty()){
					std::cout << "Wait for connections" << std::endl;
					action_cond.wait(lock);
				}
				std::cout << "Take action" << std::endl;
				action action = actions.front();
				actions.pop();
				lock.unlock();

				if(action.type == SUBSCRIBE){ // New connection, need to send auth request
					std::cout << "NEW CONNECTION" << std::endl;
					lock_guard<mutex> guard(connection_lock);
					con_list.insert(action.handler);
					/* For tests */
					websocketpp::lib::error_code errCode;
					currentServer.send(action.handler, "kek", websocketpp::frame::opcode::text, errCode);
					/*-----------*/
				} else if(action.type == UNSUBSCRIBE){ // Connection closed
					std::cout << "CLOSE CONNECTION" << std::endl;
					lock_guard<mutex> guard(connection_lock);
					con_list.erase(action.handler);
				} else if(action.type == MESSAGE){ // New message for handle
					std::cout << "NEW MESSAGE" << std::endl;
					lock_guard<mutex> guard(connection_lock);

				}
			}
		}
	private:
		typedef std::set<connection_hdl, std::owner_less<connection_hdl> > connection_list;

		server currentServer;
		connection_list con_list;
		mutex action_lock;
		mutex connection_lock;
		std::queue<action> actions;
		condition_variable action_cond;
};

int main() {
	try{
		pineServer pineServer;
		thread thread(bind(&pineServer::messages_process, &pineServer));
		pineServer.run(port);
		thread.join();
	} catch (websocketpp::exception const & e) {
		std::cout << "Error in main: " << e.what() << std::endl;
	}
	return 0; 
}
