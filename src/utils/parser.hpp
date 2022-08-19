#pragma once

#include <string>
#include <bits/stdc++.h>

namespace utils {

	class Parser {
		
		std::string _configfile;

		std::list<std::string> convertToList(std::string);

		public:

		Parser (std::string configfile);
		
		void parse();
	};

}