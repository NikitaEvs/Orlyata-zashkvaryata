/* _____  _                                    */
/*|  _  \(_)                            _      */
/*| |_) | _ ____  ___  __ _ ____  ____ | | ___ */
/*| ____/| |  _ \/ _ \/ _` |  _ \|  _ \| |/ _ \*/
/*| |    | | | |   __/ (_| | |_) | |_) | |  __/*/
/*|_|    |_|_| |_\___|\__,_| ___/| ___/|_|\___|*/
/*                         | |   | |           */
/*                         |_|   |_|           */

#ifndef PINEBOX_H_
#define PINEBOX_H_

#include "server.hpp"


class pinebox {
	public:
		int token;
		string name = "";
		string email = "";
		string login = "";
		string pass = "";
		bool loginstatus;
		json lastdata;
		json hourdata;
		json daydata;
		json weekdata;
		json monthdata;
		
		pinebox(int c_token, string c_name, string c_email, string c_login, string c_pass, bool c_loginStatus, json c_lastData, json c_hourData, json c_dayData, json c_weekData, json c_monthData) {
			token = c_token;
			name = c_name;
			email = c_email;
			login = c_login;
			pass = c_pass;
			loginstatus = c_loginStatus;
			lastdata = c_lastData;
			hourdata = c_hourData;
			daydata = c_dayData;
			weekdata = c_weekData;
			monthdata = c_monthData;
		}
		
		pinebox(int c_token, string c_name, string c_login, string c_pass) {
			token = c_token;
			name = c_name;
			login = c_login;
			pass = c_pass;
		}
		
		pinebox(int c_token, json c_lastdata) {
			token = c_token;
			lastdata = c_lastdata;
		}
		
		pinebox(int c_token, json c_hourdata, bool salt) {
			token = c_token;
			hourdata = c_hourdata;
		} 
		
		pinebox(int c_token, json c_daydata, bool salt, bool salt2) {
			token = c_token;
			daydata = c_daydata;
		} 
		
		pinebox(int c_token, json c_monthdata, bool salt, bool salt2, bool salt3) {
			token = c_token;
			monthdata = c_monthdata;
		} 
		
	private:
	
};

#endif /* PINEBOX_H_ */

