#!/usr/bin/python2.3

# rfcclient.py
# Calls RFM Z_FOO_HELLOWORLD in SAP

if __name__ == "__main__":
#   bunf to sort out the library path
	import sys
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


#   This is where it really begins

import saprfc
import sys

if len(sys.argv) > 1:
	name = sys.argv[1]
else:
	name = "EuroFoo"

conn = saprfc.conn(ashost='/H/drs00',
sysnr='00',lang='EN',client='200',user='harding_p',passwd='tw4ddle')
conn.connect()

helloworld = conn.discover("Z_RFC_TEST")
helloworld.NAME.setValue(name)
conn.callrfc(helloworld)

print helloworld.MESSAGE.getValue()
conn.close()
