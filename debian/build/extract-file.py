#!/usr/bin/python

import os
import sys
import os.path
import tarfile
import getopt
import tempfile
import shutil

if __name__ == '__main__':
    try:
        opts, args = getopt.getopt(sys.argv[1:], 'o:t:i:')
    except getopt.GetoptError, err:
        print str(err)
        exit(1)

    filename = None
    dest = None
    tarball = None

    for o, a in opts:
        if o == "-o":
            dest = os.path.abspath(a)
        elif o == "-t":
            tarball = os.path.abspath(a)
        elif o == "-i":
            filename = a

    if tarball == None:
        print >> sys.stderr, "Need to specify the tarball to extract from"
        exit(1)

    if filename == None:
        print >> sys.stderr, "Need to specify a source file"
        exit(1)

    if os.path.isabs(filename):
        print >> sys.stderr, "Specified filename should not be absolute"
        exit(1)

    if dest == None:
        dest = os.path.join(os.getcwd(), filename)

    if os.path.exists(dest):
        exit(0)

    if not os.path.isdir(os.path.dirname(dest)):
        os.makedirs(os.path.dirname(dest))

    tb = tarfile.open(tarball, 'r')
    names = tb.getnames()
    temp = None

    try:
        if os.path.join(names[0], filename) in names:
            temp = tempfile.mkdtemp()
            tb.extract(os.path.join(names[0], filename), temp)
            shutil.copyfile(os.path.join(temp, names[0], filename), dest)
    finally:
        if temp != None:
            shutil.rmtree(temp)
