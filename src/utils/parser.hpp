#pragma once

#include <string>

namespace utils {

	class Parser {
		
		std::string _configfile;

		public:

		Parser (std::string configfile);
		
		void parse();
	};

}