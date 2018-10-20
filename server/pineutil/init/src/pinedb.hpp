/* _____  _                                    */
/*|  _  \(_)                            _      */
/*| |_) | _ ____  ___  __ _ ____  ____ | | ___ */
/*| ____/| |  _ \/ _ \/ _` |  _ \|  _ \| |/ _ \*/
/*| |    | | | |   __/ (_| | |_) | |_) | |  __/*/
/*|_|    |_|_| |_\___|\__,_| ___/| ___/|_|\___|*/
/*                         | |   | |           */
/*                         |_|   |_|           */

#ifndef PINEDB_H_
#define PINEDB_H_

/* Rows index from database */
#define TOKEN 0
#define NAME 1
#define EMAIL 2
#define LOGIN 3
#define PASS 4
#define LOGINSTATUS 5
#define LASTDATA 6
#define HOURDATA 7
#define DAYDATA 8
#define WEEKDATA 9
#define MONTHDATA 10

#include "server.hpp"

/* Global variable */
const int connectionsCount = 10; // Count of connections in pool

/* Connection pool for pinebase class */
class pinepool {
	public:
		void createPool(int currentCount, string currentConnectionCommand) {
			countOfConnections = currentCount;
			connectionCommand = currentConnectionCommand;
			std::lock_guard<std::mutex> locker(mutex);
			/* Add new connections in pool */
			for(int i = 0; i < countOfConnections; i++) {
				pool.emplace(std::make_shared<pqxx::connection>(connectionCommand)); 
			}
		}
		
		/* Get free connection from pool */
		std::shared_ptr<pqxx::connection> getConnection() {
			std::unique_lock<std::mutex> lock(mutex);
			/* Wait for free connection */
			while(pool.empty()) {
				condition.wait(lock);
			}	
			std::shared_ptr<pqxx::connection> connection = pool.front();
			pool.pop();
			return connection;
		}
		
		/* Return connection in pool */
		void returnConnection(std::shared_ptr<pqxx::connection> connection) {
			std::unique_lock<std::mutex> lock(mutex);
			pool.push(connection);
			lock.unlock();
			condition.notify_one();
		}
		
		void closeAllConnections() {
			while(!pool.empty()){
				auto conn = pool.front();
				conn.get() -> disconnect();
				pool.pop();
			}
		}
	private:
		int countOfConnections;
		string connectionCommand;
		
		std::queue<std::shared_ptr<pqxx::connection> > pool;
		std::mutex mutex;
		std::condition_variable condition;
};

/* Command for using in sql */
string DROP = "DROP TABLE pinebox";
string SELECT_WITH_NAME = "SELECT * FROM pinebox WHERE name LIKE \'";
string SELECT_WITH_NAME_ONLY_REG = "SELECT token, name, login, pass FROM pinebox WHERE name LIKE \'";
string SELECT_WITH_LOGIN_ONLY_REG = "SELECT token, name, login, pass FROM pinebox WHERE login LIKE \'";
string SELECT_WITH_LOGIN = "SELECT * FROM pinebox WHERE login LIKE \'";
string SELECT_WITH_TOKEN = "SELECT * FROM pinebox WHERE token = ";
string SELECT_WITH_TOKEN_ONLY_LAST_DATA = "SELECT token, lastdata FROM pinebox WHERE token = ";
string SELECT_WITH_TOKEN_ONLY_HOUR_DATA = "SELECT token, hourdata FROM pinebox WHERE token = ";
string SELECT_WITH_TOKEN_ONLY_DAY_DATA = "SELECT token, daydata FROM pinebox WHERE token = ";
string SELECT_WITH_TOKEN_ONLY_MONTH_DATA = "SELECT token, monthdata FROM pinebox WHERE token = ";
string SELECT_TOKENS = "SELECT token FROM pinebox";
string UPDATE_PINE = "UPDATE pinebox SET email = ";
string UPDATE_PINE_ONLY_REG = "UPDATE pinebox SET login = ";
string UPDATE_PINE_ONLY_LAST_DATA = "UPDATE pinebox SET	lastdata = ";
string UPDATE_PINE_ONLY_HOUR_DATA = "UPDATE pinebox SET hourdata = ";
string UPDATE_PINE_ONLY_DAY_DATA = "UPDATE pinebox SET daydata = ";
string UPDATE_PINE_ONLY_MONTH_DATA = "UPDATE pinebox SET monthdata = ";


/* Interacting with PostgreSQL database */
class pinebase {
	public:
		/* Read and parse configure file for database, use it in advance */
		void readConfigureFile() {
			/* Open configure file */
			string dbParamStr;
			std::ifstream configFile("config/database.json", std::ios::binary);
			configFile.seekg(0, std::ios_base::end);
			dbParamStr.resize(configFile.tellg());
			configFile.seekg(0, std::ios_base::beg);
			configFile.read((char*)dbParamStr.data(), (int)dbParamStr.size());
			json dbParam = json::parse(dbParamStr);
			dbname = dbParam["param"]["dbname"];
			user = dbParam["param"]["user"];
			password = dbParam["param"]["password"];
			hostaddr = dbParam["param"]["hostaddr"];
			port = dbParam["param"]["port"];
			dataset = dbParam["param"]["dataset"];
			configFile.close();
		}
		
		/* Return connection command for database */
		string getConnectionCommand() {
			return "dbname = " + dbname + " user = " + user + " password = " + password + " hostaddr = " + hostaddr + " port = " + port;
		}
		
		/* Return string with server time */
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
		
		/* Return string with server time with hour pattern */
		std::string getTimeHour() {
			tm * timeinfo;
			time_t rawtime;
			char timebuff[120] = {0};
			time(&rawtime);
			timeinfo = localtime(&rawtime);
			if (strftime(timebuff, 120, "%d.%m.%Y %H", timeinfo) == 0) {
				return "err";
			} else {
				return std::string(timebuff);
			}
		}
		
		/* Return string with server time with day pattern */
		std::string getTimeDay() {
			tm * timeinfo;
			time_t rawtime;
			char timebuff[120] = {0};
			time(&rawtime);
			timeinfo = localtime(&rawtime);
			if (strftime(timebuff, 120, "%d.%m.%Y", timeinfo) == 0) {
				return "err";
			} else {
				return std::string(timebuff);
			}
		}
		
		/* Return string with server time with month pattern */
		std::string getTimeMonth() {
			tm * timeinfo;
			time_t rawtime;
			char timebuff[120] = {0};
			time(&rawtime);
			timeinfo = localtime(&rawtime);
			if (strftime(timebuff, 120, "%m.%Y", timeinfo) == 0) {
				return "err";
			} else {
				return std::string(timebuff);
			}
		}
		
		
		/* Simple initialization for read configure file, not working for anything else! */
		pinebase() {
			readConfigureFile();
		}
		
		/* Main initialization */
		pinebase(std::shared_ptr<pinepool> currentPinepoolPtr) {
			pinepoolPtr = currentPinepoolPtr;
		}
		
		/* Init with setting dataset */
		pinebase(std::shared_ptr<pinepool> currentPinepoolPtr, string currentDataset) {
			pinepoolPtr = currentPinepoolPtr;
			dataset = currentDataset;
		}
		
		/* Get dataset name (useful for config pine database) */
		string getDatasetName() {
			return dataset;
		}
		
		/* Test open attempt, return true in success */
		bool checkConnection() {
			auto connection = pinepoolPtr -> getConnection();
			if(connection.get() -> is_open()) {
				std::cout << "Database open successfull: " << connection.get() -> dbname() << std::endl;
				pinepoolPtr -> returnConnection(connection);
				return true;
			} else {
				std::cout << "Error in opening" << std::endl;
				pinepoolPtr -> returnConnection(connection);
				return false;
			}
		}
		
		/* Drop table in code (useless function) */
		void drop() {
			auto connection = pinepoolPtr -> getConnection();
			
			pqxx::work work(*connection);
			work.exec(DROP);
			work.commit();
			
			pinepoolPtr -> returnConnection(connection);
		}
		
		/* Execute some command in PostgreSQL database */
		void write(string command) {
			auto connection = pinepoolPtr -> getConnection();
			
			pqxx::work work(*connection);
			work.exec(command);
			work.commit();
			
			pinepoolPtr -> returnConnection(connection);
		}
		
		/* Execute SELECT operation and return result */
		pqxx::result read(string command) {
			auto connection = pinepoolPtr -> getConnection();
			
			pqxx::nontransaction non(*connection);
			pqxx::result res(non.exec(command));
			
			pinepoolPtr -> returnConnection(connection);
			return res;
		}
		
		/* Next functions give access to the information in database using different way */
		/* Functions return pinebox object with impossible token = -1 if pinebox with current conditions does not exist */
		
		pinebox getPine(int token) {
			string command = SELECT_WITH_TOKEN + std::to_string(token); + ";";
			pqxx::result res = read(command);
			if (res.size() == 0) {
				pinebox pinebox(-1, "", "", "");
				return pinebox;
			}
			for(pqxx::result::const_iterator c = res.begin(); c != res.end(); ++c) {
				json lastdata = json::parse(c[6].as<string>());
				json hourdata = json::parse(c[7].as<string>());
				json daydata = json::parse(c[8].as<string>());
				json weekdata = json::parse(c[9].as<string>());
				json monthdata = json::parse(c[10].as<string>());
				pinebox pinebox(c[0].as<int>(), c[1].as<string>(), c[2].as<string>(), c[3].as<string>(), c[4].as<string>(), c[5].as<bool>(), lastdata, hourdata, daydata, weekdata, monthdata);
				return pinebox;
			}
		}
		
		pinebox getPine(string name) {
			string command = SELECT_WITH_NAME + name; + "';";
			pqxx::result res = read(command);
			if (res.size() == 0) {
				pinebox pinebox(-1, "", "", "");
				return pinebox;
			}
			for(pqxx::result::const_iterator c = res.begin(); c != res.end(); ++c) {
				json lastdata = json::parse(c[6].as<string>());
				json hourdata = json::parse(c[7].as<string>());
				json daydata = json::parse(c[8].as<string>());
				json weekdata = json::parse(c[9].as<string>());
				json monthdata = json::parse(c[10].as<string>());
				pinebox pinebox(c[0].as<int>(), c[1].as<string>(), c[2].as<string>(), c[3].as<string>(), c[4].as<string>(), c[5].as<bool>(), lastdata, hourdata, daydata, weekdata, monthdata);
				return pinebox;
			}
		}
		
		pinebox getPineViaLogin(string login) {
			string command = SELECT_WITH_LOGIN + login + "';";
			pqxx::result res = read(command);
			if (res.size() == 0) {
				pinebox pinebox(-1, "", "", "");
				return pinebox;
			}
			for(pqxx::result::const_iterator c = res.begin(); c != res.end(); ++c) {
				json lastdata = json::parse(c[6].as<string>());
				json hourdata = json::parse(c[7].as<string>());
				json daydata = json::parse(c[8].as<string>());
				json weekdata = json::parse(c[9].as<string>());
				json monthdata = json::parse(c[10].as<string>());
				pinebox pinebox(c[0].as<int>(), c[1].as<string>(), c[2].as<string>(), c[3].as<string>(), c[4].as<string>(), c[5].as<bool>(), lastdata, hourdata, daydata, weekdata, monthdata);
				return pinebox;
			}
		}
		
		pinebox getPineViaRegData(string login, string pass) {
			pinebox pineboxRes = getPineViaLogin(login);
			if(pineboxRes.token == -1) {
				return pineboxRes;
			}
			if(pineboxRes.pass != pass) {
				pinebox pinebox(-1, "", "", "");
				return pinebox;
			}
			return pineboxRes;
		}
		
		pinebox getPineOnlyReg(string name) {
			string command = SELECT_WITH_NAME_ONLY_REG + name + "\';";
			pqxx::result res = read(command);
			if (res.size() == 0) {
				pinebox pinebox(-1, "", "", "");
				return pinebox;
			}
			for(pqxx::result::const_iterator c = res.begin(); c != res.end(); ++c) {
				pinebox pinebox(c[0].as<int>(), c[1].as<string>(), c[2].as<string>(), c[3].as<string>());
				return pinebox;
			}
		}
		
		pinebox getPineOnlyRegViaLogin(string login) {
			string command = SELECT_WITH_LOGIN_ONLY_REG + login + "';";
			pqxx::result res = read(command);
			if (res.size() == 0) {
				pinebox pinebox(-1, "", "", "");
				return pinebox;
			}
			for(pqxx::result::const_iterator c = res.begin(); c != res.end(); ++c) {
				pinebox pinebox(c[0].as<int>(), c[1].as<string>(), c[2].as<string>(), c[3].as<string>());
				return pinebox;
			}
		}
		
		pinebox getPineOnlyLastdata(int token) {
			string command = SELECT_WITH_TOKEN_ONLY_LAST_DATA + std::to_string(token) + ";";
			pqxx::result res = read(command);
			if (res.size() == 0) {
				pinebox pinebox(-1, "", "", "");
				return pinebox;
			}
			for(pqxx::result::const_iterator c = res.begin(); c != res.end(); ++c) {
				json lastdata = json::parse(c[1].as<string>());
				pinebox pinebox(c[0].as<int>(), lastdata);
				return pinebox;
			}
		}
		
		pinebox getPineOnlyHourdata(int token) {
			string command = SELECT_WITH_TOKEN_ONLY_HOUR_DATA + std::to_string(token) + ";";
			pqxx::result res = read(command);
			if (res.size() == 0) {
				pinebox pinebox(-1, "", "", "");
				return pinebox;
			}
			for(pqxx::result::const_iterator c = res.begin(); c != res.end(); ++c) {
				json lastdata = json::parse(c[1].as<string>());
				pinebox pinebox(c[0].as<int>(), lastdata, true);
				return pinebox;
			}
		}
		
		pinebox getPineOnlyDaydata(int token) {
			string command = SELECT_WITH_TOKEN_ONLY_DAY_DATA + std::to_string(token) + ";";
			pqxx::result res = read(command);
			if (res.size() == 0) {
				pinebox pinebox(-1, "", "", "");
				return pinebox;
			}
			for(pqxx::result::const_iterator c = res.begin(); c != res.end(); ++c) {
				json lastdata = json::parse(c[1].as<string>());
				pinebox pinebox(c[0].as<int>(), lastdata, true, true);
				return pinebox;
			}
		}
		
		pinebox getPineOnlyMonthdata(int token) {
			string command = SELECT_WITH_TOKEN_ONLY_MONTH_DATA + std::to_string(token) + ";";
			pqxx::result res = read(command);
			if (res.size() == 0) {
				pinebox pinebox(-1, "", "", "");
				return pinebox;
			}
			for(pqxx::result::const_iterator c = res.begin(); c != res.end(); ++c) {
				json lastdata = json::parse(c[1].as<string>());
				pinebox pinebox(c[0].as<int>(), lastdata, true, true, true);
				return pinebox;
			}
		}
		
		/* Return all tokens in database */
		std::vector<int> tokenList() {
			std::vector<int> result;
			string command = SELECT_TOKENS;
			pqxx::result res = read(command);
			for(pqxx::result::const_iterator c = res.begin(); c != res.end(); ++c) {
				result.push_back(c[0].as<int>());
			}
			return result;
		}
		
		/* Next functions update info in database */
		
		void updatePine(pinebox pinebox) {
			string command = UPDATE_PINE + pinebox.email + ", login = " + "\'" + pinebox.login + "\'" + ", pass = " + "\'" + pinebox.pass + "\'" + ", loginstatus = " + "\'" + std::to_string(pinebox.loginstatus) + "\'" + ", lastdata = " + "\'" + pinebox.lastdata.dump() +  "\'" + ", hourdata = " + "\'" + pinebox.hourdata.dump() + "\'" + ", daydata = " + "\'" + pinebox.daydata.dump() + "\'" + ", weekdata = " + "\'" + pinebox.weekdata.dump() + "\'" + ", monthdata = " + "\'" + pinebox.monthdata.dump() + "\'" + " WHERE token = " + std::to_string(pinebox.token) + ";";
			write(command);
		}
		
		void updateRegInfo(pinebox pinebox) {
			string command = UPDATE_PINE_ONLY_REG + "\'" + pinebox.login + "\'" + ", pass = " + "\'" + pinebox.pass + "\'" + " WHERE token = " + std::to_string(pinebox.token) + ";";
			write(command);
		}
		
		void updatePineLastdata(pinebox pinebox) {
			string command = UPDATE_PINE_ONLY_LAST_DATA + "\'" + pinebox.lastdata.dump() + "\'" + " WHERE token = " + std::to_string(pinebox.token) + ";";
			write(command);
		}
		
		void updatePineHourdata(pinebox pinebox) {
			string command = UPDATE_PINE_ONLY_HOUR_DATA + "\'" + pinebox.hourdata.dump() + "\'" + " WHERE token = " + std::to_string(pinebox.token) + ";";
			write(command);
		}
		
		void updatePineDaydata(pinebox pinebox) {
			string command = UPDATE_PINE_ONLY_DAY_DATA + "\'" + pinebox.daydata.dump() + "\'" + " WHERE token = " + std::to_string(pinebox.token) + ";";
			write(command);
		}
		
		void updatePineMonthdata(pinebox pinebox) {
			string command = UPDATE_PINE_ONLY_MONTH_DATA + "\'" + pinebox.monthdata.dump() + "\'" + " WHERE token = " + std::to_string(pinebox.token) + ";";
			write(command);
		}
		
		void updatePineDayHourdata(pinebox pinebox) {
			updatePineHourdata(pinebox);
			updatePineDaydata(pinebox);
		}
		
		void updatePineMonthDaydata(pinebox pinebox) {
			updatePineDaydata(pinebox);
			updatePineMonthdata(pinebox);
		}
		
		/* Register new account, return true in success */
		bool reg(string name, string login, string pass) {
			pinebox pinebox = getPineOnlyReg(name);
			if(pinebox.token == -1) {
				return false;
			}
			if(pinebox.login != "") {
				return false;
			}
			if(!checkLoginValid(login)) {
				return false;
			}
			pinebox.login = login;
			pinebox.pass = pass;
			updateRegInfo(pinebox);
			return true;
		}
		
		/* Check login, if valid return true */
		bool checkLoginValid(string login) {
			/* Custom conditions for login */
			
			pinebox pinebox = getPineOnlyRegViaLogin(login);
			if(pinebox.token != -1) {
				return false;
			}
			return true;
		}
		
		/* Next functions for updating database in-realtime from client */
		void updateRealtime(int token, json data) {
			pinebox pinebox(token, data);
			updatePineLastdata(pinebox);
			updateHourData(token, data);
		}
		
		void updateRealtime(int token, string data) {
			updateRealtime(token, json::parse(data));
			updateHourData(token, json::parse(data));
		}
		
		/* Updating hour data in database with new realtime-data */
		void updateHourData(int token, json lastdata) {
			pinebox pineboxHour = getPineOnlyHourdata(token);
			if(pineboxHour.token == -1) {
				return;
			}
			json lastHourdata = pineboxHour.hourdata;
			json result = lastHourdata;
			result["powerScore"] = result["powerScore"].get<int>() + lastdata["value"].get<int>();
			result["countMeasures"] = result["countMeasures"].get<int>() + 1;
			result["power"] = (double)result["powerScore"].get<int>()/(double)result["countMeasures"].get<int>();
			std::vector<string> typesLastdata = lastdata["types"];
			std::vector<string> typesHourdata = lastHourdata["types"];
			std::vector<int> valuesLastdata = lastdata["values"];
			std::vector<int> valuesHourdata = lastHourdata["values"];;
			
			for(int i = 0; i < typesLastdata.size(); i++) {
				std::cout << "Checkpoint2" << std::endl;
				string type = typesLastdata[i];
				if(std::find(typesHourdata.begin(), typesHourdata.end(), type) == typesHourdata.end()) {
					typesHourdata.push_back(type);
					valuesHourdata.push_back(0);
				}
			}
			for(int i = 0; i < valuesLastdata.size(); i++) {
				int index = std::find(typesHourdata.begin(), typesHourdata.end(), typesLastdata[i]) - typesHourdata.begin();
				if(index == typesHourdata.size()) {
					typesHourdata.push_back(typesLastdata[i]);
					valuesHourdata.push_back(valuesLastdata[i]);
				} else {
					valuesHourdata[index] += valuesLastdata[i];
				}
			}
			result["values"] = valuesHourdata;
			result["types"] = typesHourdata;
			pineboxHour.hourdata = result;
			updatePineHourdata(pineboxHour);
		}
		
		/* Util function for split string with char */
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
		
		/* Util function for save data string in csv file */
		void saveToCSV(int token, string data) {
			string hat = "1,2,3,4,5,6,7,8,9,10,11,12";
			string resultData = hat + '\n' + data;
			std::ofstream dataFile;
			string name = "data/"+getTime()+'t'+std::to_string(token)+'t'+".csv";
			dataFile.open(name);
			dataFile << resultData;
			dataFile.close();
		}
		
		/* Util function for read dataset config file */
		json getDatasetConfig() {
			string buff;
			std::ifstream datasetFile("config/" + dataset + ".json", std::ios::binary);
			datasetFile.seekg(0, std::ios_base::end);
			buff.resize(datasetFile.tellg());
			datasetFile.seekg(0, std::ios_base::beg);
			datasetFile.read((char*)buff.data(), (int)buff.size());
			return json::parse(buff);
		}		
		
		/* Function for get ans from netwrok and add ans in database */
		void getAnsFromCSV() {
			DIR *dir;
			struct dirent *ent;
			if((dir = opendir("ans")) != NULL) {
				while((ent = readdir (dir)) != NULL) {
					std::string name = ent -> d_name;
					if(name[0] == 'a') {
						std::string buff;
						std::ifstream ansFile("ans/"+name);
						std::getline(ansFile, buff);
						std::getline(ansFile, buff);
						std::vector<std::string> names = split(name, 't');
						std::vector<std::string> data = split(buff, ',');
						int token = std::stoi(names[1]);
						int val = std::stod(data[0])*1000;
						int type = std::stoi(data[1]);
						std::cout << type << std::endl;
						json keys = getDatasetConfig();
						json lastdata;
						lastdata["time"] = getTime();
						std::vector<string> types = keys[std::to_string(type)]["types"];
						std::vector<string> valuesPattern = keys[std::to_string(type)]["values"];
						std::vector<int> values;
						for(auto d : valuesPattern) {
							values.push_back((int)(std::stod(d)*val));
						}
						lastdata["types"] = types;
						lastdata["values"] = values;
						lastdata["value"] = val;
						updateRealtime(token, lastdata);
					}
				}
			}
		}
		
		
	
	private:
		string dbname, user, password, hostaddr, port, dataset;
		std::shared_ptr<pinepool> pinepoolPtr;
};


/* Function for test multithreading */
void checkMultithreading (std::shared_ptr<pinebase> pine) {
	while(true) {
		pine.get() -> checkConnection();
	}
}

#endif /* PINEDB_H_ */
