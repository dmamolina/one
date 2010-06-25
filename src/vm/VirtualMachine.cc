/* -------------------------------------------------------------------------- */
/* Copyright 2002-2010, OpenNebula Project Leads (OpenNebula.org)             */
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
#include <limits.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>
#include <sstream>

#include "VirtualMachine.h"
#include "VirtualNetworkPool.h"
#include "NebulaLog.h"

#include "Nebula.h"


#include "vm_var_syntax.h"

/* ************************************************************************** */
/* Virtual Machine :: Constructor/Destructor                                  */
/* ************************************************************************** */

VirtualMachine::VirtualMachine(int id):
        PoolObjectSQL(id),
        uid(-1),
        last_poll(0),
        name(""),
        vm_template(),
        state(INIT),
        lcm_state(LCM_INIT),
        stime(time(0)),
        etime(0),
        deploy_id(""),
        memory(0),
        cpu(0),
        net_tx(0),
        net_rx(0),
        last_seq(-1),
        history(0),
        previous_history(0),
        _log(0)
{
}

VirtualMachine::~VirtualMachine()
{
    if ( history != 0 )
    {
        delete history;
    }

    if ( previous_history != 0 )
    {
        delete previous_history;
    }

    if ( _log != 0 )
    {
        delete _log;
    }
}

/* ************************************************************************** */
/* Virtual Machine :: Database Access Functions                               */
/* ************************************************************************** */

const char * VirtualMachine::table = "vm_pool";

const char * VirtualMachine::db_names =
    "(oid,uid,name,last_poll,template_id,state,lcm_state,stime,etime,deploy_id"
                                        ",memory,cpu,net_tx,net_rx,last_seq)";

const char * VirtualMachine::db_bootstrap = "CREATE TABLE IF NOT EXISTS "
        "vm_pool ("
        "oid INTEGER PRIMARY KEY,uid INTEGER,name TEXT,"
        "last_poll INTEGER, template_id INTEGER,state INTEGER,lcm_state INTEGER,"
        "stime INTEGER,etime INTEGER,deploy_id TEXT,memory INTEGER,cpu INTEGER,"
        "net_tx INTEGER,net_rx INTEGER, last_seq INTEGER)";

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int VirtualMachine::select_cb(void *nil, int num, char **values, char **names)
{
    if ((values[OID] == 0) ||
            (values[UID] == 0) ||
            (values[NAME] == 0) ||
            (values[LAST_POLL] == 0) ||
            (values[TEMPLATE_ID] == 0) ||
            (values[STATE] == 0) ||
            (values[LCM_STATE] == 0) ||
            (values[STIME] == 0) ||
            (values[ETIME] == 0) ||
            (values[MEMORY] == 0) ||
            (values[CPU] == 0) ||
            (values[NET_TX] == 0) ||
            (values[NET_RX] == 0) ||
            (values[LAST_SEQ] == 0) ||
            (num != LIMIT ))
    {
        return -1;
    }

    oid  = atoi(values[OID]);
    uid  = atoi(values[UID]);
    name = values[NAME];

    last_poll = static_cast<time_t>(atoi(values[LAST_POLL]));

    state     = static_cast<VmState>(atoi(values[STATE]));
    lcm_state = static_cast<LcmState>(atoi(values[LCM_STATE]));

    stime = static_cast<time_t>(atoi(values[STIME]));
    etime = static_cast<time_t>(atoi(values[ETIME]));

    if ( values[DEPLOY_ID] != 0 )
    {
        deploy_id = values[DEPLOY_ID];
    }

    memory      = atoi(values[MEMORY]);
    cpu         = atoi(values[CPU]);
    net_tx      = atoi(values[NET_TX]);
    net_rx      = atoi(values[NET_RX]);
    last_seq    = atoi(values[LAST_SEQ]);

    vm_template.id = atoi(values[TEMPLATE_ID]);

    return 0;
}

/* -------------------------------------------------------------------------- */

int VirtualMachine::select(SqlDB * db)
{
    ostringstream   oss;
    ostringstream   ose;

    int             rc;
    int             boid;

    string          filename;
    Nebula&         nd = Nebula::instance();

    set_callback(
        static_cast<Callbackable::Callback>(&VirtualMachine::select_cb));

    oss << "SELECT * FROM " << table << " WHERE oid = " << oid;

    boid = oid;
    oid  = -1;

    rc = db->exec(oss,this);

    unset_callback();

    if ((rc != 0) || (oid != boid ))
    {
        goto error_id;
    }

    //Get the template
    rc = vm_template.select(db);

    if (rc != 0)
    {
        goto error_template;
    }

    //Get the History Records

    if ( last_seq != -1 )
    {
        history = new History(oid, last_seq);

        rc = history->select(db);

        if (rc != 0)
        {
            goto error_history;
        }
    }

    if ( last_seq > 0 )
    {
        previous_history = new History(oid, last_seq - 1);

        rc = previous_history->select(db);

        if ( rc != 0)
        {
            goto error_previous_history;
        }
    }

    //Create support directory fo this VM

    oss.str("");
    oss << nd.get_var_location() << oid;

    mkdir(oss.str().c_str(), 0777);
    chmod(oss.str().c_str(), 0777);

    //Create Log support fo this VM

    try
    {
        _log = new FileLog(nd.get_vm_log_filename(oid),Log::DEBUG);
    }
    catch(exception &e)
    {
        ose << "Error creating log: " << e.what();
        NebulaLog::log("ONE",Log::ERROR, ose);

        _log = 0;
    }

    return 0;

error_id:
    ose << "Error getting VM id: " << oid;
    log("VMM", Log::ERROR, ose);
    return -1;

error_template:
    ose << "Can not get template for VM id: " << oid;
    log("ONE", Log::ERROR, ose);
    return -1;

error_history:
    ose << "Can not get history for VM id: " << oid;
    log("ONE", Log::ERROR, ose);
    return -1;

error_previous_history:
    ose << "Can not get previous history record (seq:" << history->seq
        << ") for VM id: " << oid;
    log("ONE", Log::ERROR, ose);
    return -1;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int VirtualMachine::insert(SqlDB * db)
{
    int    rc;
    string name;

    SingleAttribute *   attr;
    string              value;
    ostringstream       oss;

    // -----------------------------------------------------------------------
    // Set a name if the VM has not got one and VM_ID
    // ------------------------------------------------------------------------
    oss << oid;
    value = oss.str();

    attr = new SingleAttribute("VMID",value);

    vm_template.set(attr);


    get_template_attribute("NAME",name);

    if ( name.empty() == true )
    {
        oss.str("");
        oss << "one-" << oid;
        name = oss.str();

        attr = new SingleAttribute("NAME",name);
        vm_template.set(attr);
    }

    this->name = name;

    // ------------------------------------------------------------------------
    // Get network leases
    // ------------------------------------------------------------------------

    rc = get_network_leases();

    if ( rc != 0 )
    {
    	goto error_leases;
    }

    // ------------------------------------------------------------------------
    // Get disk images
    // ------------------------------------------------------------------------

    rc = get_disk_images();

    if ( rc != 0 )
    {
        goto error_images;
    }

    // ------------------------------------------------------------------------
    // Insert the template first, so we get a valid template ID. Then the VM
    // ------------------------------------------------------------------------

    rc = vm_template.insert(db);

    if ( rc != 0 )
    {
        goto error_template;
    }

    rc = insert_replace(db, false);

    if ( rc != 0 )
    {
        goto error_update;
    }

    return 0;

error_update:
    NebulaLog::log("ONE",Log::ERROR, "Can not update VM in the database");
    vm_template.drop(db);
    return -1;

error_template:
    NebulaLog::log("ONE",Log::ERROR, "Can not insert template in the database");
    release_network_leases();
    return -1;

error_leases:
    NebulaLog::log("ONE",Log::ERROR, "Could not get network lease for VM");
    release_network_leases();
    return -1;

error_images:
    NebulaLog::log("ONE",Log::ERROR, "Could not get disk image for VM");
    release_disk_images();
    return -1;
}

/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */

int VirtualMachine::update(SqlDB * db)
{
    int             rc;

    rc = insert_replace(db, true);

    return rc;
}

/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */

int VirtualMachine::insert_replace(SqlDB *db, bool replace)
{
    ostringstream   oss;
    int             rc;

    char * sql_deploy_id;
    char * sql_name;

    sql_deploy_id = db->escape_str(deploy_id.c_str());

    if ( sql_deploy_id == 0 )
    {
        return -1;
    }

    sql_name =  db->escape_str(name.c_str());

    if ( sql_name == 0 )
    {
       db->free_str(sql_deploy_id);
       return -1;
    }

    if(replace)
    {
        oss << "REPLACE";
    }
    else
    {
        oss << "INSERT";
    }

    oss << " INTO " << table << " "<< db_names <<" VALUES ("
        <<          oid             << ","
        <<          uid             << ","
        << "'" <<   sql_name        << "',"
        <<          last_poll       << ","
        <<          vm_template.id  << ","
        <<          state           << ","
        <<          lcm_state       << ","
        <<          stime           << ","
        <<          etime           << ","
        << "'" <<   sql_deploy_id   << "',"
        <<          memory          << ","
        <<          cpu             << ","
        <<          net_tx          << ","
        <<          net_rx          << ","
        <<          last_seq        << ")";

    db->free_str(sql_deploy_id);
    db->free_str(sql_name);

    rc = db->exec(oss);

    return rc;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int VirtualMachine::dump(ostringstream& oss,int num,char **values,char **names)
{
    if ((!values[OID])||
        (!values[UID])||
        (!values[NAME]) ||
        (!values[LAST_POLL])||
        (!values[STATE])||
        (!values[LCM_STATE])||
        (!values[STIME])||
        (!values[ETIME])||
        (!values[DEPLOY_ID])||
        (!values[MEMORY])||
        (!values[CPU])||
        (!values[NET_TX])||
        (!values[NET_RX])||
        (!values[LAST_SEQ])||
        (num != (LIMIT + History::LIMIT + 1)))
    {
        return -1;
    }

    oss <<
        "<VM>" <<
            "<ID>"       << values[OID]      << "</ID>"       <<
            "<UID>"      << values[UID]      << "</UID>"      <<
            "<USERNAME>" << values[LIMIT]     << "</USERNAME>"<<
            "<NAME>"     << values[NAME]     << "</NAME>"     <<
            "<LAST_POLL>"<< values[LAST_POLL]<< "</LAST_POLL>"<<
            "<STATE>"    << values[STATE]    << "</STATE>"    <<
            "<LCM_STATE>"<< values[LCM_STATE]<< "</LCM_STATE>"<<
            "<STIME>"    << values[STIME]    << "</STIME>"    <<
            "<ETIME>"    << values[ETIME]    << "</ETIME>"    <<
            "<DEPLOY_ID>"<< values[DEPLOY_ID]<< "</DEPLOY_ID>"<<
            "<MEMORY>"   << values[MEMORY]   << "</MEMORY>"   <<
            "<CPU>"      << values[CPU]      << "</CPU>"      <<
            "<NET_TX>"   << values[NET_TX]   << "</NET_TX>"   <<
            "<NET_RX>"   << values[NET_RX]   << "</NET_RX>"   <<
            "<LAST_SEQ>" << values[LAST_SEQ] << "</LAST_SEQ>";

    History::dump(oss, num-LIMIT-1, values+LIMIT+1, names+LIMIT+1);

    oss << "</VM>";

    return 0;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void VirtualMachine::add_history(
    int     hid,
    string& hostname,
    string& vm_dir,
    string& vmm_mad,
    string& tm_mad)
{
    ostringstream os;
    int           seq;

    if (history == 0)
    {
        seq = 0;
    }
    else
    {
        seq = history->seq + 1;

        if (previous_history != 0)
        {
            delete previous_history;
        }

        previous_history = history;
    }

    last_seq = seq;

    history = new History(oid,seq,hid,hostname,vm_dir,vmm_mad,tm_mad);
};

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void VirtualMachine::cp_history()
{
    History * htmp;

    if (history == 0)
    {
        return;
    }

    htmp = new History(oid,
            history->seq + 1,
            history->hid,
            history->hostname,
            history->vm_dir,
            history->vmm_mad_name,
            history->tm_mad_name);

    if ( previous_history != 0 )
    {
        delete previous_history;
    }

    previous_history = history;

    history = htmp;

    last_seq = history->seq;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void VirtualMachine::cp_previous_history()
{
    History * htmp;

    if ( previous_history == 0 || history == 0)
    {
        return;
    }

    htmp = new History(oid,
            history->seq + 1,
            previous_history->hid,
            previous_history->hostname,
            previous_history->vm_dir,
            previous_history->vmm_mad_name,
            previous_history->tm_mad_name);

    delete previous_history;

    previous_history = history;

    history = htmp;

    last_seq = history->seq;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void VirtualMachine::get_requirements (int& cpu, int& memory, int& disk)
{
    string          scpu;
    istringstream   iss;
    float           fcpu;

    get_template_attribute("MEMORY",memory);
    get_template_attribute("CPU",scpu);

    if ((memory == 0) || (scpu==""))
    {
        cpu    = 0;
        memory = 0;
        disk   = 0;

        return;
    }

    iss.str(scpu);
    iss >> fcpu;

    cpu    = (int) (fcpu * 100);//now in 100%
    memory = memory * 1024;     //now in bytes
    disk   = 0;

    return;
}
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int VirtualMachine::get_disk_images()
{
    int                   num_disks, rc;
    vector<Attribute  * > disks;
    ImagePool *           ipool;
    VectorAttribute *     disk;

    Nebula& nd = Nebula::instance();
    ipool      = nd.get_ipool();

    num_disks  = vm_template.get("DISK",disks);

    for(int i=0, index=0; i<num_disks; i++)
    {

        disk = dynamic_cast<VectorAttribute * >(disks[i]);

        if ( disk == 0 )
        {
            continue;
        }

        rc = ipool->disk_attribute(disk, &index);

        if (rc == -1) // 0 OK, -2 not using the Image pool
        {
            return -1;
        }
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void VirtualMachine::release_disk_images()
{
    string  iid;
    int     num_disks;

    vector<Attribute const  * > disks;
    Image *                     img;
    ImagePool *                 ipool;

    Nebula& nd = Nebula::instance();
    ipool      = nd.get_ipool();

    num_disks   = get_template_attribute("DISK",disks);

    for(int i=0; i<num_disks; i++)
    {
        VectorAttribute const *  disk =
            dynamic_cast<VectorAttribute const * >(disks[i]);

        if ( disk == 0 )
        {
            continue;
        }

        iid = disk->vector_value("IID");

        if ( iid.empty() )
        {
            continue;
        }

        img = ipool->get(atoi(iid.c_str()),true);

        if ( img == 0 )
        {
            continue;
        }

        img->release_image();

        img->unlock();
    }
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int VirtualMachine::get_network_leases()
{
    int                   num_nics, rc;
    vector<Attribute  * > nics;
    VirtualNetworkPool *  vnpool;
    VectorAttribute *     nic;

    Nebula& nd = Nebula::instance();
    vnpool     = nd.get_vnpool();

    num_nics   = vm_template.get("NIC",nics);

    for(int i=0; i<num_nics; i++)
    {
        nic = dynamic_cast<VectorAttribute * >(nics[i]);

        if ( nic == 0 )
        {
            continue;
        }

        rc = vnpool->nic_attribute(nic, oid);

        if (rc == -1)
        {
            return -1;
        }
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void VirtualMachine::release_network_leases()
{
    Nebula& nd = Nebula::instance();

    VirtualNetworkPool * vnpool = nd.get_vnpool();

    string                        vnid;
    string                        ip;
    int                           num_nics;

    vector<Attribute const  * >   nics;
    VirtualNetwork          *     vn;

    num_nics   = get_template_attribute("NIC",nics);

    for(int i=0; i<num_nics; i++)
    {
        VectorAttribute const *  nic =
            dynamic_cast<VectorAttribute const * >(nics[i]);

        if ( nic == 0 )
        {
            continue;
        }

        vnid = nic->vector_value("VNID");

        if ( vnid.empty() )
        {
            continue;
        }

        ip   = nic->vector_value("IP");

        if ( ip.empty() )
        {
            continue;
        }

        vn = vnpool->get(atoi(vnid.c_str()),true);

        if ( vn == 0 )
        {
            continue;
        }

        vn->release_lease(ip);
        vn->unlock();
    }
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int VirtualMachine::generate_context(string &files)
{
    ofstream file;

    vector<const Attribute*> attrs;
    const VectorAttribute *  context;

    map<string, string>::const_iterator it;

    files = "";

    if ( history == 0 )
        return -1;

    if ( get_template_attribute("CONTEXT",attrs) != 1 )
    {
        log("VM", Log::INFO, "Virtual Machine has no context");
        return 0;
    }

    file.open(history->context_file.c_str(),ios::out);

    if (file.fail() == true)
    {
        ostringstream oss;

        oss << "Could not open context file: " << history->context_file;
        log("VM", Log::ERROR, oss);
        return -1;
    }

    context = dynamic_cast<const VectorAttribute *>(attrs[0]);

    if (context == 0)
    {
        file.close();
        return -1;
    }

    files = context->vector_value("FILES");

    const map<string, string> values = context->value();

    file << "# Context variables generated by OpenNebula\n";

    for (it=values.begin(); it != values.end(); it++ )
    {
        file << it->first <<"=\""<< it->second << "\"" << endl;
    }

    file.close();

    return 1;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int VirtualMachine::parse_template_attribute(const string& attribute,
                                             string&       parsed)
{
    int rc;
    char * err = 0;

    rc = parse_attribute(this,-1,attribute,parsed,&err);

    if ( rc != 0 && err != 0 )
    {
        ostringstream oss;

        oss << "Error parsing: " << attribute << ". " << err;
        log("VM",Log::ERROR,oss);
    }

    return rc;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

pthread_mutex_t VirtualMachine::lex_mutex = PTHREAD_MUTEX_INITIALIZER;

extern "C"
{
    typedef struct yy_buffer_state * YY_BUFFER_STATE;

    int vm_var_parse (VirtualMachine * vm,
                      int              vm_id,
                      ostringstream *  parsed,
                      char **          errmsg);

    int vm_var_lex_destroy();

    YY_BUFFER_STATE vm_var__scan_string(const char * str);

    void vm_var__delete_buffer(YY_BUFFER_STATE);
}

/* -------------------------------------------------------------------------- */

int VirtualMachine::parse_attribute(VirtualMachine * vm,
                                    int              vm_id,
                                    const string&    attribute,
                                    string&          parsed,
                                    char **          error_msg)
{
    YY_BUFFER_STATE  str_buffer = 0;
    const char *     str;
    int              rc;
    ostringstream    oss_parsed;

    *error_msg = 0;

    pthread_mutex_lock(&lex_mutex);

    str        = attribute.c_str();
    str_buffer = vm_var__scan_string(str);

    if (str_buffer == 0)
    {
        goto error_yy;
    }

    rc = vm_var_parse(vm,vm_id,&oss_parsed,error_msg);

    vm_var__delete_buffer(str_buffer);

    vm_var_lex_destroy();

    pthread_mutex_unlock(&lex_mutex);

    parsed = oss_parsed.str();

    return rc;

error_yy:
    *error_msg=strdup("Error setting scan buffer");
    pthread_mutex_unlock(&lex_mutex);
    return -1;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

ostream& operator<<(ostream& os, const VirtualMachine& vm)
{
	string vm_str;

		os << vm.to_xml(vm_str);

    return os;
};

/* -------------------------------------------------------------------------- */

string& VirtualMachine::to_xml(string& xml) const
{
	string template_xml;
    string history_xml;

	ostringstream	oss;

	oss << "<VM>"
	      << "<ID>"       << oid       << "</ID>"
	      << "<UID>"       << uid       << "</UID>"
          << "<NAME>"      << name      << "</NAME>"
	      << "<LAST_POLL>" << last_poll << "</LAST_POLL>"
	      << "<STATE>"     << state     << "</STATE>"
	      << "<LCM_STATE>" << lcm_state << "</LCM_STATE>"
	      << "<STIME>"     << stime     << "</STIME>"
	      << "<ETIME>"     << etime     << "</ETIME>"
          << "<DEPLOY_ID>" << deploy_id << "</DEPLOY_ID>"
	      << "<MEMORY>"    << memory    << "</MEMORY>"
	      << "<CPU>"       << cpu       << "</CPU>"
	      << "<NET_TX>"    << net_tx    << "</NET_TX>"
	      << "<NET_RX>"    << net_rx    << "</NET_RX>"
          << "<LAST_SEQ>"  << last_seq  << "</LAST_SEQ>"
          << vm_template.to_xml(template_xml);

    if ( hasHistory() )
    {
        oss << history->to_xml(history_xml);
    }
	oss << "</VM>";

	xml = oss.str();

	return xml;
}

/* -------------------------------------------------------------------------- */

string& VirtualMachine::to_str(string& str) const
{
	string template_str;
    string history_str;

	ostringstream	oss;

	oss<< "ID                : " << oid << endl
       << "UID               : " << uid << endl
       << "NAME              : " << name << endl
       << "STATE             : " << state << endl
       << "LCM STATE         : " << lcm_state << endl
       << "DEPLOY ID         : " << deploy_id << endl
       << "MEMORY            : " << memory << endl
       << "CPU               : " << cpu << endl
       << "LAST POLL         : " << last_poll << endl
       << "START TIME        : " << stime << endl
       << "STOP TIME         : " << etime << endl
       << "NET TX            : " << net_tx << endl
       << "NET RX            : " << net_rx << endl
       << "LAST SEQ          : " << last_seq << endl
       << "Template" << endl << vm_template.to_str(template_str) << endl;

    if ( hasHistory() )
    {
        oss << "Last History Record" << endl << history->to_str(history_str);
    }

	str = oss.str();

	return str;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
