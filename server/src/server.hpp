#ifndef SERVER_H_
#define SERVER_H_

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/common/thread.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include "json/json.hpp"
#include <pqxx/pqxx>
#include <memory>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <ctime>
#include <unistd.h>
#include <dirent.h>

#include <thread>

using json = nlohmann::json;
using string = std::string;

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

#include "pinebox.hpp"
#include "pinedb.hpp"
#include "pineserver.hpp"

#endif /* SERVER_H_ */
