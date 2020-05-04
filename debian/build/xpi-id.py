#!/usr/bin/python3

import sys
import json
import os
import zipfile

if __name__ == '__main__':
    if not len(sys.argv) == 2:
        print("Must specify an xpi", file=sys.stderr)
        exit(1)

    json_doc = json.loads(zipfile.ZipFile(sys.argv[1]).open('manifest.json').read().decode('utf-8').strip())
    gecko_id = json_doc["applications"]["gecko"]["id"]

    assert gecko_id
    print("%s" % gecko_id)
    exit(0)
