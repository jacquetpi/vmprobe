#include "libvirtcli.hpp"
#include <libvirt/libvirt.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include "utils/log.hpp"
#include "error.hpp"
#include "utils/config.hpp"

namespace server {

    LibvirtClient::LibvirtClient (const char * uri) :_conn (nullptr), _uri (uri){
        if (getuid()) {
            utils::logging::error ("you are not root. This program will only work if run as root.");
            exit(1);
        }
    }

    /**
     * ================================================================================
     * ================================================================================
     * =========================          CONNECTION          =========================
     * ================================================================================
     * ================================================================================
     */
    void LibvirtClient::connect () {
        // First disconnect, maybe it was connected to something
        this-> disconnect ();
        
        // We need an auth connection to have write access to the domains
        this-> _conn = virConnectOpenAuth (this-> _uri, virConnectAuthPtrDefault, 0);
        if (this-> _conn == nullptr) {
            utils::logging::error ("LibvirtClient::connect Failed to connect libvirt client to :", this-> _uri);
            throw ProbeError ("Connection to hypervisor failed\n");
        }

        utils::logging::success ("Libvirt client connected to :", this-> _uri);
    }

    void LibvirtClient::disconnect () {
        if (this-> _conn != nullptr) {
        virConnectClose (this-> _conn);
        this-> _conn = nullptr;
        utils::logging::info ("Libvirt client disconnected");
        }
    }

    void LibvirtClient::addAllDomainsMetrics(Dump* dump) {
        virDomainPtr * domains = nullptr;  
        auto num_domains = virConnectListAllDomains (this-> _conn, &domains, VIR_CONNECT_LIST_DOMAINS_ACTIVE);
        for (int i = 0 ; i < num_domains ; i++) {
            virDomainPtr dom = domains [i];
            addDomainCPUMetrics(dump, dom);
            addDomainMemoryMetrics(dump, dom);
            togglePerfEvents(dom, true);
            virDomainFree(dom);
        }
        addDomainsPerfInfo(dump);
        free (domains);
    }

    void LibvirtClient::addDomainMemoryMetrics(Dump* dump, virDomainPtr dom) {    
        std::string name = virDomainGetName (dom);
        virDomainMemoryStatPtr minfo =  (virDomainMemoryStatPtr) calloc(VIR_DOMAIN_MEMORY_STAT_NR, sizeof(*minfo));
        if (minfo == NULL) {
            utils::logging::error ("LibvirtClient::addDomainMemoryMetrics failed (failed calloc):", this-> _uri, name);
            throw ProbeError ("LibvirtClient::addDomainMemoryMetrics failed\n");
        }
        int mem_stats = virDomainMemoryStats(dom, minfo, VIR_DOMAIN_MEMORY_STAT_NR, 0);
        for (int i = 0; i < mem_stats; i++) {
            switch (minfo[i].tag) {
                case VIR_DOMAIN_MEMORY_STAT_SWAP_IN:
                    dump->addSpecificMetric(name, "memory_swapin", minfo[i].val);
                    break;
                case VIR_DOMAIN_MEMORY_STAT_SWAP_OUT:
                    dump->addSpecificMetric(name, "memory_swapout", minfo[i].val);
                    break;
                case VIR_DOMAIN_MEMORY_STAT_MAJOR_FAULT:
                    dump->addSpecificMetric(name, "memory_majorfault", minfo[i].val);
                    break;
                case VIR_DOMAIN_MEMORY_STAT_MINOR_FAULT:
                    dump->addSpecificMetric(name, "memory_minorfault", minfo[i].val);
                    break;
                case VIR_DOMAIN_MEMORY_STAT_UNUSED:
                    dump->addSpecificMetric(name, "memory_unused", minfo[i].val);
                    break;
                case VIR_DOMAIN_MEMORY_STAT_AVAILABLE:
                    dump->addSpecificMetric(name, "memory_available", minfo[i].val);
                    break;
                case VIR_DOMAIN_MEMORY_STAT_ACTUAL_BALLOON:
                    dump->addSpecificMetric(name, "memory_alloc", minfo[i].val);
                    break;
                case VIR_DOMAIN_MEMORY_STAT_RSS:
                    dump->addSpecificMetric(name, "memory_rss", minfo[i].val);
                    break;
                case VIR_DOMAIN_MEMORY_STAT_USABLE:
                    dump->addSpecificMetric(name, "memory_usable", minfo[i].val);
                    break;
                case VIR_DOMAIN_MEMORY_STAT_LAST_UPDATE:
                    dump->addSpecificMetric(name, "memory_last_update", minfo[i].val);
                    break;
                case VIR_DOMAIN_MEMORY_STAT_DISK_CACHES:
                    dump->addSpecificMetric(name, "memory_diskcaches", minfo[i].val);
                    break;
                case  VIR_DOMAIN_MEMORY_STAT_HUGETLB_PGALLOC:
                    dump->addSpecificMetric(name, "memory_hugetlbpgalloc", minfo[i].val);
                    break;
                case VIR_DOMAIN_MEMORY_STAT_HUGETLB_PGFAIL:
                    dump->addSpecificMetric(name, "memory_hugetlb_pgfail", minfo[i].val);
                    break;
            }
        }
    }

    void LibvirtClient::addNodeMemoryMetrics(Dump* dump) {
        int nparams = 0;
        virNodeMemoryStatsPtr params;
        // Retrieve dynamic nparams value : https://libvirt.org/html/libvirt-libvirt-host.html#virNodeGetMemoryStats
        if (virNodeGetMemoryStats(this-> _conn, VIR_NODE_MEMORY_STATS_ALL_CELLS, NULL, &nparams, 0) == 0 && nparams != 0) {
            if ((params = (virNodeMemoryStatsPtr) malloc(sizeof(virNodeMemoryStats) * nparams)) == NULL){
                utils::logging::error ("LibvirtClient::get_node_memory_stats failed (failed alloc):", this-> _uri);
                throw ProbeError ("LibvirtClient::get_node_memory_stats failed\n");
            }
            memset(params, 0, sizeof(virNodeMemoryStats) * nparams);
            if (virNodeGetMemoryStats(this-> _conn, VIR_NODE_MEMORY_STATS_ALL_CELLS, params, &nparams, 0)){
                utils::logging::error ("LibvirtClient::get_node_memory_stats failed (failed call):", this-> _uri);
                throw ProbeError ("LibvirtClient::get_node_memory_stats failed\n");
            }
            for (int i = 0; i < nparams; i++) {
                switch (params[i].field[0]) {
                    case 't':
                        dump->addGlobalMetric("memory_total", params[i].value);
                        break;
                    case 'f':
                        dump->addGlobalMetric("memory_vfree", params[i].value);
                        break;
                    case 'b':
                        dump->addGlobalMetric("memory_buffers", params[i].value);
                        break;
                    case 'c':
                        dump->addGlobalMetric("memory_cached", params[i].value);
                        break;
                }
            }
            free(params);
        }
    }

    void LibvirtClient::addDomainCPUMetrics(Dump* dump, virDomainPtr dom) {
        std::string name = virDomainGetName (dom);
        dump->addSpecificMetric(name, "cpu_alloc", virDomainGetMaxVcpus(dom));
        int nparams = virDomainGetCPUStats(dom, NULL, 0, -1, 1, 0);
        if (nparams <= 0) {
            utils::logging::info ("LibvirtClient::get_domain_cpu_stats failed (invalid nparams) domain probably died:", this-> _uri, name);
            return;
        }
        virTypedParameterPtr params;
        if ((params = (virTypedParameterPtr) malloc(sizeof(virTypedParameter) * nparams)) == NULL){
            utils::logging::error ("LibvirtClient::get_domain_cpu_stats failed (failed alloc):", this-> _uri);
            throw ProbeError ("LibvirtClient::get_domain_cpu_stats failed\n");
        }
        memset(params, 0, sizeof(virTypedParameter) * nparams);
        nparams = virDomainGetCPUStats(dom, params, nparams, -1, 1, 0);
        for (int i = 0; i < nparams; i++) {
            if(params[i].type != VIR_TYPED_PARAM_ULLONG){
                utils::logging::error ("LibvirtClient::get_domain_cpu_stats failed (type error):", this-> _uri);
                throw ProbeError ("LibvirtClient::get_domain_cpu_stats failed\n");
            }
            switch (params[i].field[0]) {
                    case 'c':
                        dump->addSpecificMetric(name, "cpu_cputime", params[i].value.ul);
                        break;
                    case 'u':
                        dump->addSpecificMetric(name, "cpu_usertime", params[i].value.ul);
                        break;
                    case 's':
                        dump->addSpecificMetric(name, "cpu_systemtime", params[i].value.ul);
                        break;
                }
        }
        free(params);
    }

    // Check src/util/virperf.h
    void LibvirtClient::togglePerfEvents(virDomainPtr domain, bool status) {
        unsigned int flags = VIR_DOMAIN_AFFECT_CURRENT;
        flags |= VIR_DOMAIN_AFFECT_LIVE;
        int nparams = 0;
        int maxparams = 0;
        virTypedParameterPtr params = NULL;
        // IPC
        virTypedParamsAddBoolean(&params, &nparams, &maxparams, VIR_PERF_PARAM_INSTRUCTIONS, status);
        virTypedParamsAddBoolean(&params, &nparams, &maxparams, VIR_PERF_PARAM_CPU_CYCLES, status);
        // BM%
        virTypedParamsAddBoolean(&params, &nparams, &maxparams, VIR_PERF_PARAM_BRANCH_MISSES, status);
        virTypedParamsAddBoolean(&params, &nparams, &maxparams, VIR_PERF_PARAM_BRANCH_INSTRUCTIONS, status);
        // Context switch
        virTypedParamsAddBoolean(&params, &nparams, &maxparams, VIR_PERF_PARAM_CONTEXT_SWITCHES, status);
        // Cache miss
        virTypedParamsAddBoolean(&params, &nparams, &maxparams, VIR_PERF_PARAM_CACHE_MISSES, status);
        virTypedParamsAddBoolean(&params, &nparams, &maxparams, VIR_PERF_PARAM_CACHE_REFERENCES, status);
        virTypedParamsAddBoolean(&params, &nparams, &maxparams, VIR_PERF_PARAM_PAGE_FAULTS, status);
        virTypedParamsAddBoolean(&params, &nparams, &maxparams, VIR_PERF_PARAM_ALIGNMENT_FAULTS, status);
        virTypedParamsAddBoolean(&params, &nparams, &maxparams, VIR_PERF_PARAM_EMULATION_FAULTS, status);

        if (virDomainSetPerfEvents(domain, params, nparams, flags) != 0){
            utils::logging::error ("LibvirtClient::enablePerfEvents failed rc");
        }
        virTypedParamsFree(params, nparams);
    }

    void LibvirtClient::addDomainsPerfInfo(Dump* dump) {
        int flags = 0;
        unsigned int stats = 0;
        stats |= VIR_DOMAIN_STATS_PERF;
        virDomainStatsRecordPtr *next;
        virDomainStatsRecordPtr *records = NULL;
        if ((virConnectGetAllDomainStats(this->_conn, stats, &records, flags)) < 0){
            utils::logging::error ("LibvirtClient::addDomainsPerfInfo virConnectGetAllDomainStats failed rc");
            return;
        }

        next = records;
        while (*next) {
            std::string name = virDomainGetName((*next)->dom);
            for (int i = 0; i < (*next)->nparams; i++) {
                if((*next)->params[i].type != VIR_TYPED_PARAM_ULLONG){
                    utils::logging::error ("LibvirtClient::addDomainPerfInfo failed (type error):", this-> _uri, (*next)->params[i].field);
                    continue;
                }
                std::string field = (*next)->params[i].field;
                field.erase(remove(field.begin(), field.end(), '_'), field.end());
                std::replace(field.begin(), field.end(), '.', '_');
                dump->addSpecificMetric(name, field, (*next)->params[i].value.ul);
            }
            ++next;
        }
        virDomainStatsRecordListFree(records);
    }

    void LibvirtClient::addNodeCPUMetrics(Dump* dump) {
        // Dynamic nparams https://libvirt.org/html/libvirt-libvirt-host.html#virNodeGetCPUStats
        int nparams = 0;
        virNodeCPUStatsPtr params;
        if (virNodeGetCPUStats(this-> _conn, VIR_NODE_CPU_STATS_ALL_CPUS, NULL, &nparams, 0) == 0 && nparams != 0) {
            if ((params = (virNodeCPUStatsPtr) malloc(sizeof(virNodeCPUStats) * nparams)) == NULL){
                utils::logging::error ("LibvirtClient::get_node_cpu_stats Failed (failed alloc):", this-> _uri);
                throw ProbeError ("LibvirtClient::get_node_cpu_stats Failed\n");
            }
            memset(params, 0, sizeof(virNodeCPUStats) * nparams);
            if (virNodeGetCPUStats(this-> _conn, VIR_NODE_CPU_STATS_ALL_CPUS, params, &nparams, 0)){
                utils::logging::error ("LibvirtClient::get_node_cpu_stats Failed (failed call):", this-> _uri);
                throw ProbeError ("LibvirtClient::get_node_cpu_stats Failed\n");
            }
            unsigned long long kernel = -1;
            unsigned long long user = -1;
            unsigned long long idle = -1;
            unsigned long long iowait = -1;
            for (int i = 0; i < nparams; i++) {
                switch (params[i].field[1]) {
                    case 'e':
                        dump->addGlobalMetric("cpu_kernel", params[i].value);
                        break;
                    case 's':
                        dump->addGlobalMetric("cpu_user", params[i].value);
                        break;
                    case 'd':
                        dump->addGlobalMetric("cpu_idle", params[i].value);
                        break;
                    case 'o':
                        dump->addGlobalMetric("cpu_iowait", params[i].value);
                        break;
                }
            }
            free(params);
        }
    }

}