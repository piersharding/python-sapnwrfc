
"""
Python utils for RFC calls to SAP NetWeaver System
"""

import sys
if sys.version < '2.4':
    print('Wrong Python Version (must be >=2.4) !!!')
    sys.exit(1)

from struct import *
from string import *
import re


# Parameter types
IMPORT = 1
EXPORT = 2
CHANGING = 3
TABLES = 7

# basic data types
CHAR = 0
DATE = 1
BCD = 2
TIME = 3
BYTE = 4
TABLE = 5
NUM = 6
FLOAT = 7
INT = 8
INT2 = 9
INT1 = 10
NULL = 14
STRUCTURE = 17
DECF16 = 23
DECF34 = 24
XMLDATA = 28
STRING = 29
XSTRING = 30
EXCEPTION = 98

# return codes
RFC_OK = 0
RFC_COMMUNICATION_FAILURE = 1
RFC_LOGON_FAILURE = 2
RFC_ABAP_RUNTIME_FAILURE = 3
RFC_ABAP_MESSAGE = 4
RFC_ABAP_EXCEPTION = 5
RFC_CLOSED = 6
RFC_CANCELED = 7
RFC_TIMEOUT = 8
RFC_MEMORY_INSUFFICIENT = 9
RFC_VERSION_MISMATCH = 10
RFC_INVALID_PROTOCOL = 11
RFC_SERIALIZATION_FAILURE = 12
RFC_INVALID_HANDLE = 13
RFC_RETRY = 14
RFC_EXTERNAL_FAILURE = 15
RFC_EXECUTED = 16
RFC_NOT_FOUND = 17
RFC_NOT_SUPPORTED = 18
RFC_ILLEGAL_STATE = 19
RFC_INVALID_PARAMETER = 20
RFC_CODEPAGE_CONVERSION_FAILURE = 21
RFC_CONVERSION_FAILURE = 22
RFC_BUFFER_TOO_SMALL = 23
RFC_TABLE_MOVE_BOF = 24
RFC_TABLE_MOVE_EOF = 25


class ParameterException(Exception):
    def __init__(self, value):
        self.value = value

    def __str__(self):
        return repr(self.value)


class Parameter:
    """
    Represents a RFC parameter.
    """

    def __init__(self, funcdesc=None, name=None, type=None, len=0, ulen=0, decimals=0, typedef=None):
        self.function_descriptor = funcdesc
        self.name = name
        self.type = type
        self.len = len
        self.ulen = ulen
        self.decimals = decimals
        self.typedef = typedef
        self.value = None
        if name == None:
            raise ParameterException("Missing parameter name")
        if type == None:
            raise ParameterException("Missing parameter type from %s" % name)

    def __str__(self):
        #print "get __str__\n"
        return self.name + ": " + repr(self.value)

    def __repr__(self):
        #print "get __repr__\n"
        return "<" + self.name + ": " + repr(self.value) + ">"

    def __call__(self, *args, **kwdargs):
        #print "get __call__\n"
        if len(args) > 0:
            self.Value(args[0])
        return self.value

    def __getattr__(self, name):
        return self.value

    # value setting for parameters - does basic Type checking to preserve
    # sanity for the underlying C extension
    def Value(self, val=None):
        if self.type == INT or self.type == INT2 or self.type == INT1:
            if not type(val) == int:
                raise ParameterException("Must be Fixnum for INT, INT1, and INT2 (%s/%d/%s)" % (self.name, self.type, type(val)))
        elif self.type == NUM:
            if not type(val) == str:
                raise ParameterException("Must be String for NUMC (%s/%d/%s)" % (self.name, self.type, type(val)))
        elif self.type == BCD:
            if not type(val) == float:
                raise ParameterException("Must be Float or *NUM for BCD (%s/%d/%s)" % (self.name, self.type, type(val)))
            val = str(val)
        elif self.type == FLOAT:
            if not type(val) == float:
                raise ParameterException("Must be Float for FLOAT (%s/%d/%s)" % (self.name, self.type, type(val)))
        elif self.type == STRING or self.type == XSTRING:
            if not type(val) == str:
                raise ParameterException("Must be String for STRING, and XSTRING (%s/%d/%s)" % (self.name, self.type, type(val)))
        elif self.type == BYTE:
            if not type(val) == str:
                raise ParameterException("Must be String for BYTE (%s/%d/%s)" % (self.name, self.type, type(val)))
        elif self.type == CHAR or self.type == DATE or self.type == TIME:
            if not type(val) == str:
                raise ParameterException("Must be String for CHAR, DATE, and TIME (%s/%d/%s)" % (self.name, self.type, type(val)))
        elif self.type == TABLE:
            if not type(val) == list:
                raise ParameterException("Must be Array for table value (%s/%s)" % (self.name, type(val)))
                cnt = 0
                for row in val:
                    cnt += 1
                    if not type(row) == dict:
                        raise ParameterException("Must be Hash for table row value (%s/%d/%s)" % (self.name, cnt, type(row)))
        elif self.type == STRUCTURE:
            if not type(val) == dict:
                raise ParameterException("Must be a Hash for a Structure Type (%s/%d/%s)" % (self.name, self.type, type(val)))
        else:
            raise ParameterException("unknown SAP data type (%s/%d)" % (self.name, self.type))
        self.value = val
        return val


class Import(Parameter):
    """
    A RFC import parameter
    """

    def __init__(self, funcdesc=None, name=None, type=None, len=0, ulen=0, decimals=0, typedef=None):
        Parameter.__init__(self, funcdesc, name, type, len, ulen, decimals, typedef)
        self.direction = IMPORT

class Export(Parameter):
    """
    A RFC export parameter
    """

    def __init__(self, funcdesc=None, name=None, type=None, len=0, ulen=0, decimals=0, typedef=None):
        Parameter.__init__(self, funcdesc, name, type, len, ulen, decimals, typedef)
        self.direction = EXPORT

class Changing(Parameter):
    """
    A RFC changing parameter
    """

    def __init__(self, funcdesc=None, name=None, type=None, len=0, ulen=0, decimals=0, typedef=None):
        Parameter.__init__(self, funcdesc, name, type, len, ulen, decimals, typedef)
        self.direction = CHANGING

class Table(Parameter):
    """
    A RFC table parameter
    """

    def __init__(self, funcdesc=None, name=None, type=None, len=0, ulen=0, decimals=0, typedef=None):
        Parameter.__init__(self, funcdesc, name, type, len, ulen, decimals, typedef)
        self.direction = TABLES

    # returns the no. of rows currently in the table
    def length(self):
        return len(self.value)

    # assign an Array, of rows represented by Hashes to the value of
    # the Table parameter.
    def Value(self, val=[]):
        if not type(val) == list:
            raise ParameterException("Must be list for table value (%s/%s)" % (self.name, type(val)))
            cnt = 0
            for row in val:
                cnt += 1
                if not type(row) == dict:
                    raise ParameterException("Must be Hash for table row value (%s/%d/%s)" % (self.name, cnt, type(row)))
        self.value = val

    # Yields each row of the table to passed Proc
    def each(self):
        if not self.value == None:
            for row in self.value:
                yield row

