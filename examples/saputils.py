# -*- coding: iso-8859-1 -*-
# Thomas Guettler 2011 http://www.thomas-guettler.de/
# This script is in the Public Domain

# Python
import sys

import sapnwrfc # https://github.com/piersharding/python-sapnwrfc

SAP_CONNECTION=dict(
    ashost='...', 
    sysnr='NN', 
    client='NNN', 
    user='myuser', 
    passwd='password', 
    lang='EN',
    )


dirs={1: 'IMPORT', 2: 'EXPORT', 3: 'CHANGING', 7: 'TABLES'} # See sapnwrfc.IMPORT, ...

sap_types={
    0: 'CHAR',
    1: 'DATE',
    2: 'BCD',
    3: 'TIME',
    4: 'BYTE',
    5: 'TABLE',
    6: 'NUM',
    7: 'FLOAT',
    8: 'INT',
    9: 'INT2',
    10: 'INT1',
    14: 'NULL',
    17: 'STRUCTURE',
    23: 'DECF16',
    24: 'DECF34',
    28: 'XMLDATA',
    29: 'STRING',
    30: 'XSTRING',
    98: 'EXCEPTION',
    }

def print_rfc_interface(method_name, conn=None):
    if conn is None:
        conn = sapnwrfc.base.rfc_connect(cfg=SAP_CONNECTION)
    iface = conn.discover(method_name)
    print(iface.name)
    for key, var_dict in sorted(iface.handle.parameters.items()):
        #Example: {'direction': 1, 'name': 'ARCHIV_DOC_ID', 'type': 0, 'len': 40, 'decimals': 0, 'ulen': 80}
        value=dict(var_dict)
        name=value.pop('name')
        assert key==name, (key, name)
        direction=value.pop('direction')
        direction=dirs.get(direction, direction)
        sap_type=value.pop('type')
        sap_type=sap_types.get(sap_type, sap_type)
        print(key, 'direction=%s type=%s len=%s decimals=%s ulen=%s rest=%s' % (direction, sap_type, value.pop('len'), value.pop('decimals'), value.pop('ulen'), value))

def main():
    print_rfc_interface(sys.argv[1])
if __name__=='__main__':
    main()
