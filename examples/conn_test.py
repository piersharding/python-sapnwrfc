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

print("making a new connection:")
try:
    conn = sapnwrfc.base.rfc_connect()
    print("connection attributes: ", conn.connection_attributes())
    conn.close()
except sapnwrfc.RFCCommunicationError:
    print("bang!")
