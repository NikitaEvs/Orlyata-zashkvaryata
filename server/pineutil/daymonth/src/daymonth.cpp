#include "server.hpp" 

json newMonthData (json dayData, json monthData) {
	dayData["power"] = dayData["powerScore"].get<int>()/dayData["countMeasures"].get<int>();
	monthData["powerScore"] = monthData["powerScore"].get<int>() + dayData["power"].get<int>();
	monthData["countMeasures"] = monthData["countMeasures"].get<int>() + 1;
	monthData["days"].push_back(dayData);
	return monthData;
}

int main() {
	try {
		/* Init connections pool (using simple init of pinebase for read config file) */
		std::shared_ptr<pinepool> pool(new pinepool); // Pool for connections, each pinebase include shared_ptr of pinepool
		pinebase configPinebase;
		pool.get() -> createPool(connectionsCount, configPinebase.getConnectionCommand());
		
		pinebase pine(pool, configPinebase.getDatasetName());
		
		std::vector<int> tokens = pine.tokenList();
		
		for(auto i : tokens) {
			pinebox pineDay = pine.getPineOnlyDaydata(i);
			pinebox pineMonth = pine.getPineOnlyMonthdata(i);
			if(pineDay.daydata["countMeasures"].get<int>() == 0) {
				return 0;
			}
			pineMonth.monthdata = newMonthData(pineDay.daydata, pineMonth.monthdata);
			string dayTime = pine.getTimeDay();
			string emptyDayData = "\'{\"time\":\" " + dayTime + "\",\"powerScore\":0,\"countMeasures\":0,\"power\":0,\"hours\":[] }\'";
			string commandDay = "UPDATE pinebox SET daydata = " + emptyDayData + " WHERE token = " + std::to_string(i) + ";";
			pine.write(commandDay);
			pine.updatePineMonthdata(pineMonth);
		}

		pool.get() -> closeAllConnections();
		
	} catch (const std::exception &e) {
		std::cerr << e.what() << std::endl;
	}
	return 0;
}
