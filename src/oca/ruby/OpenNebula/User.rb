# -------------------------------------------------------------------------- #
# Copyright 2002-2011, OpenNebula Project Leads (OpenNebula.org)             #
#                                                                            #
# Licensed under the Apache License, Version 2.0 (the "License"); you may    #
# not use this file except in compliance with the License. You may obtain    #
# a copy of the License at                                                   #
#                                                                            #
# http://www.apache.org/licenses/LICENSE-2.0                                 #
#                                                                            #
# Unless required by applicable law or agreed to in writing, software        #
# distributed under the License is distributed on an "AS IS" BASIS,          #
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   #
# See the License for the specific language governing permissions and        #
# limitations under the License.                                             #
#--------------------------------------------------------------------------- #

require 'OpenNebula/Pool'

module OpenNebula
    class User < PoolElement
        # ---------------------------------------------------------------------
        # Constants and Class Methods
        # ---------------------------------------------------------------------
        USER_METHODS = {
            :info     => "user.info",
            :allocate => "user.allocate",
            :delete   => "user.delete",
            :passwd   => "user.passwd"
        }

        # Creates a User description with just its identifier
        # this method should be used to create plain User objects.
        # +id+ the id of the user
        #
        # Example:
        #   user = User.new(User.build_xml(3),rpc_client)
        #
        def User.build_xml(pe_id=nil)
            if pe_id
                user_xml = "<USER><ID>#{pe_id}</ID></USER>"
            else
                user_xml = "<USER></USER>"
            end

            XMLElement.build_xml(user_xml, 'USER')
        end

        # ---------------------------------------------------------------------
        # Class constructor
        # ---------------------------------------------------------------------
        def initialize(xml, client)
            super(xml,client)

            @client = client
        end

        # ---------------------------------------------------------------------
        # XML-RPC Methods for the User Object
        # ---------------------------------------------------------------------
        
        # Retrieves the information of the given User.
        def info()
            super(USER_METHODS[:info], 'USER')
        end

        # Allocates a new User in OpenNebula
        #
        # +username+ Name of the new user.
        #
        # +password+ Password for the new user
        def allocate(username, password)
            super(USER_METHODS[:allocate], username, password)
        end

        # Deletes the User
        def delete()
            super(USER_METHODS[:delete])
        end

        # Changes the password of the given User
        #
        # +password+ String containing the new password
        def passwd(password)
            return Error.new('ID not defined') if !@pe_id

            rc = @client.call(USER_METHODS[:passwd], @pe_id, password)
            rc = nil if !OpenNebula.is_error?(rc)

            return rc
        end

    end
end
