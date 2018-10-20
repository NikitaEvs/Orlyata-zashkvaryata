#include "server.hpp" 

int main() {
	try {
		/* Init connections pool (using simple init of pinebase for read config file) */
		std::shared_ptr<pinepool> pool(new pinepool); // Pool for connections, each pinebase include shared_ptr of pinepool
		pinebase configPinebase;
		pool.get() -> createPool(connectionsCount, configPinebase.getConnectionCommand());
		
		pinebase pine(pool, configPinebase.getDatasetName());
		
		string hourTime = pine.getTimeHour();
		string dayTime = pine.getTimeDay();
		string monthTime = pine.getTimeMonth();
		
		string emptyHourData = "\'{\"time\":\" "+ hourTime +"\",\"powerScore\":0,\"countMeasures\":0,\"power\":0,\"types\":[],\"values\":[] }\'";
		string emptyDayData = "\'{\"time\":\" " + dayTime + "\",\"powerScore\":0,\"countMeasures\":0,\"power\":0,\"hours\":[] }\'";
		string emptyMonthData = "\'{\"time\":\" " + monthTime + "\",\"powerScore\":0,\"countMeasures\":0,\"power\":0,\"days\":[] }\'";
		
		string commandHour = "UPDATE pinebox SET hourdata = " + emptyHourData + " WHERE token = 1;";
		string commandDay = "UPDATE pinebox SET daydata = " + emptyDayData + " WHERE token = 1;";
		string commandMonth = "UPDATE pinebox SET monthdata = " + emptyMonthData + " WHERE token = 1;";
		
		pine.write(commandHour);
		pine.write(commandDay);
		pine.write(commandMonth);
		

		pool.get() -> closeAllConnections();
		
	} catch (const std::exception &e) {
		std::cerr << e.what() << std::endl;
	}
	return 0;
}
