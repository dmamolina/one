require 'OpenNebula'

include OpenNebula

class VirtualMachineOCCI < VirtualMachine
    # Creates the VMI representation of a Virtual Machine
    def to_occi
        occi_xml  = "<COMPUTE>"
        occi_xml += "<ID>" + id.to_s + "</ID>"
        occi_xml += "<NAME>" + self['NAME'] + "</NAME>"
        occi_xml += "<TYPE>" + self['OCCI_SIZE_TYPE'] + "</TYPE>" if self['OCCI_SIZE_TYPE']
        occi_xml += "<STATE>" + state_str + "</STATE>"
        
        # Now let's parse the template
        template=self.to_hash("TEMPLATE")
        
        template['DISK']=[template['DISK']].flatten
        
         if template['DISK']
             
             occi_xml += "<STORAGE>"
        
            template['DISK'].each{|disk|
                case disk['TYPE']
                    when "disk" then
                        occi_xml += "<DISK image=\"#{disk['IMAGE_ID']}\" dev=\"#{disk['DEV']}\"/>"
                    when "swap" then
                        occi_xml += "<SWAP size=\"#{disk['SIZE']}\" dev=\"#{disk['DEV']}\"/>"
                    when "fs" then
                        occi_xml += "<FS size=\"#{disk['SIZE']}\" format=\"#{disk['FORMAT']}\" dev=\"#{disk['DEV']}\"/>"
                end
            }
        
            occi_xml += "</STORAGE>"
        end 
        
        template['NIC']=[template['NIC']].flatten
                 
        if template['NIC']
            occi_xml += "<NETWORK>" 
        
            template['NIC'].each{|nic|
                
                occi_xml += "<NIC network=\"#{nic['VNID']}\""
                if nic['IP']
                     occi_xml += " ip=\"#{nic['IP']}\""
                end
                occi_xml += "/>"
            }
        
            occi_xml += "</NETWORK>" 
        end
        
        occi_xml  += "</COMPUTE>"
        
        return occi_xml

    end
end

if $0 == __FILE__
    t=VirtualMachineOCCI.new(VirtualMachine.build_xml(6),Client.new("tinova:opennebula"))
 #   t=VirtualMachineOCCI.new(VirtualMachine.build_xml,nil)
    t.info
    puts t.to_occi
end



