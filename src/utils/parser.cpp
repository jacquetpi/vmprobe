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
				}else if(name == "perfhardware"){
					utils::Config::Get().perfEventHardware = convertToList(value);
				}else if(name == "perfhardwarecache"){
					utils::Config::Get().perfEventHardwareCache = convertToList(value);
				}else if(name == "perfsoftware"){
					utils::Config::Get().perfEventSoftware = convertToList(value);
				}else if(name == "counterspercore"){
					utils::Config::Get().countersPerCore = std::stoi(value);
				}else{
					utils::logging::error ("Config parser, unknown option", name);
				}
			}
		}
		else {
			utils::logging::error ("Failed to open config file:", _configfile);
		}
	}

	std::list<std::string> Parser::convertToList(std::string value){
		size_t pos = 0;
		std::string token;
		std::list<std::string> list;
		while ((pos = value.find(',')) != std::string::npos) {
    		token = value.substr(0, pos);
			if(!token.empty())
				list.push_back(token);
    		value.erase(0, pos + 1);
		}
		if(!value.empty())
			list.push_back(value);
		return list;
	}
}