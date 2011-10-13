#!/usr/bin/env /usr/bin/python
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

sapnwrfc.base.config_location = 'examples/sap.yml'
sapnwrfc.base.load_config()

for i in range(10):
  print("making a new connection (%d)." % i)
  #conn = sapnwrfc.base.rfc_connect({'user': 'developer', 'passwd': 'developer'})
  conn = sapnwrfc.base.rfc_connect()
  print("connection attributes: ", conn.connection_attributes())
  print("discover...")
  fd = conn.discover("RFC_READ_TABLE")
  print("finished discover...")
  f = fd.create_function_call()
  f.QUERY_TABLE("TRDIR")
  f.ROWCOUNT(50)
  f.OPTIONS( [{ 'TEXT': "NAME LIKE 'RS%'"}] )
  print("do the call...")
  f.invoke()
  d = f.DATA.value
  print("PROGS[0]: ", d[0], " \n")
  print("NO. PROGS: ", len(f.DATA()), " \n")
  conn.close()


print("making a new connection (%d)." % i)
#conn = sapnwrfc.base.rfc_connect({'user': 'developer', 'passwd': 'developer'})
conn = sapnwrfc.base.rfc_connect()
print("connection attributes: ", conn.connection_attributes())

for i in range(10):
  print("discover...")
  fd = conn.discover("RFC_READ_TABLE")
  print("finished discover...")
  f = fd.create_function_call()
  f.QUERY_TABLE("TRDIR")
  f.ROWCOUNT(50)
  f.OPTIONS( [{ 'TEXT': "NAME LIKE 'RS%'"}] )
  print("do the call...")
  f.invoke()
  d = f.DATA.value
  print("PROGS[0]: ", d[0], " \n")
  print("NO. PROGS: ", len(f.DATA()), " \n")

conn.close()


print("making a new connection (%d)." % i)
#conn = sapnwrfc.base.rfc_connect({'user': 'developer', 'passwd': 'developer'})
conn = sapnwrfc.base.rfc_connect()
print("connection attributes: ", conn.connection_attributes())
print("discover...")
fd = conn.discover("RFC_READ_TABLE")
print("finished discover...")

for i in range(10):
  f = fd.create_function_call()
  f.QUERY_TABLE("TRDIR")
  f.ROWCOUNT(50)
  f.OPTIONS( [{ 'TEXT': "NAME LIKE 'RS%'"}] )
  print("do the call...")
  f.invoke()
  d = f.DATA.value
  print("PROGS[0]: ", d[0], " \n")
  print("NO. PROGS: ", len(f.DATA()), " \n")

conn.close()
