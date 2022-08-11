#pragma once
#include "utils/exception.hpp"

namespace server {

	class ProbeError : public utils::exception {

	public:

	    ProbeError (const std::string & msg);
	    
	};
    
}
