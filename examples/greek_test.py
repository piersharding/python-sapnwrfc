#!/usr/bin/python
# -*- coding: utf-8 -*-
import unittest


class sapnwrfctest(unittest.TestCase):

  def setUp(self):
    
    sapnwrfc.base.config_location = 'examples/sap.yml'
    sapnwrfc.base.load_config()


  def testConn1(self):
    print("testConn1")
    conn = sapnwrfc.base.rfc_connect({'user': 'developer', 'passwd': 'developer'})
    self.assertNotEquals(conn, None)
    #print "connection attributes: ", conn.connection_attributes()
    attr = conn.connection_attributes()
    self.assertEquals(attr['partnerHost'], 'gecko')
    self.assertEquals(conn.close(), 1)

  def testDeep2(self):
    print("testDeep2")
    conn = sapnwrfc.base.rfc_connect()
    self.assertNotEquals(conn, None)
    fds = conn.discover("STFC_DEEP_STRUCTURE")
    self.assertEquals(fds.name, "STFC_DEEP_STRUCTURE")
    fdt = conn.discover("STFC_DEEP_TABLE")
    self.assertEquals(fdt.name, "STFC_DEEP_TABLE")
    for i in range(100):
      fs = fds.create_function_call()
      self.assertEquals(fs.name, "STFC_DEEP_STRUCTURE")
      fs.IMPORTSTRUCT( { 'I': 123, 'C': 'ΑΒΓΔΕΖΗΘΙΚ', 'STR': 'Greek: Μενέλαος Μαγκλής', 'XSTR': "deadbeef".decode("hex") } )
      s = fs.IMPORTSTRUCT()
      fs.invoke()
      s = fs.ECHOSTRUCT()
      self.failUnless(s['I'] == 123)
      self.failUnless(s['C'].rstrip() == 'ΑΒΓΔΕΖΗΘΙΚ')
      self.failUnless(s['STR'] == 'Greek: Μενέλαος Μαγκλής')
      self.failUnless(s['XSTR'].encode("hex") == 'deadbeef')
      ft = fdt.create_function_call()
      self.assertEquals(ft.name, "STFC_DEEP_TABLE")
      ft.IMPORT_TAB( [{ 'I': 123, 'C': 'ΑΒΓΔΕΖΗΘΙΚ', 'STR': 'Greek: Μενέλαος Μαγκλής', 'XSTR': "deadbeef".decode("hex") }] )
      ft.invoke()
      t = ft.EXPORT_TAB()
      self.assertEquals(t[0]['C'].rstrip(),'ΑΒΓΔΕΖΗΘΙΚ')
      self.assertEquals(t[0]['XSTR'].encode("hex"), 'deadbeef')
      if t[0]['STR'][0:5] == 'Greek':
        self.assertEquals(t[0]['STR'], 'Greek: Μενέλαος Μαγκλής')
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
    print("\n\n   You Must Have Python Version >= 2.2  To run saprfc \n\n")
    sys.exit(1)
  import os
  path = ""
  if 'build' in os.listdir(os.getcwd()):
    path = os.path.join(os.getcwd(), 'build')
  elif os.listdir(os.path.join(os.getcwd(), '../')):
    path = os.path.join(os.getcwd(), '../build')
  else:
    print("cant find ./build directory to load the saprfc module, try runnig from the package root dir")
    print("   looked in:", os.getcwd(), " and ", os.path.join(os.getcwd(), '../'))
    sys.exit(1)

  libdir = ""
  for i in  os.listdir(path):
    if i.startswith("lib"):
      libdir = os.path.join(path, i)
  if libdir == "":
    print("cant find ./build directory to load the saprfc module, try runnig from the package root dir")
    print("   looked in:", os.getcwd(), " and ", os.path.join(os.getcwd(), '../'))
    sys.exit(1)

  sys.path.append(libdir)

  print("using library path: " + libdir)

  import sapnwrfc

  unittest.main()
