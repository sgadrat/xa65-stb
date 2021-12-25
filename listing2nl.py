#!/usr/bin/env python
import sys
import re

class AddressData:
    def __init__(self, address):
        self.address = address  # str version
        self.label = None
        self.comments = []  # list of lines

# address_data[address] = AddressData
address_data = {}

for line in sys.stdin:
    m = re.match(r'^(?P<line>[ 0-9]){5} A:(?P<address>[0-9a-f]{4})  (?P<instructions>.{24}) (?: {9}(?P<code>.*)|(?P<label>\w+) *)$', line)
    if not m:
        # empty line or filename
        continue

    address = m.group('address')
    if address not in address_data:
        address_data[address] = AddressData(address)
    data = address_data[address]

    if m.group('label'):
        data.label = m.group('label')
    else:
        data.comments.append(m.group('code'))

for address in sorted(address_data):
    data = address_data[address]
    print("$%s#%s#%s" % (address, data.label or '', '\n\\'.join(data.comments)))

