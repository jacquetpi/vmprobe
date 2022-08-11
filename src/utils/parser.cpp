#include <iostream>
#include <fstream>
#include <algorithm>
#include "parser.hpp"
#include "config.hpp"
#include "log.hpp"

namespace utils {

	Parser::Parser(std::string configfile) : _configfile(configfile) {};

	void Parser::parse(){
		utils::logging::info ("Loading configuration from file", _configfile);
		std::ifstream cFile (_configfile);
		if (cFile.is_open())
		{
			std::string line;
			while(getline(cFile, line)){
				line.erase(std::remove_if(line.begin(), line.end(), isspace),
									line.end());
				if(line[0] == '#' || line.empty())
					continue;
				auto delimiterPos = line.find("=");
				auto name = line.substr(0, delimiterPos);
				auto value = line.substr(delimiterPos + 1);
				//utils::logging::info ("Parser", name, value);
				if(name == "prefix"){
					utils::Config::Get().prefix = value;
				}else if(name == "delay"){
					utils::Config::Get().delay = std::stoi(value);
				}else if(name == "endpoint"){
					utils::Config::Get().endpoint = value;
				}else if(name == "url"){
					utils::Config::Get().url = value;
				}else{
					utils::logging::error ("Config parser, unknown option", name);
				}

			}
		}
		else {
			utils::logging::error ("Failed to open config file:", _configfile);
		}
	}
}