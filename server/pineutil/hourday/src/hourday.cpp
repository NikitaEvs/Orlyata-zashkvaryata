#include "server.hpp" 

json newDayData (json hourData, json dayData) {
	std::vector<int> newVal;
	std::vector<int> oldVal = hourData["values"];
	int ind = hourData["countMeasures"].get<int>();
	for(int i : oldVal) {
		newVal.push_back((int)(i/ind));
	}
	hourData["values"] = newVal;
	dayData["hours"].push_back(hourData);
	dayData["powerScore"] = dayData["powerScore"].get<int>() + hourData["power"].get<int>();
	dayData["countMeasures"] = dayData["countMeasures"].get<int>() + 1;
	return dayData;
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
			pinebox pineHour = pine.getPineOnlyHourdata(i);
			pinebox pineDay = pine.getPineOnlyDaydata(i);
			if(pineHour.hourdata["countMeasures"].get<int>() == 0) {
				return 0;
			}
			pineDay.daydata = newDayData(pineHour.hourdata, pineDay.daydata);
			string hourTime = pine.getTimeHour();
			string emptyHourData = "\'{\"time\":\" "+ hourTime +"\",\"powerScore\":0,\"countMeasures\":0,\"power\":0,\"types\":[],\"values\":[] }\'";
			string commandHour = "UPDATE pinebox SET hourdata = " + emptyHourData + " WHERE token = " + std::to_string(i) + ";";
			pine.write(commandHour);
			pine.updatePineDaydata(pineDay);
		}

		pool.get() -> closeAllConnections();
		
	} catch (const std::exception &e) {
		std::cerr << e.what() << std::endl;
	}
	return 0;
}
