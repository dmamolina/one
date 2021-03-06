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

#include "RequestManager.h"
#include "NebulaLog.h"

#include "AuthManager.h"

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void RequestManager::ClusterInfo::execute(
    xmlrpc_c::paramList const& paramList,
    xmlrpc_c::value *   const  retval)
{
    string  session;
    string  info;

    int     clid;
    int     rc;
    
    const string   method_name = "ClusterInfo";

    ostringstream oss;

    /*   -- RPC specific vars --  */
    vector<xmlrpc_c::value> arrayData;
    xmlrpc_c::value_array * arrayresult;

    NebulaLog::log("ReM",Log::DEBUG,"ClusterInfo method invoked");

    // Get the parameters
    session      = xmlrpc_c::value_string(paramList.getString(0));
    clid         = xmlrpc_c::value_int   (paramList.getInt(1));

    //Authenticate the user
    rc = ClusterInfo::upool->authenticate(session);

    if ( rc == -1 )
    {
        goto error_authenticate;
    }

    info = ClusterInfo::hpool->info_cluster(clid);

    // Cluster does not exists
    if ( info.empty() )
    {
        goto error_cluster;
    }

    // All nice, return the cluster info to the client
    arrayData.push_back(xmlrpc_c::value_boolean(true)); // SUCCESS
    arrayData.push_back(xmlrpc_c::value_string(info));

    // Copy arrayresult into retval mem space
    arrayresult = new xmlrpc_c::value_array(arrayData);
    *retval = *arrayresult;

    delete arrayresult; // and get rid of the original

    return;

error_authenticate:
    oss.str(authenticate_error(method_name));
    goto error_common;

error_cluster:
    oss.str(get_error(method_name, "CLUSTER", clid));
    goto error_common;

error_common:

    arrayData.push_back(xmlrpc_c::value_boolean(false)); // FAILURE
    arrayData.push_back(xmlrpc_c::value_string(oss.str()));

    NebulaLog::log("ReM",Log::ERROR,oss);

    xmlrpc_c::value_array arrayresult_error(arrayData);

    *retval = arrayresult_error;

    return;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
