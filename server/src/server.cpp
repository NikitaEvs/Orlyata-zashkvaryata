#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/common/thread.hpp>

#include <sqlite_orm/sqlite_orm.h>

#include "json/json.hpp"

#include <iostream>
#include <set>
#include <vector>
#include <string.h>
#include <ctime>
#include <unistd.h>
#include <dirent.h>
#include <fstream>

using websocketpp::connection_hdl;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;
using websocketpp::lib::lock_guard;
using websocketpp::lib::thread;
using websocketpp::lib::mutex;
using websocketpp::lib::condition_variable;
using websocketpp::lib::unique_lock;

using json = nlohmann::json;

typedef websocketpp::server<websocketpp::config::asio> server;

std::string debug = "DEBUG"; // On debug mode with start of program for e.g. ./server DEBUG in directory of project
/* Command for DEBUG mode */
std::string drop = "DROP"; // Drop all database
std::string add = "ADD"; // Start add PineBox mode
std::string addData = "ADD_DATA"; // Start add data mode
std::string give = "GIVE"; // Give pineapple box from datebase with pattern: GIVE %token%
std::string exitDebug = "EXIT"; // Exit from debug mode
std::string help = "HELP"; // Info about command
std::string end = "END"; // End input new part
std::string giveCount = "GIVEC"; // Give count of elements in database
/* -----------------------*/
const int port = 8080;
bool valDatabaseUpdate = false;
std::ofstream fOut;

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
	bool regStatus;
	std::string name;
	std::string email;
	std::string login;
	std::string password;
};

struct pineBoxVal {
	int token;
	int val;
	//std::vector<std::vector<std::string> > types;
	std::string types;
	//std::vector<std::vector<int> > values;
	std::string values;
	//std::vector<std::string> timestamps;
	std::string timestamps;
	bool sent;
};

std::string getTime() {
	tm * timeinfo;
	time_t rawtime;
	char timebuff[120] = {0};
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	if (strftime(timebuff, 120, "%d.%m.%Y %H:%M:%S", timeinfo) == 0) {
		return "err";
	} else {
		return std::string(timebuff);
	}
}

class reformat {
	public:
		std::string typesToStr(std::vector<std::vector<std::string> > types) {
			std::string out = "";
			std::string diff = "|";
			std::string prob = " ";
			for(int i = 0; i < types.size(); i++) {
				for(int j = 0; j < types[i].size(); j++) {
					out += types[i][j] + prob;
				}
				out += diff;
			}
			return out;
		}

		std::string valuesToStr(std::vector<std::vector<int> > values) {
			std::string out = "";
			std::string diff = "|";
			std::string prob = " ";
			for(int i = 0; i < values.size(); i++) {
				for(int j = 0; j < values[i].size(); j++) {
					std::string temp;
					std::stringstream ss;
					ss << values[i][j];
					temp = ss.str();
					out += temp + prob;
				}
				out += diff;
			}
			return out;
		}

		std::string timestampsToStr(std::vector<std::string> timestamps) {
			std::string out = "";
			std::string diff = "|";
			for(int i = 0; i < timestamps.size(); i++) {
				out += timestamps[i] + diff;
			}
			return out;
		}

		std::vector<std::vector<std::string> > strToTypes(std::string in) {
			//std::cout << "Reformat to type, inStr: " << in << std::endl;
			std::vector<std::vector<std::string> > out;
			std::vector<std::string> temp = split(in, '|');
			for(int i = 0; i < temp.size(); i++) {
				std::vector<std::string> tempSplit = split(temp[i], ' ');
				for(int j = 0; j < tempSplit.size(); j++) {
					if(tempSplit[j] == " ") {
						tempSplit.erase(tempSplit.begin() + j);
					}
				}
				out.push_back(tempSplit);
			}
			
			std::cout << "Reformat to type, out size: " << out.size() << std::endl;
			return out;
		}

		std::vector<std::vector<int> > strToValues(std::string in) {
			std::vector<std::vector<int> > out;
			std::vector<std::string> temp = split(in, '|');
			for(int i = 0; i < temp.size(); i++) {
				out.push_back(splitToInt(temp[i], ' '));
			}
			return out;
		}

		std::vector<std::string> strToTimestamps(std::string in) {
			return split(in, '|');
		}	

		std::vector<std::string> split(std::string s, char c) {
			std::string buff("");
			std::vector<std::string> v;
			for(auto n:s) {
				if(n != c) {
					buff += n;
				} else if(n == c && buff != "") {
					v.push_back(buff);
					buff = "";
				}
			}
			if(buff != ""){
				v.push_back(buff);
			}
			return v;
		}

		std::vector<int> splitToInt(std::string s, char c) {
			std::string buff("");
			std::vector<int> v;
			for(auto n:s) {
				if(n != c) {
					buff += n;
				} else if(n == c && buff != "") {
					v.push_back(std::stoi(buff));
					buff = "";
				}
			}
			if(buff != ""){
				v.push_back(std::stoi(buff));
			}
			return v;
		}
		
		std::vector<double> splitToDouble(std::string s, char c) {
			std::string buff("");
			std::vector<double> v;
			for(auto n:s) {
				if(n != c) {
					buff += n;
				} else if(n == c && buff != "") {
					v.push_back(std::stod(buff));
					buff = "";
				}
			}
			if(buff != ""){
				v.push_back(std::stod(buff));
			}
			return v;
		}
	private:
		
};

class database {
	public:
		static auto initDatabase() {
			std::cout << "Init database!" << std::endl;
			return sqlite_orm::make_storage("server.sqlite",
						sqlite_orm::make_table("PINEBOX",
								sqlite_orm::make_column("TOKEN", &pineBox::token, sqlite_orm::primary_key()),
								sqlite_orm::make_column("REGSTATUS", &pineBox::regStatus),
								sqlite_orm::make_column("NAME", &pineBox::name),
								sqlite_orm::make_column("EMAIL", &pineBox::email),
								sqlite_orm::make_column("LOGIN", &pineBox::login),
								sqlite_orm::make_column("PASSWORD", &pineBox::password)),
						sqlite_orm::make_table("VALUES",
							sqlite_orm::make_column("TOKEN", &pineBoxVal::token, sqlite_orm::primary_key()),
							sqlite_orm::make_column("VAL", &pineBoxVal::val),
							sqlite_orm::make_column("TYPES", &pineBoxVal::types),
							sqlite_orm::make_column("VALUES", &pineBoxVal::values),
							sqlite_orm::make_column("TIMESTAMPS", &pineBoxVal::timestamps),
							sqlite_orm::make_column("SENT", &pineBoxVal::sent)));
		}

		database() {
			decltype(initDatabase()) storage = initDatabase();
			storage.sync_schema();
		}
		
		database(bool check) {
			
		}

		void drop() {
			std::cout << "Drop database in code!" << std::endl;
			decltype(initDatabase()) storage = initDatabase();
			storage.remove_all<pineBox>();
			storage.remove_all<pineBoxVal>();
			storage.sync_schema();
		}

		void add(int token) {
			std::cout << "Add new elements in database" << std::endl;
			decltype(initDatabase()) storage = initDatabase();
			pineBox newPine{ token, false, "", "", "", "" };
			storage.insert(newPine);
			storage.sync_schema();
		}

		void add(int token, std::string name) {
			std::cout << "Add new elements in database" << std::endl;
			decltype(initDatabase()) storage = initDatabase();
			pineBox newPine{ token, false, name, "", "", "" };
			std::cout << "Create new pine" << std::endl;
			storage.insert(newPine);
			std::cout << "Insert new pine" << std::endl;
			storage.sync_schema();
		}

		void add(int token, std::string name, std::string email) {
			std::cout << "Add new elements in database" << std::endl;
			decltype(initDatabase()) storage = initDatabase();
			pineBox newPine{ token, false, name, email, "", "" };
			storage.insert(newPine);
			storage.sync_schema();
		}

		void add(int token, std::string name, std::string email, std::string login) {
			std::cout << "Add new elements in database" << std::endl;
			decltype(initDatabase()) storage = initDatabase();
			pineBox newPine{ token, false, name, email, login, "" };
			storage.insert(newPine);
			storage.sync_schema();
		}

		void add(int token, std::string name, std::string email, std::string login, std::string password) {
			std::cout << "Add new elements in database" << std::endl;
			decltype(initDatabase()) storage = initDatabase();
			pineBox newPine{ token, false, name, email, login, password };
			storage.insert(newPine);
			storage.sync_schema();
		}

		void add(int token, std::vector<std::vector<std::string> > types, std::vector<std::vector<int> > values, std::vector<std::string> timestamps) {
			std::cout << "Add new elements in database" << std::endl;
			decltype(initDatabase()) storageVal = initDatabase();
			std::string typesStr = "";
			std::string valuesStr = "";
			std::string timestampsStr = "";
			reformat formatter;
			typesStr = formatter.typesToStr(types);
			valuesStr = formatter.valuesToStr(values);
			timestampsStr = formatter.timestampsToStr(timestamps);
			pineBoxVal newPineVal{ token, 0, typesStr, valuesStr, timestampsStr, false };
			storageVal.insert(newPineVal);
			storageVal.sync_schema();
		}

		void addToElement(int token, std::vector<std::vector<std::string> > types, std::vector<std::vector<int> > values, std::vector<std::string> timestamps) {
			std::cout << "Add to element!" << std::endl;
			decltype(initDatabase()) storageVal = initDatabase();
			pineBoxVal oldPineVal = storageVal.get<pineBoxVal>(token);
			reformat formatter;
			std::vector<std::vector<std::string>> oldTypes = formatter.strToTypes(oldPineVal.types);
			std::vector<std::vector<int>> oldValues = formatter.strToValues(oldPineVal.values);
			std::vector<std::string> oldTimestamps = formatter.strToTimestamps(oldPineVal.timestamps);
			for(int i = 0; i < types.size(); i++) {
				oldTypes.push_back(types[i]);
			}
			for(int i = 0; i < values.size(); i++) {
				oldValues.push_back(values[i]);
			}
			for(int i = 0; i < timestamps.size(); i++) {
				oldTimestamps.push_back(timestamps[i]);
			}
			oldPineVal.types = formatter.typesToStr(oldTypes);
			oldPineVal.values = formatter.valuesToStr(oldValues);
			oldPineVal.timestamps = formatter.timestampsToStr(oldTimestamps);
			oldPineVal.sent = false;
			storageVal.update(oldPineVal);
			valDatabaseUpdate = true;
			storageVal.sync_schema();
		}

		void updateItem(int token) {
			std::cout << "Update element in database" << std::endl;
			decltype(initDatabase()) storage = initDatabase();
			pineBox editedPine = storage.get<pineBox>(token);
			editedPine.token = token;
			storage.update(editedPine);
			storage.sync_schema();
		}

		void updateItem(int token, bool regStatus) {
			std::cout << "Update element in database" << std::endl;
			decltype(initDatabase()) storage = initDatabase();
			pineBox editedPine = storage.get<pineBox>(token);
			editedPine.token = token;
			editedPine.regStatus = regStatus;
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

		void updateItemSent(int token, bool sent) {
			std::cout << "Update val element in database" << std::endl;
			decltype(initDatabase()) storage = initDatabase();
			pineBoxVal editedPine = storage.get<pineBoxVal>(token);
			editedPine.sent = sent;
			storage.update(editedPine);
			storage.sync_schema();
		}
		
		void updateItemVal(int token, int val) {
			std::cout << "Update val element in database" << std::endl;
			decltype(initDatabase()) storage = initDatabase();
			pineBoxVal editedPine = storage.get<pineBoxVal>(token);
			editedPine.val = val;
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

		bool updateItemReg(std::string name, std::string email, std::string login, std::string password) {
			std::cout << "Update reg info" << std::endl;
			decltype(initDatabase()) storage = initDatabase();
			pineBox editedPine = givePineFromName(name);
			if((editedPine.login != "") || (editedPine.password != "") || (editedPine.token == -1)) {
				return false;
			}
			editedPine.login = login;
			editedPine.password = password;
			editedPine.email = email;
			storage.update(editedPine);
			storage.sync_schema();
			return true;
		}

		int givePineC() {
			std::cout << "Give count Pine" << std::endl;
			decltype(initDatabase()) storage = initDatabase();
			return storage.get_all<pineBox>().size();
		}

		void getAns() {
			DIR *dir;
			struct dirent *ent;
			if((dir = opendir("ans")) != NULL) {
				while((ent = readdir (dir)) != NULL) {
					std::string name = ent -> d_name;
					if(name[0] == 'a') {
						std::string buff;
						std::ifstream ansIn("ans/"+name);
						std::getline(ansIn, buff);
						std::getline(ansIn, buff);
						reformat formatter;
						std::vector<std::string> names = formatter.split(name, 'a');
						std::cout << buff << std::endl;
						std::vector<std::string> data = formatter.split(buff, ',');
						std::cout << "Size data: " << data.size();
						int token = std::stoi(names[1]);
						std::vector<std::vector<std::string> > types;
						std::vector<std::vector<int> > values;
						std::vector<std::string> timestamps;
						// TODO Refactor to values and types
						/* START TEST CODE */
						std::cout << "Start test code" << std::endl;
						int val = std::stod(data[0])*1000;
						int type = std::stod(data[1]);
						std::cout << "token: " << token << " val: " << val << " type: " << type;
						std::vector<int> valueT;
						std::vector<std::string> typeT;
						std::cout << "Continue 1" << std::endl;
						valueT.push_back(val);
						values.push_back(valueT);
						std::cout << "Continue 2" << std::endl;
						typeT.push_back(std::to_string(type));
						types.push_back(typeT);
						std::string timeStmp = names[0].substr(2, 100);
						std::cout << " timestamp: " << names[0] << std::endl;
						timestamps.push_back(names[0]);
						std::cout << "Finish test code" << std::endl;
						/* END TEST CODE */
						addToElement(token, types, values, timestamps);
					}
				}
				closedir(dir);
			} else {
				std::cout << "Could not open" << std::endl;
			}
		}
		
		pineBox givePine(int token) {
			std::cout << "Give Pine info" << std::endl;
			decltype(initDatabase()) storage = initDatabase();
			return storage.get<pineBox>(token);
		}

		pineBox givePine(std::string loginTemp) {
			using namespace sqlite_orm;
			std::cout << "Give Pine via login" << std::endl;
			decltype(initDatabase()) storage = initDatabase();
			pineBox pine;
			pine.token = -1;
			if(storage.get_all<pineBox>(where(c(&pineBox::login) == loginTemp)).size()>0){
				return storage.get_all<pineBox>(where(c(&pineBox::login) == loginTemp)).front(); 
			} else {
				return pine;
			}
		}
		
		pineBox givePineFromName(std::string name) {
			using namespace sqlite_orm;
			std::cout << "Give Pine via name" << std::endl;
			decltype(initDatabase()) storage = initDatabase();
			pineBox pine;
			pine.token = -1;
			if(storage.get_all<pineBox>(where(c(&pineBox::name) == name)).size()>0){
				return storage.get_all<pineBox>(where(c(&pineBox::name) == name)).front(); 
			} else {
				return pine;
			}
		}

		pineBoxVal givePineVal(int token) {
			std::cout << "Give PineVal via token" << std::endl;
			decltype(initDatabase()) storageVal = initDatabase();
			return storageVal.get<pineBoxVal>(token);
		}
	private:
};

class authCheck {
	public:
		authCheck(std::string login, std::string password) {
			std::cout << "Auth check" << std::endl;
			try{
				database pineBase;
				pineBox currentPine = pineBase.givePine(login);
				std::cout << "Check" << std::endl;
				if(currentPine.token == -1){
					std::cout << "Failed with password" << std::endl;
					isAuth = false;
				} else if(currentPine.password == password) {
					std::cout << "Auth success" << std::endl;
					pineBase.updateItem(currentPine.token, true);	
					currentToken = currentPine.token;				
					isAuth = true;
				} else {
					std::cout << "Failed with password" << std::endl;
					isAuth = false;
				}
			} catch (...) {
			
			}
		}
		
		
		bool getIsAuth() {
			return isAuth;
		}

		int getCurrentToken() {
			return currentToken;
		}
	private:
		bool isAuth;
		int currentToken;
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

		void run(uint16_t port) {
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

		void on_open(connection_hdl handler) {
			{
				std::cout << "Connection open!" << std::endl;
				lock_guard<mutex> guard(action_lock);
				actions.push(action(SUBSCRIBE, handler)); 
			}
			action_cond.notify_one();
		}

		void on_close(connection_hdl handler) {
			{
				std::cout << "Connection close!" << std::endl;
				lock_guard<mutex> guard(action_lock);
				actions.push(action(UNSUBSCRIBE, handler));	
			}
			action_cond.notify_one();
		}

		void on_message(connection_hdl handler, server::message_ptr msg) {
			{
				std::cout << "Receive message" << std::endl;
				lock_guard<mutex> guard(action_lock);
				actions.push(action(MESSAGE, handler, msg));
			}
			action_cond.notify_one();
		}

		void messages_process() {
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
					websocketpp::lib::error_code errCode;
					currentServer.send(action.handler, createAuthReq(), websocketpp::frame::opcode::text, errCode);
				} else if(action.type == UNSUBSCRIBE){ // Connection closed
					std::cout << "CLOSE CONNECTION" << std::endl;
					lock_guard<mutex> guard(connection_lock);
					for(auto itMap = pineHandlers.begin(); itMap != pineHandlers.end(); itMap++) {
						if(connections_equal(itMap -> second, action.handler)) { //action.handler
							std::cout << "true" << std::endl;
							int token = itMap -> first;
							pineHandlers.erase(token);
							database pineBase;
							pineBase.updateItem(token, false);
							pineBase.updateItemVal(token, currentConnections[token]);
							goto exitFromForI; // Very sorry for this
						} else {
							std::cout << "false" << std::endl;
						}
					}
					exitFromForI:
					std::cout << "Con list erase" << std::endl;
					con_list.erase(action.handler);
				} else if(action.type == MESSAGE){ // New message for handle
					std::cout << "NEW MESSAGE" << std::endl;
					lock_guard<mutex> guard(connection_lock);
					msgAnalyzer(action.msg, action.handler);
					websocketpp::lib::error_code errCode;
					currentServer.send(action.handler, msgOut, websocketpp::frame::opcode::text, errCode);
				}
			}
		}

		void messages_out() {
			std::cout << "Start messages out process" << std::endl;
			bool start = true;
			while(true) {
				//database pineBase(true);
				if(pineHandlers.size() > 0 /*&& (valDatabaseUpdate || start)*/) {
					sendData();
					start = false;
					/* For tests 
					for(auto itMap = pineHandlers.begin(); itMap != pineHandlers.end(); itMap++) {
						websocketpp::lib::error_code errCode;
						currentServer.send(itMap -> second, "kek", websocketpp::frame::opcode::text, errCode);
					} */
				}
				sleep(10);
			}
		}

		bool connections_equal(websocketpp::connection_hdl hdl1, websocketpp::connection_hdl hdl2) {
			std::cout << "**Compare with result: ";
			return !hdl1.owner_before(hdl2) && !hdl2.owner_before(hdl1);
		}

		std::string getPkgOut(std::vector<std::string> types, std::vector<int> values, std::string timestamp, int val) {
			json jOut;
			jOut["event"] = "data_m";
			jOut["data"]["types"] = types;
			jOut["data"]["values"] = values;
			jOut["data"]["timestamp"] = timestamp;
			jOut["data"]["val"] = val;
			return jOut.dump();
		}


		void sendData() {
			std::cout << "PineHandlers size: " << pineHandlers.size() << std::endl;
			for(auto itMap = pineHandlers.begin(); itMap != pineHandlers.end(); ++itMap) {
				std::cout << "Send data in map" << std::endl;
				websocketpp::lib::error_code errCode;
				database pineBase;
				reformat formatter;
				pineBox currentBox = pineBase.givePine(itMap -> first);
				pineBoxVal currentValBox = pineBase.givePineVal(currentBox.token);
				std::vector<std::vector<std::string> > types = formatter.strToTypes(currentValBox.types);
				std::vector<std::vector<int> > values = formatter.strToValues(currentValBox.values);
				std::vector<std::string> timestamps = formatter.strToTimestamps(currentValBox.timestamps);
				for(int i = 0; i < types.size(); i++) {
					std::cout << "currentVal fori" << std::endl;
					if(/*!currentValBox.sent &&*/ i >= currentConnections[currentValBox.token]) {
						std::cout << "Send data!" << std::endl;
						std::cout << "Token2: " << currentValBox.token << std::endl;
						int tempToken = currentConnections[currentValBox.token];
						currentConnections[currentValBox.token] = tempToken + 1;
						std::cout << tempToken + 1 << std::endl;
						std::cout << "types size: " << types.size() << " value size: " << values.size() << " timestamps size: " << timestamps.size();
						std::string outData = getPkgOut(types[i], values[i], timestamps[i], currentConnections[currentValBox.token]);
						std::cout << "Out data: " << outData << std::endl;
						currentServer.send(itMap -> second, outData, websocketpp::frame::opcode::text, errCode);
						//pineBase.updateItemSent(currentValBox.token, true);
						//pineBase.updateItemVal(currentValBox.token, ++currentValBox.val);
					}
				}
			}
			valDatabaseUpdate = false;
		}

		void msgAnalyzer(server::message_ptr messageIn, websocketpp::connection_hdl handlerCur) {
			msgIn = messageIn;
			handlerCurrent = handlerCur;
			analyzeMsgIn(handlerCur);
		}

		
		std::string getMsgOut() {
			return msgOut;
		}

		void saveData(std::string data, int token){
			std::string name = "data/"+getTime()+'a'+std::to_string(token)+'a'+".csv";
			std::cout << name << std::endl;
			fOut.open(name);
			fOut << data;
			fOut.close();
		}

		void saveDataArr(std::vector<std::string> data, int token) {
			std::string name = "data/"+getTime()+'a'+std::to_string(token)+'a'+".csv";
			std::cout << name << std::endl;
			fOut.open(name);
			for(auto s:data) {
				fOut << s;
			}
			fOut.close();
		}

		void analyzeMsgIn(websocketpp::connection_hdl handlerCur) {
			json jIn = json::parse(msgIn -> get_payload());
			std::cout << "Analyze Msg input: " << jIn.dump() << std::endl;
			json jOut;
			std::string event = jIn["event"];
			if(event == "auth_resp_m_s") { //Response from Mobile App
				jOut["event"] = "auth_resp_s_m";
				authCheck check(jIn["data"]["login"], jIn["data"]["password"]);
				if(check.getIsAuth()) {
					pineHandlers[check.getCurrentToken()] = handlerCur;
					std::cout << "Token1: " << check.getCurrentToken() << std::endl;
					currentConnections[check.getCurrentToken()] = jIn["data"]["val"];
					std::cout << "Val: " << jIn["data"]["val"] << std::endl;
					jOut["data"]["response"] = "ok";
					con_list.insert(handlerCurrent);					
				} else {
					jOut["data"]["response"] = "fail";
				}
				msgOut = jOut.dump();
			} else if(event == "reg_req_m_s") {
				jOut["event"] = "reg_resp_s_m";
				try {
					database pineBase;
					if(pineBase.updateItemReg(jIn["data"]["name"], jIn["data"]["email"], jIn["data"]["login"], jIn["data"]["password"])) {
						jOut["data"]["response"] = "ok";
					} else {
						jOut["data"]["response"] = "fail";
					}
				} catch (...) {
					jOut["data"]["response"] = "fail";
				}
				msgOut = jOut.dump();
			} else if(event == "data_pi") {
				std::cout << "Give data from Pi" << std::endl;
				saveData(jIn["data"]["values"], jIn["data"]["token"]);
				jOut["event"] = "data_pi_req";
				jOut["data"] = "";
				msgOut = jOut.dump();
				database pineBase;
				pineBase.getAns();
			}
		} 
		
	private:
		typedef std::set<connection_hdl, std::owner_less<connection_hdl> > connection_list;
		server currentServer;
		connection_list con_list;
		mutex action_lock;
		mutex connection_lock;
		std::queue<action> actions;
		std::map<int, websocketpp::connection_hdl> pineHandlers;
		std::map<int, int> currentConnections;
		condition_variable action_cond;
		server::message_ptr msgIn;
		websocketpp::connection_hdl handlerCurrent;
		std::string msgOut;
		std::string createAuthReq() {
			json j;
			j["event"] = "auth_req_s_m";
			j["data"]["info"] = "null";
			return j.dump();
		}
};

int main(int argc, char *argv[]) {
	std::cout << "FOR START DEBUG MODE TYPE DEBUG IN CONSOLE ARGUMENT" << std::endl;
	database pineBase;
	/* DEBUG MODE */
	if(argc > 1){
		std::string in = argv[1];
		if(in == debug) {
		
			//pineBase.getAns();
		
			std::string input = "";
			std::cout << "*******************************" << std::endl;
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
					std::cout << "Type params:" << std::endl;
					std::vector<int> tokens;
					std::vector<std::string> names;
					std::vector<std::string> logins;
					std::vector<std::string> passwords;
					std::vector<std::string> emails;
					std::string tempInput;
					std::cout << "type token: ";
					getline(std::cin, tempInput);
					while (tempInput != end) {
						tokens.push_back(std::stoi(tempInput));
						std::cout << "type name: ";
						getline(std::cin, tempInput);
						names.push_back(tempInput);
						std::cout << "type login: ";
						getline(std::cin, tempInput); 
						logins.push_back(tempInput);
						std::cout << "type password: ";
						getline(std::cin, tempInput);
						passwords.push_back(tempInput);
						std::cout << "type email: ";
						getline(std::cin, tempInput);
						emails.push_back(tempInput);
						std::cout << "For end type END or continue add elements via type token" << std::endl;
						getline(std::cin, tempInput);
					}		
					for(int i = 0; i < tokens.size(); i++){
						std::cout << "Add new element in fori" << std::endl;
						std::vector<std::vector<std::string> > typesFin;
						std::vector<std::vector<int> > valuesFin;
						std::vector<std::string> timestamps;
						pineBase.add(tokens[i], names[i], emails[i], logins[i], passwords[i]);
						pineBase.add(tokens[i], typesFin, valuesFin, timestamps);
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
						pineBoxVal pineVal = pineBase.givePineVal(token);
						std::cout << "***********" << std::endl;
						std::cout << "*Name: " << pine.name << std::endl;
						std::cout << "*Token: " << pine.token << std::endl;
						std::cout << "*Email: " << pine.email << std::endl;
						std::cout << "*Login: " << pine.login << std::endl;
						std::cout << "*Password: " << pine.password << std::endl;
						std::cout << "*Registartion status: " << pine.regStatus << std::endl;
						std::cout << "*TokenCpy: " << pineVal.token << std::endl;
						std::cout << "*Values: " << pineVal.values << std::endl;
						std::cout << "*Val: " << pineVal.val << std::endl;
						std::cout << "*Types: " << pineVal.types << std::endl;
						std::cout << "*Timestamps: " << pineVal.timestamps << std::endl;
						std::cout << "***********" << std::endl;
						getline(std::cin, tempInput);
					}
				} else if(input == giveCount) {
					std::cout << "Give size of database!" << std::endl;
					std::cout << "***********" << std::endl;
					std::cout << "Size: " << pineBase.givePineC() << std::endl;
					std::cout << "***********" << std::endl;
				} else if(input == addData) {
					int token;
					std::string temp;
					std::vector<std::string> types;
					std::vector<int> values;
					std::vector<std::vector<std::string> > typesFin;
					std::vector<std::vector<int> > valuesFin;
					std::vector<std::string> timestamps;
					std::cout << "Add test data to database" << std::endl;
					std::cout << "type token of pinebox: ";
					getline(std::cin, temp);
					token = std::stoi(temp);
					std::cout << "type first type: ";
					getline(std::cin, temp);
					types.push_back(temp);
					std::cout << "type second type: ";
					getline(std::cin, temp);
					types.push_back(temp);
					std::cout << "type first value: ";
					getline(std::cin, temp);
					values.push_back(std::stoi(temp));
					std::cout << "type second value: ";
					getline(std::cin, temp);
					values.push_back(std::stoi(temp));
					temp = getTime();
					timestamps.push_back(temp);
					temp = getTime();
					timestamps.push_back(temp);
					typesFin.push_back(types);
					valuesFin.push_back(values);
					typesFin.push_back(types);
					valuesFin.push_back(values);
					pineBase.addToElement(token, typesFin, valuesFin, timestamps);	
				} else if(input == help) {
					std::cout << "*************************************" << std::endl;
					std::cout << "*Info about all debug command:      *" << std::endl;
					std::cout << "*DEBUG - start debug mode           *" << std::endl;
					std::cout << "*DROP - drop all database on server *" << std::endl;
					std::cout << "*ADD - start add mode               *" << std::endl;
					std::cout << "*ADD_DATA - start add data mode     *" << std::endl;
					std::cout << "*GIVE - start give mode             *" << std::endl;
					std::cout << "*GIVEC - give size of database      *" << std::endl;
					std::cout << "*END - end mode                     *" << std::endl;
					std::cout << "*EXIT - finish debug mode           *" << std::endl;
					std::cout << "*HELP - info about all debug command*" << std::endl;
					std::cout << "*************************************" << std::endl;
				} else {
					std::cout << "Unknown debug command. Type HELP for list of all command" << std::endl;
				}
			}
		}
	}
	/* END DEBUG MODE */
	try{
		pineServer pineServer;
		thread thread(bind(&pineServer::messages_process, &pineServer)), threadOut(bind(&pineServer::messages_out, &pineServer));
		pineServer.run(port);
		thread.join();
		threadOut.join();
	} catch (websocketpp::exception const & e) {
		std::cout << "Error in main: " << e.what() << std::endl;
	}
	return 0; 
}
