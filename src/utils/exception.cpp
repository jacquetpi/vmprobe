#include "exception.hpp"

#ifdef linux
#include <cxxabi.h>
#endif

namespace utils {

	exception::exception (const std::string & msg) :
	    msg (msg)
	{}

	void exception::print () const {
	    std::cerr << this-> msg << std::endl;
	}
    
}

std::ostream & operator << (std::ostream & ss, const utils::exception & ex) {
    ex.print ();
    return ss;
}
