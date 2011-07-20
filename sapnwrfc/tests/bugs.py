"""
Contains tests created to track down bugs
"""
import logging
import os
import sapnwrfc
import unittest
from copy import copy
from sapnwrfc import RFCException

log = logging.getLogger(__name__)


class ConnectionProblemTestCase(unittest.TestCase):

    def setUp(self):
        logging.basicConfig(level=logging.DEBUG)
        
        # REFACTOR: find better way to inject configuration
        sapnwrfc.base.config_location = os.path.expanduser('~/.sapnwrfc.yml')
        sapnwrfc.base.load_config()
        
    
    def test_reconnection(self):
        """Checking re-connection issue"""
        
        def check_connection():
            log.debug("making a new connection")

            # create connection handle
            conn = sapnwrfc.base.rfc_connect()
            self.assertNotEquals(conn, None)
            
            # discover function module
            fd = conn.discover("RFC_READ_TABLE")
            self.assertEquals(fd.name, "RFC_READ_TABLE")
            
            # create call
            f = fd.create_function_call()
            self.assertEquals(f.name, "RFC_READ_TABLE")
            
            # set parameters
            f.QUERY_TABLE("TRDIR")
            f.ROWCOUNT(50)
            f.OPTIONS( [{ 'TEXT': "NAME LIKE 'RS%'"}] )
            
            # invoke function module
            f.invoke()
            
            
            d = f.DATA.value
            log.debug("NO. PROGS: %d" % len(f.DATA()))
            
            self.assertEquals(len(f.DATA()), 50)
            self.assertEquals(conn.close(), 1)

        # try it twice        
        check_connection()
        check_connection()
        
        
    def test_reconnect(self):
        """Checking re-using the connection"""
        
        cfg = copy(sapnwrfc.base.configuration)

        call_cfg = {
            'user': cfg['user'],
            'passwd': cfg['passwd'],
            }
        
        def call_rfc(conn):
            """Call some rfc"""
            # discover function module
            
            fd = conn.discover("RFC_READ_TABLE")
            f = fd.create_function_call()

            # set parameters
            f.QUERY_TABLE("TRDIR")
            f.ROWCOUNT(50)
            f.OPTIONS( [{ 'TEXT': "NAME LIKE 'RS%'"}] )
            
            # invoke function module
            f.invoke()
            
            return f
        
        
        ref_holder = []    
        
        conn = sapnwrfc.base.rfc_connect(call_cfg)
        
        d = call_rfc(conn).DATA.value
        ref_holder.append(d)
        
        conn2 = sapnwrfc.base.rfc_connect(call_cfg)
        
        d = call_rfc(conn2).DATA.value
        ref_holder.append(d)
        d = call_rfc(conn).DATA.value
        ref_holder.append(d)
        
        conn.close()
        
        self.assertRaises(RFCException, call_rfc, conn)

        conn = sapnwrfc.base.rfc_connect(call_cfg)

        d = call_rfc(conn).DATA.value
        ref_holder.append(d)
        d = call_rfc(conn2).DATA.value
        ref_holder.append(d)
        
        conn2.close()
        
        d = call_rfc(conn).DATA.value
        ref_holder.append(d)
        
        
        
