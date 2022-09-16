#include "daemon.hpp"
#include "iostream"
#include <string>
#include "utils/config.hpp"
#include "utils/log.hpp"
#include <chrono>

namespace server {

    Daemon::Daemon(){
        _delay = utils::Config::Get().delay;
        _dump = new server::Dump(utils::Config::Get().prefix, utils::Config::Get().endpoint);
        _libvirt = new server::LibvirtClient(utils::Config::Get().url.c_str());
        _perfcli = new server::PerfClient();
    };

    void Daemon::start () {
        this-> _libvirt->connect ();
        this-> _perfcli->perfInit();
        this-> _perfcli->perfEnable();
        long long epochBegin;
        long long epochEnd;
        while(true){
            _dump->addGlobalMetric("probe_delay", _delay);
            epochBegin =  std::chrono::duration_cast< std::chrono::milliseconds >(std::chrono::system_clock::now().time_since_epoch()).count();
            _dump->addGlobalMetric("probe_epoch", epochBegin);
            retrievePerfMetrics();
            retrieveLibvirtMetrics();
            _dump->dump();
            _dump->clear();
            epochEnd= std::chrono::duration_cast< std::chrono::milliseconds >(std::chrono::system_clock::now().time_since_epoch()).count();
            if(_delay > (epochEnd-epochBegin)){
                sleep((_delay - (epochEnd-epochBegin))/1000);
            }
            else{
                utils::logging::warn("delay exceeded by fetching time", (epochEnd-epochBegin), ">", _delay);
            }
        }
    }

    inline void Daemon::retrievePerfMetrics(){
        long long instructions;
        long long cycles;
        _perfcli->perfRead(_dump);
        _perfcli->perfReset();
        _dump->addGlobalMetric("cpu_freq", _perfcli->readCPUFrequency());
        _dump->addGlobalMetric("cpu_minfreq", _perfcli->getMinFreq());
        _dump->addGlobalMetric("cpu_maxfreq", _perfcli->getMaxFreq());
        _dump->addGlobalMetric("cpu_total", _perfcli->getVCPUs());
        _perfcli->addHostMemoryUsage(_dump);
    }

    inline void Daemon::retrieveLibvirtMetrics(){
        _libvirt->addNodeCPUMetrics(_dump);
        _libvirt->addAllDomainsMetrics(_dump);
    }

    void Daemon::kill () {
        this-> _libvirt->disconnect ();
        this-> _perfcli->perfClose();
        free(_libvirt);
        free(_perfcli);
    }

}
