#include "log.hpp"
#include <iostream>

namespace utils {
	
	namespace logging {

	    utils::mutex __mutex__;
	    
	    void content_print () {}
	    
	    std::string get_time () {		
			time_t now;
			time(&now);
			char buf[sizeof "2011-10-08 07:07:09"];
			strftime(buf, sizeof buf, "%F %T", gmtime(&now));

			return std::string (buf);
	    }

	    std::string get_time_no_space () {		
			time_t now;
			time(&now);
			char buf[sizeof "2011-10-08_07:07:09"];
			strftime(buf, sizeof buf, "%F_%T", gmtime(&now));

			return std::string (buf);
	    }

	}
}