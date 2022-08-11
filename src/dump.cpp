#include "dump.hpp"
#include <algorithm>
#include "utils/log.hpp"
#include <fstream>

namespace server {

    Dump::Dump(std::string prefix, std::string file) : _prefix(prefix), _file(file) {};

    void Dump::dump(){      
      std::ofstream stream(_file);
      for(auto& kv : _map) {
         stream << kv.first << " " << kv.second <<"\n"; 
      }
      stream.close();
    }

    void Dump::clear(){
      this -> _map.clear();
    }

   void Dump::addGlobalMetric(std::string key, int value){
      this-> addGlobalMetric(key, std::to_string(value));
   }

   void Dump::addGlobalMetric(std::string key, long long value){
      this-> addGlobalMetric(key, std::to_string(value));
   }

   void Dump::addGlobalMetric(std::string key, unsigned long value){
      this-> addGlobalMetric(key, std::to_string(value));
   }

   void Dump::addGlobalMetric(std::string key, unsigned long long value){
      this-> addGlobalMetric(key, std::to_string(value));
   }

   void Dump::addGlobalMetric(std::string key, std::string value){
      this-> addMetric("global_" + key, value);
   }

   void Dump::addSpecificMetric(std::string identifier, std::string key, unsigned short value){
      this-> addSpecificMetric(identifier, key, std::to_string(value));
   }

   void Dump::addSpecificMetric(std::string identifier, std::string key, long long value){
      this-> addSpecificMetric(identifier, key, std::to_string(value));
   }

   void Dump::addSpecificMetric(std::string identifier, std::string key, unsigned long value){
      this-> addSpecificMetric(identifier, key, std::to_string(value));
   }

   void Dump::addSpecificMetric(std::string identifier, std::string key, unsigned long long value){
      this-> addSpecificMetric(identifier, key, std::to_string(value));
   }

   void Dump::addSpecificMetric(std::string identifier, std::string key, std::string value){
      this-> addMetric("domain_" + identifier + '_' + key, value);
   }

   inline void Dump::addMetric(std::string key, std::string value){
      this -> _map.insert({_prefix + "_" + key, value});
   }   

}