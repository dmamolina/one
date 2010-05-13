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


#ifndef OBJECT_XML_H_
#define OBJECT_XML_H_

#include <string>
#include <vector>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

using namespace std;

/**
 *  This class represents a generic Object supported by a xml document.
 *  The class provides basic methods to query attributes, and get xml nodes
 */
class ObjectXML
{
public:

    // ---------------------- Constructors ------------------------------------

    ObjectXML():xml(0),ctx(0){};

    /**
     *  Constructs an object using a XML document
     */
    ObjectXML(const string &xml_doc);

    /**
     *  Constructs an object using a XML Node. The node is copied to the new
     *  object
     */
    ObjectXML(const xmlNodePtr node);

    virtual ~ObjectXML();

    /**
     *  Access Object elements using Xpath
     *    @param xpath_expr the Xpath of the element
     *    @return a vector with the elements
     */
    vector<string> operator[] (const char * xpath_expr);

    /**
     *  Get xml nodes by Xpath
     *    @param xpath_expr the Xpath for the elements
     *    @param content nodes for the given Xpath expression. The nodes are
     *    returned as pointers to the object nodes.
     *    @return the number of nodes found
     */
    int get_nodes (const char * xpath_expr, vector<xmlNodePtr>& content);

    /**
     *   Updates the object representation with a new XML document. Previous
     *   XML resources are freed
     *   @param xml_doc the new xml document
     */
    int update(const string &xml_doc);

private:
    /**
     *  XML representation of the Object
     */
    xmlDocPtr   xml;

    /**
     *  XPath Context to access Object elements
     */
    xmlXPathContextPtr ctx;

    /**
     *  Parse a XML documents and initializes XPath contexts
     */
    void xml_parse(const string &xml_doc);
};

#endif /*OBJECT_XML_H_*/
