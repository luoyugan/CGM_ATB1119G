# -*- coding: utf-8 -*-
import os
import re
import sys
import struct
import argparse

try:
    import xml.etree.cElementTree as ET
except ImportError:
    import xml.etree.ElementTree as ET




class XML_Parse(object):
    """
    XML file parser.
    """
    def __init__(self):
        self.xml_root       = None

    def open_xml(self, xml_handle):
        if os.path.isfile(xml_handle):
            tree            = ET.ElementTree(file = xml_handle)
            self.xml_root   = tree.getroot()
        elif isinstance(xml_handle, basestring):
            self.xml_root   = ET.fromstring(xml_handle)
        else:
            raise Exception("%r NOT a XML file or XML string" %xml_handle)

    def enumerate_all_subroot(self, root = None):
        """Enumerate all sub item of the root.
        """
        subroot_list = []
        if root is None:
            root = self.xml_root

        for sub_root in root.iter():
            # print(sub_root.tag, sub_root.attrib)
            subroot_list.append(sub_root)

        return subroot_list

    def enumerate_subroot(self, key, root = None):
        """Enumerate sub item of the root's first level.
        """
        if root is None:
            root = self.xml_root

        return root.findall(key)

    def indent(self, elem, level = 0):
        """Add indent for the xml string
        """
        i = "\n" + level * "    "
        if len(elem):
            if not elem.text or not elem.text.strip():
                elem.text = i + "    "
            if not elem.tail or not elem.tail.strip():
                elem.tail = i
            for elem in elem:
                xml_indent(elem, level + 1)
            if not elem.tail or not elem.tail.strip():
                elem.tail = i
        else:
            if level and (not elem.tail or not elem.tail.strip()):
                elem.tail = i



class ConfigXML(XML_Parse):
    """
    config xml file parser.
    """
    def __init__(self):
        super().__init__()

        self.cfg_key    = "config"
        self.item_key   = "item"

        self.config_name_key    = "name"
        self.config_id_key      = "cfg_id"
        self.config_size_key    = "size"

        self.item_name_key      = "name"
        self.item_offset_key    = "offs"
        self.item_size_key      = "size"

    def get_config_key_info(self):
        # info pattern: config name, config id, offset, size
        config_info_list = []

        cfg_element_list = self.enumerate_subroot(self.cfg_key)
        for cfg_root in cfg_element_list:
            config_name = cfg_root.attrib[self.config_name_key]
            config_id   = int(cfg_root.attrib[self.config_id_key], 0)
            config_size = int(cfg_root.attrib[self.config_size_key], 0)

            item_element_list = self.enumerate_subroot(self.item_key, cfg_root)
            for item_element in item_element_list:
                item_name   = item_element.attrib[self.item_name_key]
                item_offset = int(item_element.attrib[self.item_offset_key], 0)
                item_size   = int(item_element.attrib[self.item_size_key], 0)

                if (item_offset + item_size) > config_size:
                    print("Size of item %s in config %s exceed %dbytes"
                        %(item_name, config_name, item_offset + item_size - config_size))
                    break

                config_info_list.append(["%s_%s" %(config_name, item_name), config_id, item_offset, item_size])

        # print(config_info_list)
        return config_info_list

    def export_config_item_info(self, output_file):
        """Build driver header file, include all config item
        """
        head_macro = os.path.basename(output_file).split(".")[0].upper()
        config_info_list = self.get_config_key_info()

        with open(output_file, r'w', encoding = 'utf-8') as f_w:
            f_w.write("\n\n#ifndef __%s_H__\n#define __%s_H__\n\n" %(head_macro, head_macro))

            prv_id = -1
            for config_name, config_id, item_offset, item_size in config_info_list:
                key_value = ((config_id & 0xFF) << 24) + ((item_offset & 0xFFF) << 12) + (item_size & 0xFFF)
                if prv_id != config_id:
                    f_w.write("\n")

                prv_id = config_id
                f_w.write("#define CFG_%s %s(0x%08x)    // id:%3d, off:%4d, size:%4d\n"
                    %(config_name, ''.ljust(67 - len(config_name)), key_value, config_id, item_offset, item_size))

            f_w.write("\n\n#endif  // __%s_H__\n\n" %(head_macro))



def main(argv):
    parser = argparse.ArgumentParser(
        description='Export driver config key from config XML file.',
    )

    parser.add_argument('-i',   dest = 'cfg_xml_file',  required = True,                help = 'input config xml files')
    parser.add_argument('-o',   dest = 'output_file',   default = 'drv_config_head.h',  help = 'output driver config header file')

    args            = parser.parse_args()
    cfg_xml_file    = args.cfg_xml_file
    output_file     = args.output_file

    if not os.path.isfile(cfg_xml_file):
        print("%s NOT exist" %cfg_xml_file)
        sys.exit(1)

    cfg_xml = ConfigXML()
    cfg_xml.open_xml(cfg_xml_file)
    cfg_xml.export_config_item_info(output_file)


if __name__ == "__main__":
    main(sys.argv[1:])

