#pragma once

#include <string>
#include <bits/stdc++.h>

namespace utils {
	
	class Config{ //singleton

		private:
		Config() = default;

		public:
		static Config& Get(){ static Config instance; return instance;}
		Config(const Config&) = delete;
    	void operator=(const Config&) = delete;

		std::string prefix;
		int delay;
		std::string endpoint;
		std::string url;
		std::list<std::string> perfEventHardware;
		std::list<std::string> perfEventSoftware;
		std::list<std::string> perfEventHardwareCache;
		std::list<std::string> perfEventTracepoint;
	};

}