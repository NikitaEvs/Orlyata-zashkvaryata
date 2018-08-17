#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/common/thread.hpp>

#include <sqlite_orm/sqlite_orm.h>

#include "json/json.hpp"

#include <iostream>
#include <set>
#include <vector>
#include <string.h>

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

std::string debug = "DEBUG"; // On debug mode with start of program for e.g. ./server DEBUG in directory of project
/* Command for DEBUG mode */
std::string drop = "DROP"; // Drop all database
std::string add = "ADD"; // Add new Pineapple box with pattern: ADD %token1% %name1% %token2% %name2%
std::string give = "GIVE"; // Give pineapple box from datebase with pattern: GIVE %token%
std::string exitDebug = "EXIT"; // Exit from debug mode
std::string help = "HELP"; // Info about command
std::string end = "END"; // End input new part
/* -----------------------*/
const int port = 8080;

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

struct pineBox {
	int token;
	std::string name;
	std::string email;
	std::string login;
	std::string password;
};

class database {
	public:
		static auto initDatabase(){
			std::cout << "Init database!" << std::endl;
			return sqlite_orm::make_storage("server.sqlite",
						sqlite_orm::make_table("PINEBOX",
								sqlite_orm::make_column("TOKEN", &pineBox::token, sqlite_orm::primary_key()),
								sqlite_orm::make_column("NAME", &pineBox::name),
								sqlite_orm::make_column("EMAIL", &pineBox::email),
								sqlite_orm::make_column("LOGIN", &pineBox::login),
								sqlite_orm::make_column("PASSWORD", &pineBox::password)));
		}

		database() {
			decltype(initDatabase()) storage = initDatabase();
			storage.sync_schema();
		}

		void drop() {
			std::cout << "Drop database in code!" << std::endl;
			decltype(initDatabase()) storage = initDatabase();
			storage.remove_all<pineBox>();
			storage.sync_schema();
		}

		void add(int token) {
			std::cout << "Add new elements in database" << std::endl;
			decltype(initDatabase()) storage = initDatabase();
			pineBox newPine{ token, "", "", "", "" };
			storage.insert(newPine);
			storage.sync_schema();
		}

		void add(int token, std::string name) {
			std::cout << "Add new elements in database" << std::endl;
			decltype(initDatabase()) storage = initDatabase();
			pineBox newPine{ token, name, "", "", "" };
			std::cout << "Create new pine" << std::endl;
			storage.insert(newPine);
			std::cout << "Insert new pine" << std::endl;
			storage.sync_schema();
		}

		void add(int token, std::string name, std::string email) {
			std::cout << "Add new elements in database" << std::endl;
			decltype(initDatabase()) storage = initDatabase();
			pineBox newPine{ token, name, email, "", "" };
			storage.insert(newPine);
			storage.sync_schema();
		}

		void add(int token, std::string name, std::string email, std::string login) {
			std::cout << "Add new elements in database" << std::endl;
			decltype(initDatabase()) storage = initDatabase();
			pineBox newPine{ token, name, email, login, "" };
			storage.insert(newPine);
			storage.sync_schema();
		}

		void add(int token, std::string name, std::string email, std::string login, std::string password) {
			std::cout << "Add new elements in database" << std::endl;
			decltype(initDatabase()) storage = initDatabase();
			pineBox newPine{ token, name, email, login, password };
			storage.insert(newPine);
			storage.sync_schema();
		}

		void updateItem(int token) {
			std::cout << "Update element in database" << std::endl;
			decltype(initDatabase()) storage = initDatabase();
			pineBox editedPine = storage.get<pineBox>(token);
			editedPine.token = token;
			storage.update(editedPine);
			storage.sync_schema();
		}

		void updateItem(int token, std::string name) {
			std::cout << "Update element in database" << std::endl;
			decltype(initDatabase()) storage = initDatabase();
			pineBox editedPine = storage.get<pineBox>(token);
			editedPine.name = name;	
			storage.update(editedPine);
			storage.sync_schema();
		}

		void updateItem(int token, std::string name, std::string email) {
			std::cout << "Update element in database" << std::endl;
			decltype(initDatabase()) storage = initDatabase();
			pineBox editedPine = storage.get<pineBox>(token);
			editedPine.name = name;
			editedPine.email = email;
			storage.update(editedPine);
			storage.sync_schema();		
		}

		void updateItem(int token, std::string name, std::string email, std::string login) {
			std::cout << "Update element in database" << std::endl;
			decltype(initDatabase()) storage = initDatabase();
			pineBox editedPine = storage.get<pineBox>(token);
			editedPine.name = name;
			editedPine.email = email;
			editedPine.login = login;
			storage.update(editedPine);
			storage.sync_schema();		
		}

		void updateItem(int token, std::string name, std::string email, std::string login, std::string password) {
			std::cout << "Update element in database" << std::endl;
			decltype(initDatabase()) storage = initDatabase();
			pineBox editedPine = storage.get<pineBox>(token);
			editedPine.name = name;
			editedPine.email = email;
			editedPine.login = login;
			editedPine.password = password;	
			storage.update(editedPine);
			storage.sync_schema();	
		}
		
		void updateItemPass(int token, std::string password) {
			std::cout << "Update element in database" << std::endl;
			decltype(initDatabase()) storage = initDatabase();
			pineBox editedPine = storage.get<pineBox>(token);
			editedPine.password = password;
			storage.update(editedPine);
			storage.sync_schema();
		}
		
		pineBox givePine(int token) {
			std::cout << "Give Pine info" << std::endl;
			decltype(initDatabase()) storage = initDatabase();
			return storage.get<pineBox>(token);
		}
	private:
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

int main(int argc, char *argv[]) {
	std::cout << "FOR START DEBUG MODE TYPE DEBUG IN CONSOLE ARGUMENT" << std::endl;
	database pineBase;
	if(argc > 1){
		std::string in = argv[1];
		if(in == debug) {
			std::string input = "";
			std::cout << "*******START DEBUG MODE!*******" << std::endl;
			std::cout << "*FOR EXIT TYPE IN CONSOLE EXIT*" << std::endl;
			std::cout << "******TYPE HELP FOR INFO!******" << std::endl;
			std::cout << "*******************************" << std::endl;
			while(input != exitDebug) {
				std::cout << "input: ";
				getline(std::cin, input);
				if(input == drop) { // Function for drop database
					std::cout << "Drop database!" << std::endl;
					pineBase.drop();	
				} else if(input == add) { // Function for add new elements
					std::cout << "Add new elements!" << std::endl;
					std::cout << "For type element:" << std::endl;
					std::cout << "#token1# ENTER" << std::endl;
					std::cout << "#name1# ENTER" << std::endl;
					std::cout << "#token2# ENTER" << std::endl;
					std::cout << "#name2# ENTER" << std::endl;
					std::cout << "END" << std::endl;
					std::vector<int> tokens;
					std::vector<std::string> names;
					std::string tempInput;
					std::cout << "type token: ";
					getline(std::cin, tempInput);
					while (tempInput != end) {
						tokens.push_back(std::stoi(tempInput));
						std::cout << "type name: ";
						getline(std::cin, tempInput);
						names.push_back(tempInput);
						std::cout << "For end type END or continue add elements via type token" << std::endl;
						getline(std::cin, tempInput);
					}		
					for(int i = 0; i < tokens.size(); i++){
						std::cout << "Add new element in fori" << std::endl;
						pineBase.add(tokens[i], names[i]);
					}
				} else if(input == give) {
					std::cout << "Give element!" << std::endl;
					std::cout << "For give element type token or type END for exit from giving mode" << std::endl;
					std::string tempInput;
					while (tempInput != end) {
						std::cout << "type token: ";
						getline(std::cin, tempInput);
						int token = std::stoi(tempInput);
						pineBox pine = pineBase.givePine(token);
						std::cout << "***********" << std::endl;
						std::cout << "*Name: " << pine.name << std::endl;
						std::cout << "*Token: " << pine.token << std::endl;
						std::cout << "*Login: " << pine.login << std::endl;
						std::cout << "***********" << std::endl;
						getline(std::cin, tempInput);
					}
				} else if(input == help) {
					std::cout << "*************************************" << std::endl;
					std::cout << "*Info about all debug command:      *" << std::endl;
					std::cout << "*DEBUG - start debug mode           *" << std::endl;
					std::cout << "*DROP - drop all database on server *" << std::endl;
					std::cout << "*ADD - start add mode               *" << std::endl;
					std::cout << "*GIVE - start give mode             *" << std::endl;
					std::cout << "*END - end mode                     *" << std::endl;
					std::cout << "*EXIT - finish debug mode           *" << std::endl;
					std::cout << "*HELP - info about all debug command*" << std::endl;
					std::cout << "*************************************" << std::endl;
				}
			}
		}
	}
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
