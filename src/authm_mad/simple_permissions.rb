
require 'quota'
require 'base64'

class SimplePermissions
    
    def initialize(database, client, conf={})
        @quota=Quota.new(database, client, conf[:quota] || {})
        @quota_enabled=conf[:quota][:enabled]
    end
    
    def auth_message(result, message)
        result ? true : message
    end
    
    def get_vm_usage(data)
        vm_xml=Base64::decode64(data)
        vm=OpenNebula::VirtualMachine.new(
            OpenNebula::XMLElement.build_xml(vm_xml, 'TEMPLATE'),
            OpenNebula::Client.new)
        
        # Should set more sensible defaults or get driver configuration
        cpu=vm['CPU']
        cpu||=1.0
        cpu=cpu.to_f
        
        memory=vm['MEMORY']
        memory||=64
        memory=memory.to_f
        
        VmUsage.new(cpu, memory)
    end
    
    def auth(uid, tokens)
        result=true
        
        tokens.each do |token|
            object, id, action, owner, pub=token.split(':')
            result=auth_object(uid.to_s, object, id, action, owner, pub)
            break result if result!=true
        end
        
        result
    end
    
    def auth_object(uid, object, id, action, owner, pub)
        return true if uid=='0'
        
        auth_result=false
        
        case action
        when 'CREATE'
            auth_result=true if %w{VM NET IMAGE}.include? object
            
            if @quota_enabled and object=='VM' and auth_result
                STDERR.puts 'quota enabled'
                @quota.update(uid.to_i)
                if !@quota.check(uid.to_i, get_vm_usage(id))
                    auth_result="Quota exceeded"
                end
            end
            
        when 'DELETE'
            auth_result = (owner == uid)
            
        when 'USE'
            if %w{VM NET IMAGE}.include? object
                auth_result = ((owner == uid) | (pub=='1'))
            elsif object == 'HOST'
                auth_result=true
            end
            
        when 'MANAGE'
            auth_result = (owner == uid)
            
        when 'INFO'
        end
        
        return auth_result
    end
end