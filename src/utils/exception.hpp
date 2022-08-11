#pragma once

#include <string>
#include <iostream>

namespace utils {
	
	struct exception {	    
	    std::string msg;
	    
	    exception (const std::string & msg);

	    void print () const;
	    
	};

	struct file_error : public exception {

	    file_error (const std::string & msg) : exception (msg) {}
	    
	};

	struct command_line_error : public exception {
	    command_line_error (const std::string & msg) : exception (msg) {}
	};

	struct addr_error : public exception {
	    addr_error (const std::string & msg) : exception (msg) {}	    
	};

}


std::ostream & operator << (std::ostream & ss, const utils::exception & ex);
