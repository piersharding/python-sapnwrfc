#!/usr/bin/python
# -*- coding: utf-8 -*-
import unittest


class sapnwrfctest(unittest.TestCase):

  def setUp(self):
    
    sapnwrfc.base.config_location = 'examples/sap.yml'
    sapnwrfc.base.load_config()


  def testConn1(self):
    print "testConn1"
    conn = sapnwrfc.base.rfc_connect({'user': 'developer', 'passwd': 'developer'})
    self.assertNotEquals(conn, None)
    #print "connection attributes: ", conn.connection_attributes()
    attr = conn.connection_attributes()
    self.assertEquals(attr['partnerHost'], 'gecko')
    self.assertEquals(conn.close(), 1)

  def testConn2(self):
    print "testConn2"
    conn = sapnwrfc.base.rfc_connect()
    self.assertNotEquals(conn, None)
    #print "connection attributes: ", conn.connection_attributes()
    attr = conn.connection_attributes()
    self.assertEquals(attr['partnerHost'], 'gecko')
    self.assertEquals(conn.close(), 1)

  def testConn3(self):
    print "testConn3"
    for i in range(25):
      #print "making a new connection (%d)." % i
      conn = sapnwrfc.base.rfc_connect()
      self.assertNotEquals(conn, None)
      attr = conn.connection_attributes()
      self.assertEquals(attr['partnerHost'], 'gecko')
      self.assertEquals(conn.close(), 1)

  def testFuncDesc1(self):
    print "testFuncDesc1"
    for i in range(10):
      #print "making a new connection (%d)." % i
      conn = sapnwrfc.base.rfc_connect()
      self.assertNotEquals(conn, None)
      for j in ("STFC_CHANGING", "STFC_XSTRING", "RFC_READ_TABLE", "RFC_READ_REPORT", "RPY_PROGRAM_READ", "RFC_PING", "RFC_SYSTEM_INFO"):
        #print "discover: %s" % j
        fd = conn.discover(j)
        self.assertEquals(fd.name, j)
      self.assertEquals(conn.close(), 1)

  def testFuncDesc2(self):
    print "testFuncDesc2"
    for i in range(10):
      #print "making a new connection (%d)." % i
      conn = sapnwrfc.base.rfc_connect()
      self.assertNotEquals(conn, None)
      for j in ("STFC_CHANGING", "STFC_XSTRING", "RFC_READ_TABLE", "RFC_READ_REPORT", "RPY_PROGRAM_READ", "RFC_PING", "RFC_SYSTEM_INFO"):
        #print "create_function_call: %s" % j
        fd = conn.discover(j)
        self.assertEquals(fd.name, j)
        f = fd.create_function_call()
        self.assertEquals(f.name, j)
      self.assertEquals(conn.close(), 1)

  def testFuncCall1(self):
    print "testFuncCall1"
    for i in range(10):
      #print "making a new connection (%d)." % i
      conn = sapnwrfc.base.rfc_connect()
      self.assertNotEquals(conn, None)
      fd = conn.discover("RFC_READ_TABLE")
      self.assertEquals(fd.name, "RFC_READ_TABLE")
      f = fd.create_function_call()
      self.assertEquals(f.name, "RFC_READ_TABLE")
      f.QUERY_TABLE("TRDIR")
      f.ROWCOUNT(50)
      f.OPTIONS( [{ 'TEXT': "NAME LIKE 'RS%'"}] )
      f.invoke()
      d = f.DATA.value
      #print "NO. PROGS: ", len(f.DATA())
      self.assertEquals(len(f.DATA()), 50)
      self.assertEquals(conn.close(), 1)

  def testFuncCall2(self):
    print "testFuncCall2"
    conn = sapnwrfc.base.rfc_connect()
    self.assertNotEquals(conn, None)
    for i in range(25):
      fd = conn.discover("RFC_READ_TABLE")
      self.assertEquals(fd.name, "RFC_READ_TABLE")
      f = fd.create_function_call()
      self.assertEquals(f.name, "RFC_READ_TABLE")
      f.QUERY_TABLE("TRDIR")
      f.ROWCOUNT(50)
      f.OPTIONS( [{ 'TEXT': "NAME LIKE 'RS%'"}] )
      f.invoke()
      d = f.DATA.value
      self.assertEquals(len(f.DATA()), 50)
    self.assertEquals(conn.close(), 1)

  def testFuncCall3(self):
    print "testFuncCall3"
    conn = sapnwrfc.base.rfc_connect()
    self.assertNotEquals(conn, None)
    fd = conn.discover("RFC_READ_TABLE")
    self.assertEquals(fd.name, "RFC_READ_TABLE")
    for i in range(100):
      f = fd.create_function_call()
      self.assertEquals(f.name, "RFC_READ_TABLE")
      f.QUERY_TABLE("TRDIR")
      f.ROWCOUNT(50)
      f.OPTIONS( [{ 'TEXT': "NAME LIKE 'RS%'"}] )
      f.invoke()
      d = f.DATA.value
      self.assertEquals(len(f.DATA()), 50)
    self.assertEquals(conn.close(), 1)

  def testFuncCall4(self):
    print "testFuncCall4"
    conn = sapnwrfc.base.rfc_connect()
    self.assertNotEquals(conn, None)
    fd = conn.discover("RPY_PROGRAM_READ")
    self.assertEquals(fd.name, "RPY_PROGRAM_READ")
    for i in range(100):
      f = fd.create_function_call()
      self.assertEquals(f.name, "RPY_PROGRAM_READ")
      f.PROGRAM_NAME("SAPLGRFC")
      self.assertEquals(f.PROGRAM_NAME(), "SAPLGRFC")
      f.invoke()
      p = f.PROG_INF.value
      self.assertEquals(p['PROGNAME'].rstrip(), "SAPLGRFC")
      p = f.SOURCE_EXTENDED.value
      self.failUnless(len(p) > 10)
    self.assertEquals(conn.close(), 1)

  def testData1(self):
    print "testData1"
    conn = sapnwrfc.base.rfc_connect()
    self.assertNotEquals(conn, None)
    fd = conn.discover("Z_TEST_DATA")
    self.assertEquals(fd.name, "Z_TEST_DATA")
    for i in range(100):
      f = fd.create_function_call()
      self.assertEquals(f.name, "Z_TEST_DATA")
      f.CHAR("German: öäüÖÄÜß")
      f.INT1(123)
      f.INT2(1234)
      f.INT4(123456)
      f.FLOAT(123456.00)
      f.NUMC('12345')
      f.DATE('20060709')
      f.TIME('200607')
      f.BCD(200607.123)
      f.ISTRUCT({ 'ZCHAR': "German: öäüÖÄÜß", 'ZINT1': 54, 'ZINT2': 134, 'ZIT4': 123456, 'ZFLT': 123456.00, 'ZNUMC': '12345', 'ZDATE': '20060709', 'ZTIME': '200607', 'ZBCD': '200607.123' })
      f.DATA( [ { 'ZCHAR': "German: öäüÖÄÜß", 'ZINT1': 54, 'ZINT2': 134, 'ZIT4': 123456, 'ZFLT': 123456.00, 'ZNUMC': '12345', 'ZDATE': '20060709', 'ZTIME': '200607', 'ZBCD': '200607.123' },
                { 'ZCHAR': "German: öäüÖÄÜß", 'ZINT1': 54, 'ZINT2': 134, 'ZIT4': 123456, 'ZFLT': 123456.00, 'ZNUMC': '12345', 'ZDATE': '20060709', 'ZTIME': '200607', 'ZBCD': '200607.123' },
                { 'ZCHAR': "German: öäüÖÄÜß", 'ZINT1': 54, 'ZINT2': 134, 'ZIT4': 123456, 'ZFLT': 123456.00, 'ZNUMC': '12345', 'ZDATE': '20060709', 'ZTIME': '200607', 'ZBCD': '200607.123' },
                { 'ZCHAR': "German: öäüÖÄÜß", 'ZINT1': 54, 'ZINT2': 134, 'ZIT4': 123456, 'ZFLT': 123456.00, 'ZNUMC': '12345', 'ZDATE': '20060709', 'ZTIME': '200607', 'ZBCD': '200607.123' } ] )
      f.invoke()
      #print "CHAR: ", f.ECHAR()
      self.assertEquals(f.ECHAR().rstrip(), f.CHAR())
      #print "INT1: ", f.EINT1()
      self.assertEquals(f.EINT1(), f.INT1())
      #print "INT2: ", f.EINT2()
      self.assertEquals(f.EINT2(), f.INT2())
      #print "INT4: ", f.EINT4()
      self.assertEquals(f.EINT4(), f.INT4())
      #print "FLOAT: ", f.EFLOAT()
      self.assertEquals(f.EFLOAT(), f.FLOAT())
      #print "NUMC: ", f.ENUMC()
      self.assertEquals(f.ENUMC(), f.NUMC())
      #print "DATE: ", f.EDATE()
      self.assertEquals(f.EDATE(), f.DATE())
      #print "TIME: ", f.ETIME()
      self.assertEquals(f.ETIME(), f.TIME())
      #print "BCD: ", f.EBCD()
      self.assertEquals(f.EBCD(), f.BCD())
      #print "ESTRUCT: ", f.ESTRUCT()
      #print "DATA: ", f.DATA()
      #print "RESULTS: ", f.RESULT()
    self.assertEquals(conn.close(), 1)

  def testChanging1(self):
    print "testChanging1"
    conn = sapnwrfc.base.rfc_connect()
    self.assertNotEquals(conn, None)
    fd = conn.discover("STFC_CHANGING")
    self.assertEquals(fd.name, "STFC_CHANGING")
    for i in range(100):
      f = fd.create_function_call()
      self.assertEquals(f.name, "STFC_CHANGING")
      f.START_VALUE(i)
      f.COUNTER(i)
      f.invoke()
      self.failUnless(f.RESULT() == (i + i))
      self.failUnless(f.COUNTER() == (i + 1))
    self.assertEquals(conn.close(), 1)

  def testDeep1(self):
    print "testDeep1"
    conn = sapnwrfc.base.rfc_connect()
    self.assertNotEquals(conn, None)
    fds = conn.discover("STFC_DEEP_STRUCTURE")
    self.assertEquals(fds.name, "STFC_DEEP_STRUCTURE")
    fdt = conn.discover("STFC_DEEP_TABLE")
    self.assertEquals(fdt.name, "STFC_DEEP_TABLE")
    for i in range(100):
      fs = fds.create_function_call()
      self.assertEquals(fs.name, "STFC_DEEP_STRUCTURE")
      #fs.IMPORTSTRUCT( { 'I': 123, 'C': 'AbCdEf', 'STR': 'The quick brown fox ...', 'XSTR': self.hextranslate("deadbeef") } )
      fs.IMPORTSTRUCT( { 'I': 123, 'C': 'AbCdEf', 'STR': 'The quick brown fox ...', 'XSTR': "deadbeef".decode("hex") } )
      fs.invoke()
      s = fs.ECHOSTRUCT()
      self.failUnless(s['I'] == 123)
      self.failUnless(s['C'].rstrip() == 'AbCdEf')
      self.failUnless(s['STR'] == 'The quick brown fox ...')
      #self.failUnless(self.unhex(s['XSTR']) == 'deadbeef')
      self.failUnless(s['XSTR'].encode("hex") == 'deadbeef')
      ft = fdt.create_function_call()
      self.assertEquals(ft.name, "STFC_DEEP_TABLE")
      #ft.IMPORT_TAB( [{ 'I': 123, 'C': 'AbCdEf', 'STR': 'The quick brown fox ...', 'XSTR': self.hextranslate("deadbeef") }] )
      ft.IMPORT_TAB( [{ 'I': 123, 'C': 'AbCdEf', 'STR': 'The quick brown fox ...', 'XSTR': "deadbeef".decode("hex") }] )
      ft.invoke()
      t = ft.EXPORT_TAB()
      for r in t:
        #print "XSTR => %s" % self.unhex(r['XSTR'])
        #self.assertEquals(self.unhex(r['XSTR']), 'deadbeef')
        self.assertEquals(r['XSTR'].encode("hex"), 'deadbeef')
    self.assertEquals(conn.close(), 1)

  def testDeep2(self):
    print "testDeep2"
    conn = sapnwrfc.base.rfc_connect()
    self.assertNotEquals(conn, None)
    fds = conn.discover("STFC_DEEP_STRUCTURE")
    self.assertEquals(fds.name, "STFC_DEEP_STRUCTURE")
    fdt = conn.discover("STFC_DEEP_TABLE")
    self.assertEquals(fdt.name, "STFC_DEEP_TABLE")
    for i in range(100):
      fs = fds.create_function_call()
      self.assertEquals(fs.name, "STFC_DEEP_STRUCTURE")
      fs.IMPORTSTRUCT( { 'I': 123, 'C': 'AbCdEf', 'STR': 'German: öäüÖÄÜß', 'XSTR': "deadbeef".decode("hex") } )
      fs.invoke()
      s = fs.ECHOSTRUCT()
      self.failUnless(s['I'] == 123)
      self.failUnless(s['C'].rstrip() == 'AbCdEf')
      self.failUnless(s['STR'] == 'German: öäüÖÄÜß')
      self.failUnless(s['XSTR'].encode("hex") == 'deadbeef')
      ft = fdt.create_function_call()
      self.assertEquals(ft.name, "STFC_DEEP_TABLE")
      ft.IMPORT_TAB( [{ 'I': 123, 'C': 'AbCdEf', 'STR': 'German: öäüÖÄÜß', 'XSTR': "deadbeef".decode("hex") }] )
      ft.invoke()
      t = ft.EXPORT_TAB()
      for r in t:
        self.assertEquals(r['XSTR'].encode("hex"), 'deadbeef')
        if r['STR'][0:5] == 'German':
          self.assertEquals(r['STR'], 'German: öäüÖÄÜß')
    self.assertEquals(conn.close(), 1)


  def hextranslate(self, s):
    res = ""
    for i in range(len(s)/2):
      realIdx = i*2
      res = res + chr(int(s[realIdx:realIdx+2],16))
    return res


  def unhex(self, s):
    res = ""
    for i in range(len(s)):
      bit = "%02x" % int(ord(s[i]))
      res = res +  bit
    return res


if __name__ == "__main__":
  import sys
  if sys.version < '2.2':
    print "\n\n   You Must Have Python Version >= 2.2  To run saprfc \n\n"
    sys.exit(1)
  import os
  path = ""
  if 'build' in os.listdir(os.getcwd()):
    path = os.path.join(os.getcwd(), 'build')
  elif os.listdir(os.path.join(os.getcwd(), '../')):
    path = os.path.join(os.getcwd(), '../build')
  else:
    print "cant find ./build directory to load the saprfc module, try runnig from the package root dir"
    print "   looked in:", os.getcwd(), " and ", os.path.join(os.getcwd(), '../')
    sys.exit(1)

  libdir = ""
  for i in  os.listdir(path):
    if i.startswith("lib"):
      libdir = os.path.join(path, i)
  if libdir == "":
    print "cant find ./build directory to load the saprfc module, try runnig from the package root dir"
    print "   looked in:", os.getcwd(), " and ", os.path.join(os.getcwd(), '../')
    sys.exit(1)

  sys.path.append(libdir)

  print "using library path: " + libdir

  import sapnwrfc

  unittest.main()

