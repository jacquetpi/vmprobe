#include <unordered_map>
#include <string>
#pragma once

namespace server {

    class Dump {

        private:

        std::unordered_map<std::string, std::string> _map;
        std::string _prefix;
        std::string _file;

        void addMetric(std::string key, std::string value);

        public:

        Dump(std::string prefix, std::string file);

        void dump();

        void clear();

        void addGlobalMetric(std::string key, int value);

        void addGlobalMetric(std::string key, long long value);
    
        void addGlobalMetric(std::string key, unsigned long value);

        void addGlobalMetric(std::string key, unsigned long long value);

        void addGlobalMetric(std::string key, std::string value);

        void addSpecificMetric(std::string identifier, std::string key, std::string value);

        void addSpecificMetric(std::string identifier, std::string key, unsigned short value);

        void addSpecificMetric(std::string identifier, std::string key, long long value);

        void addSpecificMetric(std::string identifier, std::string key, unsigned long value);

        void addSpecificMetric(std::string identifier, std::string key, unsigned long long value);

    };

}