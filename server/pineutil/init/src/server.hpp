#ifndef SERVER_H_
#define SERVER_H_

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

#include "pinebox.hpp"
#include "pinedb.hpp"

#endif /* SERVER_H_ */
