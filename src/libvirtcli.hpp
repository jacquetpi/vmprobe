#pragma once
#include <libvirt/libvirt.h>
#include <filesystem>
#include "dump.hpp"

namespace server {
	
	/**
	 * The libvirt client is used to retreive VM domains
	 * It handles the connection to the qemu system
	 * @good_practice: use only one client for the whole program, maybe this should be a singleton
	 */
	class LibvirtClient {
	    
		private:
		
	    /// The connection to the libvirt server
	    virConnectPtr _conn;

	    /// The uri of the qemu system
	    const char * _uri;
	    
		public:
	    LibvirtClient (const char * uri);

	    /**
	     * ================================================================================
	     * ================================================================================
	     * =========================          CONNECTION          =========================
	     * ================================================================================
	     * ================================================================================
	     */
	    
	    /**
	     * Connect the client
	     * @info: this is a pretty heavy function, it should be done once at the beginning of the program
	     * @throws: 
	     *   - LibvirtError: if the connection failed
	     */
	    void connect ();

	    /**
	     * Close the connection of the client
	     * @info: it can be reconnected afterward
	     */
	    void disconnect ();

        /**
         * ================================================================================
         * ================================================================================
         * =========================            DOMAIN            =========================
         * ================================================================================
         * ================================================================================
         */

	    /**
	     * @TEMP
	     * @DEBUG
	     * Print the list of domains on the machine
	     */	    
	    void addAllDomainsMetrics(Dump* dump);

		void addDomainInfo(Dump* dump, virDomainPtr dom) ;

		void addDomainMemoryMetrics(Dump* dump, virDomainPtr domain) ;

		void addDomainCPUMetrics(Dump* dump, virDomainPtr domain) ;

		void togglePerfEvents(virDomainPtr domain, bool status) ;

		void addDomainsPerfInfo(Dump* dump) ;

		/**
         * ================================================================================
         * ================================================================================
         * ==========================            NODE            ==========================
         * ================================================================================
         * ================================================================================
         */

		void addNodeMemoryMetrics(Dump* dump) ;

		void addNodeCPUMetrics(Dump* dump) ;

	};
}
    
