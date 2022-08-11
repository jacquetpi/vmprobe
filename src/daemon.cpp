#include "daemon.hpp"
#include "iostream"
#include <string>
#include "time.h"
#include "utils/config.hpp"
#include "utils/log.hpp"

namespace server {

    Daemon::Daemon(){
        _delay = utils::Config::Get().delay;
        _dump = new server::Dump(utils::Config::Get().prefix, utils::Config::Get().endpoint);
        _libvirt = new server::LibvirtClient(utils::Config::Get().url.c_str());
        _perfcli = new server::PerfClient();
    };

    void Daemon::start () {
        this-> _libvirt->connect ();
        this-> _perfcli->perf_init();
        this-> _perfcli->perf_enable();
        long long epochBegin;
        long long epochEnd;
        while(true){
            epochBegin = static_cast<long long> (time(NULL));
            _dump->addGlobalMetric("probe_epoch", epochBegin);
            _dump->addGlobalMetric("probe_delay", _delay);
            retrievePerfMetrics();
            retrieveLibvirtMetrics();
            _dump->dump();
            _dump->clear();
            epochEnd= static_cast<long long> (time(NULL));
            if(_delay > (epochEnd-epochBegin)){
                sleep(_delay - (epochEnd-epochBegin));
            }
            else{
                utils::logging::info("delay exceeded by fetching time", (epochEnd-epochBegin), ">", _delay);
            }
        }
    }

    inline void Daemon::retrievePerfMetrics(){
        long long instructions;
        long long cycles;
        _perfcli->perf_read_cpi(&instructions, &cycles);
        _perfcli->perf_reset();
        _dump->addGlobalMetric("cpu_instructions", instructions);
        _dump->addGlobalMetric("cpu_cycles", cycles);
        _dump->addGlobalMetric("cpu_freq", _perfcli->readCPUFrequency());
        _dump->addGlobalMetric("cpu_total", _perfcli->getVCPUs());
        _perfcli->addHostMemoryUsage(_dump);
    }

    inline void Daemon::retrieveLibvirtMetrics(){
        _libvirt->addNodeCPUMetrics(_dump);
        _libvirt->addAllDomainsMetrics(_dump);
    }

    void Daemon::kill () {
        this-> _libvirt->disconnect ();
        this-> _perfcli->perf_close();
        free(_libvirt);
        free(_perfcli);
    }

}
