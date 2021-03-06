/* -------------------------------------------------------------------------- */
/* Copyright 2002-2011, OpenNebula Project Leads (OpenNebula.org)             */
/*                                                                            */
/* Licensed under the Apache License, Version 2.0 (the "License"); you may    */
/* not use this file except in compliance with the License. You may obtain    */
/* a copy of the License at                                                   */
/*                                                                            */
/* http://www.apache.org/licenses/LICENSE-2.0                                 */
/*                                                                            */
/* Unless required by applicable law or agreed to in writing, software        */
/* distributed under the License is distributed on an "AS IS" BASIS,          */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   */
/* See the License for the specific language governing permissions and        */
/* limitations under the License.                                             */
/* -------------------------------------------------------------------------- */

#ifndef HOST_POOL_H_
#define HOST_POOL_H_

#include "PoolSQL.h"
#include "Host.h"
#include "ClusterPool.h"

#include <time.h>
#include <sstream>

#include <iostream>

#include <vector>

using namespace std;

/**
 *  The Host Pool class.
 */
class HostPool : public PoolSQL
{
public:
    HostPool(SqlDB *                   db,
             vector<const Attribute *> hook_mads,
             const string&             hook_location);

    ~HostPool(){};

    /**
     *  Function to allocate a new Host object
     *    @param oid the id assigned to the Host
     *    @return the oid assigned to the object or -1 in case of failure
     */
    int allocate (
        int *  oid,
        const string& hostname,
        const string& im_mad_name,
        const string& vmm_mad_name,
        const string& tm_mad_name,
        string& error_str);

    /**
     *  Function to get a Host from the pool, if the object is not in memory
     *  it is loaded from the DB
     *    @param oid Host unique id
     *    @param lock locks the Host mutex
     *    @return a pointer to the Host, 0 if the Host could not be loaded
     */
    Host * get(
        int     oid,
        bool    lock)
    {
        return static_cast<Host *>(PoolSQL::get(oid,lock));
    };

    /**
     *  Bootstraps the database table(s) associated to the Host pool
     */
    static void bootstrap(SqlDB *_db)
    {
        Host::bootstrap(_db);

        ClusterPool::bootstrap(_db);
    };

    /**
     * Get the least monitored hosts
     *   @param discovered hosts, map to store the retrieved hosts hids and
     *   hostnames
     *   @param host_limit max. number of hosts to monitor at a time
     *   @return int 0 if success
     */
    int discover(map<int, string> * discovered_hosts, int host_limit);

    /**
     * Allocates a given capacity to the host
     *   @param oid the id of the host to allocate the capacity
     *   @param cpu amount of CPU
     *   @param mem amount of main memory
     *   @param disk amount of disk
     */
    void add_capacity(int oid,int cpu, int mem, int disk)
    {
        Host *  host = get(oid, true);

        if ( host != 0 )
        {
          host->add_capacity(cpu, mem, disk);

          update(host);

          host->unlock();
        }
    };

    /**
     * De-Allocates a given capacity to the host
     *   @param oid the id of the host to allocate the capacity
     *   @param cpu amount of CPU
     *   @param mem amount of main memory
     *   @param disk amount of disk
     */
    void del_capacity(int oid,int cpu, int mem, int disk)
    {
        Host *  host = get(oid, true);

        if ( host != 0 )
        {
            host->del_capacity(cpu, mem, disk);

            update(host);

            host->unlock();
        }
    };

    /**
     *  Dumps the HOST pool in XML format. A filter can be also added to the
     *  query
     *  @param oss the output stream to dump the pool contents
     *  @param where filter for the objects, defaults to all
     *
     *  @return 0 on success
     */
    int dump(ostringstream& oss, const string& where);

    /* ---------------------------------------------------------------------- */
    /* ---------------------------------------------------------------------- */
    /* Methods for cluster management                                         */
    /* ---------------------------------------------------------------------- */
    /* ---------------------------------------------------------------------- */

    /**
     *  Returns true if the clid is an id for an existing cluster
     *  @param clid ID of the cluster
     *
     *  @return true if the clid is an id for an existing cluster
     */
   /* bool exists_cluster(int clid)
    {
        return cluster_pool.exists(clid);
    };*/

    /**
     *  Allocates a new cluster in the pool
     *    @param clid the id assigned to the cluster
     *    @return the id assigned to the cluster or -1 in case of failure
     */
    int allocate_cluster(int * clid, const string& name, string& error_str)
    {
        return cluster_pool.allocate(clid, name, db, error_str);
    };

    /**
     *  Returns the xml representation of the given cluster
     *    @param clid ID of the cluster
     *
     *    @return the xml representation of the given cluster
     */
    string info_cluster(int clid)
    {
        return cluster_pool.info(clid);
    };

    /**
     *  Removes the given cluster from the pool and the DB
     *    @param clid ID of the cluster
     *
     *    @return 0 on success
     */
    int drop_cluster(int clid);

    /**
     *  Dumps the cluster pool in XML format.
     *    @param oss the output stream to dump the pool contents
     *
     *    @return 0 on success
     */
    int dump_cluster(ostringstream& oss)
    {
        return cluster_pool.dump(oss);
    };

    /**
     *  Assigns the host to the given cluster
     *    @param host The host to assign
     *    @param clid ID of the cluster
     *
     *    @return 0 on success
     */
    int set_cluster(Host* host, int clid)
    {
        map<int, string>::iterator it;

        it = cluster_pool.cluster_names.find(clid);

        if (it == cluster_pool.cluster_names.end())
        {
            return -1;
        }

        return host->set_cluster( it->second );
    };

    /**
     *  Removes the host from the given cluster setting the default one.
     *    @param host The host to assign
     *
     *    @return 0 on success
     */
    int set_default_cluster(Host* host)
    {
        return host->set_cluster(ClusterPool::DEFAULT_CLUSTER_NAME);
    };

private:
    /**
     *  ClusterPool, clusters defined and persistance functionality
     */
    ClusterPool  cluster_pool;

    /**
     *  Factory method to produce Host objects
     *    @return a pointer to the new Host
     */
    PoolObjectSQL * create()
    {
        return new Host;
    };

    /**
     *  Callback function to build the cluster pool
     *    @param num the number of columns read from the DB
     *    @param names the column names
     *    @param vaues the column values
     *    @return 0 on success
     */
    int init_cb(void *nil, int num, char **values, char **names);

    /**
     *  Callback function to get the IDs of the hosts to be monitored
     *  (Host::discover)
     *    @param num the number of columns read from the DB
     *    @param names the column names
     *    @param vaues the column values
     *    @return 0 on success
     */
    int discover_cb(void * _map, int num, char **values, char **names);

    /**
     *  Callback function to get output the host pool in XML format
     *  (Host::dump)
     *    @param num the number of columns read from the DB
     *    @param names the column names
     *    @param vaues the column values
     *    @return 0 on success
     */
    int dump_cb(void * _oss, int num, char **values, char **names);
};

#endif /*HOST_POOL_H_*/
