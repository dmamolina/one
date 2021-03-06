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

#ifndef VIRTUAL_NETWORK_POOL_H_
#define VIRTUAL_NETWORK_POOL_H_

#include "PoolSQL.h"
#include "VirtualNetwork.h"

#include <time.h>

class AuthRequest;

using namespace std;

/**
 *  The Virtual Network Pool class. ...
 */
class VirtualNetworkPool : public PoolSQL
{
public:

    VirtualNetworkPool(SqlDB *          db,
                       const string&    str_mac_prefix,
                       int              default_size);

    ~VirtualNetworkPool(){};

    /**
     *  Function to allocate a new VNET object
     *    @param uid user identifier
     *    @param vn_template a VirtualNetworkTemplate describing the VNET
     *    @param oid the id assigned to the VM (output)
     *    @return oid on success, -1 error
     */
    int allocate (
        int     uid,
        VirtualNetworkTemplate * vn_template,
        int *   oid,
        string& error_str);

    /**
     *  Function to get a VN from the pool, if the object is not in memory
     *  it is loaded from the DB
     *    @param oid VN unique id
     *    @param lock locks the VN mutex
     *    @return a pointer to the VN, 0 if the VN could not be loaded
     */
    VirtualNetwork * get(
        int     oid,
        bool    lock)
    {
        return static_cast<VirtualNetwork *>(PoolSQL::get(oid,lock));
    };

    /**
     *  Function to get a VN from the pool using the network name
     *  If the object is not in memory it is loaded from the DB
     *    @param name VN unique name
     *    @param lock locks the VN mutex
     *    @return a pointer to the VN, 0 if the VN could not be loaded
     */
    VirtualNetwork * get(
        const string&  name,
        bool           lock);

    //--------------------------------------------------------------------------
    // Virtual Network DB access functions
    //--------------------------------------------------------------------------

    /**
     *  Generates a NIC attribute for VM templates using the VirtualNetwork
     *  metadata
     *    @param nic the nic attribute to be generated
     *    @param vid of the VM requesting the lease
     *    @return 0 on success, -1 error, -2 not using the pool
     */
    int nic_attribute(VectorAttribute * nic, int vid);

    /**
     *  Generates an Authorization token for a NIC attribute
     *    @param nic the nic to be authorized
     *    @param ar the AuthRequest
     */
    void authorize_nic(VectorAttribute * nic, AuthRequest * ar);

    /**
     *  Bootstraps the database table(s) associated to the VirtualNetwork pool
     */
    static void bootstrap(SqlDB * _db)
    {
        VirtualNetwork::bootstrap(_db);
    };

    /**
     *  Dumps the Virtual Network pool in XML format. A filter can be also added
     *  to the query
     *  @param oss the output stream to dump the pool contents
     *  @param where filter for the objects, defaults to all
     *
     *  @return 0 on success
     */
    int dump(ostringstream& oss, const string& where);

    /**
     *  Get the mac prefix
     *  @return the mac prefix
     */
    static const unsigned int& mac_prefix()
    {
        return _mac_prefix;
    };

    /**
     *  Get the default network size
     *  @return the size
     */
    static const unsigned int& default_size()
    {
        return _default_size;
    };

private:
    /**
     *  Holds the system-wide MAC prefix
     */
    static unsigned int     _mac_prefix;

    /**
     *  Default size for Virtual Networks
     */
    static unsigned int     _default_size;

    /**
     *  Factory method to produce VN objects
     *    @return a pointer to the new VN
     */
    PoolObjectSQL * create()
    {
        return new VirtualNetwork();
    };

    /**
     *  Callback function to get output the virtual network pool in XML format
     *  (VirtualNetworkPool::dump)
     *    @param num the number of columns read from the DB
     *    @param names the column names
     *    @param vaues the column values
     *    @return 0 on success
     */
    int dump_cb(void * _oss, int num, char **values, char **names);

    /**
     *  Callback function to get the ID of a given virtual network
     *  (VirtualNetworkPool::get)
     *    @param num the number of columns read from the DB
     *    @param names the column names
     *    @param vaues the column values
     *    @return 0 on success
     */
    int get_cb(void * _oss, int num, char **values, char **names);
};

#endif /*VIRTUAL_NETWORK_POOL_H_*/
