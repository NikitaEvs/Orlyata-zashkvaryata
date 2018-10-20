/* _____  _                                    */
/*|  _  \(_)                            _      */
/*| |_) | _ ____  ___  __ _ ____  ____ | | ___ */
/*| ____/| |  _ \/ _ \/ _` |  _ \|  _ \| |/ _ \*/
/*| |    | | | |   __/ (_| | |_) | |_) | |  __/*/
/*|_|    |_|_| |_\___|\__,_| ___/| ___/|_|\___|*/
/*                         | |   | |           */
/*                         |_|   |_|           */

#ifndef PINESERVER_H_
#define PINESERVER_H_

#include "server.hpp"

string authReq = "authReq";
string authNotify = "authNotify";
string authResp = "authResp";
string positive = "ok";
string negative = "fail";
string dataReq = "dataReq";
string dataResp = "dataResp";
string nothing = "null";
string regReq = "regReq";
string regResp = "regResp";
string dataBox = "dataBox";
string dataBoxReq = "dataBoxReq";

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
		/* Simple init server with handlers */
		pineServer() {
			configure();
		}
		
		/* Server initialization */
		void configure() {
			std::cout << "Server initialization..." << std::endl;
			currentServer.init_asio();
			std::cout << "Setting handlers..." << std::endl;
			currentServer.set_open_handler(bind(&pineServer::on_open, this, ::_1));
			currentServer.set_close_handler(bind(&pineServer::on_close, this, ::_1));
			currentServer.set_message_handler(bind(&pineServer::on_message, this, ::_1, ::_2));
			std::cout << "Setting conditions..." << std::endl;
			currentServer.set_reuse_addr(true); // For fixing problem "Address already in use"
			std::cout << "Initialization pool..." << std::endl;
			std::shared_ptr<pinepool> poolCurrent(new pinepool);
			pool = poolCurrent;
			pinebase configPinebase;
			pool.get() -> createPool(connectionsCount, configPinebase.getConnectionCommand());
		}
		
		/* Initialization create mode for one variable */
		void configureCreateMode(string c_nameLoad, string c_nameDataset) {
			nameLoad = c_nameLoad;
			nameDataset = c_nameDataset;
			createMode = true;
			newVariable = true;
		}
		
		/* Update status of create mode */
		void updateCreateMode(string c_nameLoad) {
			nameLoad = c_nameLoad;
			newVariable = true;
		}	

		/* Init server run configuration */
		void run(uint16_t port) {
			std::cout << "Configure running..." << std::endl;
			currentServer.listen(port);
			currentServer.start_accept();
			try{
				std::cout << "Running..." << std::endl;
				currentServer.run();
			} catch (websocketpp::exception const & e) {
				std::cout << "Oops. Err code in running: " << e.what() << std::endl;
			} catch (...) {
				std::cout << "Unknown exception :c " << std::endl;
			}
		}

		/* Function was called with open new connection */
		void on_open(connection_hdl handler) {
			{
				std::cout << "OPEN CONNECTION" << std::endl;
				lock_guard<mutex> guard(action_lock);
				actions.push(action(SUBSCRIBE, handler)); 
			}
			action_cond.notify_one();
		}

		/* Function was called with close connection */
		void on_close(connection_hdl handler) {
			{
				std::cout << "CLOSE CONNECTION" << std::endl;
				lock_guard<mutex> guard(action_lock);
				actions.push(action(UNSUBSCRIBE, handler));	
			}
			action_cond.notify_one();
		}

		/* Function was called with new message */
		void on_message(connection_hdl handler, server::message_ptr msg) {
			{
				std::cout << "RECEIVE MESSAGE" << std::endl;
				lock_guard<mutex> guard(action_lock);
				actions.push(action(MESSAGE, handler, msg));
			}
			action_cond.notify_one();
		}

		/* Message analyze in infinity loop (using in thread!) */
		void messages_process() {
			while(true){
				unique_lock<mutex> lock(action_lock);
				while(actions.empty()){
					std::cout << "Wait for connections..." << std::endl;
					action_cond.wait(lock);
				}
				std::cout << "TAKE ACTION" << std::endl;
				action action = actions.front();
				actions.pop();
				lock.unlock();
				if(action.type == SUBSCRIBE){ 
					std::cout << "NEW CONNECTION" << std::endl;
					lock_guard<mutex> guard(connection_lock);
					websocketpp::lib::error_code errCode;
					currentServer.send(action.handler, createAuthReq(), websocketpp::frame::opcode::text, errCode);
				} else if(action.type == UNSUBSCRIBE){
					std::cout << "CLOSE CONNECTION" << std::endl;
					lock_guard<mutex> guard(connection_lock);
				} else if(action.type == MESSAGE){
					std::cout << "NEW MESSAGE" << std::endl;
					lock_guard<mutex> guard(connection_lock);
					websocketpp::lib::error_code errCode;
					currentServer.send(action.handler, analyzeJSON(action.msg -> get_payload()), websocketpp::frame::opcode::text, errCode);
				}
			}
		}
		
		/* Analyze input msg and return response */
		string analyzeJSON(string jsonStr) {
			pinebase configPinebase;
			pinebase pine(pool, configPinebase.getDatasetName());
			json jIn = json::parse(jsonStr);
			json jOut;
			string event = jIn["event"];
			std::cout << "Analyze msg with event: " << event << " with data: " << jIn["data"];
			if(event == authReq) {
				jOut["event"] = authResp;
				pinebox logPine = pine.getPineViaRegData(jIn["data"]["login"], jIn["data"]["pass"]);
				if(logPine.token != -1) {
					std::cout << "Login success!" << std::endl;
					jOut["data"] = positive;
				} else {
					std::cout << "Login failed!" << std::endl;
					jOut["data"] = negative;
				}
			} else if(event == dataReq) {
				jOut["event"] = dataResp;
				pinebox logPine = pine.getPineViaRegData(jIn["auth"]["login"], jIn["auth"]["pass"]);
				if(logPine.token != -1) {
					std::cout << "Auth correct" << std::endl;
					jOut["type"] = jIn["type"];
					if(jIn["type"] == "now") {
						pinebox pineData = pine.getPineOnlyLastdata(jIn["auth"]["login"].get<string>());
						jOut["data"] = pineData.lastdata;
					} else if(jIn["type"] == "hour") {
						pinebox pineData = pine.getPineOnlyHourdata(jIn["auth"]["login"].get<string>());
						jOut["data"] = pineData.hourdata;
					} else if(jIn["type"] == "day") {
						pinebox pineData = pine.getPineOnlyDaydata(jIn["auth"]["login"].get<string>());
						jOut["data"] = pineData.daydata;
					} else if(jIn["type"] == "month") {
						pinebox pineData = pine.getPineOnlyMonthdata(jIn["auth"]["login"].get<string>());
						jOut["data"] = pineData.monthdata;
					}
				} else {
					std::cout << "Auth incorrect" << std::endl;
					jOut["type"] = negative;
					jOut["data"] = nothing;
				}	
			} else if(event == regReq) {
				jOut["event"] = regResp;
				if(pine.reg(jIn["data"]["name"], jIn["data"]["login"], jIn["data"]["pass"])) {
					jOut["data"] = positive;
				} else {
					jOut["data"] = negative;
				}
			} else if(event == dataBox) {
				
				jOut["event"] = dataBoxReq;
				if(createMode) {
					pine.saveToDataset(jIn["data"]["values"].get<string>(), nameDataset, newVariable, nameLoad);
					newVariable = false;
				} else {
				std::cout << jIn["data"]["values"].get<string>() << std::endl;
			
					pine.saveToCSV(jIn["data"]["token"].get<int>(), jIn["data"]["values"].get<string>());
				}
				
			}
			
			return jOut.dump();
		}
		
	private:
		server currentServer;
		mutex action_lock;
		mutex connection_lock;
		std::shared_ptr<pinepool> pool; // Pool for connections, each pinebase include shared_ptr of pinepool
		std::queue<action> actions;
		condition_variable action_cond;
		
		std::string createAuthReq() {
			json j;
			j["event"] = authNotify;
			j["data"]["info"] = "null";
			return j.dump();
		}
		
		/* Variables for create-mode */
		string nameLoad;
		string nameDataset;
		bool createMode = false;
		bool newVariable = true;
};


#endif /* PINESERVER_H_ */
