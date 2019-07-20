#include "server.hpp"

bool createMode = false;;
string nameOfLoad;
string nameOfDataset;

int main(int argc, char *argv[]) {
	if(argc > 1) {
		string in = argv[1];
		if(in == "CREATE") {
			if(argc > 3) {
				nameOfDataset = argv[2];
				nameOfLoad = argv[3];
				createMode = true;
			}
		}
	}
	try {
		/* Init connections pool (using simple init of pinebase for read config file) */
		std::shared_ptr<pinepool> pool(new pinepool); // Pool for connections, each pinebase include shared_ptr of pinepool
		pinebase configPinebase;
		pool.get() -> createPool(connectionsCount, configPinebase.getConnectionCommand());
		
		pinebase pine(pool, configPinebase.getDatasetName());
		
		/* REGISTRATION SAMPLE CODE 
		std::cout << "Reg result: " << pine.reg("test", "cat", "meow") << std::endl;;
		
		/* LOGIN SAMPLE CODE 
		pinebox pineboxLog = pine.getPineViaRegData("cat", "meow");
		if(pineboxLog.token != -1) {
			std::cout << "Login success! " << "Last data: " << std::endl;
			std::cout << pineboxLog.lastdata.dump() << std::endl;
		} else {
			std::cout << "Login failed!" << std::endl;
		}
		
		/* SAVE DATA FROM PI TO DATA DIR AND GET ANS FROM NETWROK SAMPLE CODE 
		string data = "1,2,3,4,5,6,7,8,9,10,11,12";
		pine.saveToCSV(1, data);
		
		/* READ DATASET SAMPLE CODE 
		std::cout << pine.getDatasetConfig().dump() << std::endl;
		*/
		
		//pine.getAnsFromCSV();
		
		try{
			pineServer pineServer;
			if(createMode) {
				std::cout << "START CREATE MODE" << std::endl;
				pineServer.configureCreateMode(nameOfLoad, nameOfDataset);
			}
			thread thread(bind(&pineServer::messages_process, &pineServer));
			pineServer.run(configPinebase.getPort());
			thread.join();
		} catch (websocketpp::exception const & e) {
			std::cout << "Error in main: " << e.what() << std::endl;
		}
		
		/* TEST MULTITHREADING */ /*
		for(int i = 0; i < 10; i++) {
			std::shared_ptr<pinebase> testPinebase(new pinebase(pool));
			std::thread thread(checkMultithreading, testPinebase);
			thread.join();
		} */
		
		//pool.get() -> closeAllConnections();
		
	} catch (const std::exception &e) {
		std::cerr << e.what() << std::endl;
	}
	return 0;
}
