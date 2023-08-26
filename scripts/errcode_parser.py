"""Generates enum values form postgres errcodes
Usage:
    errcode_parser enum-values
    errcode_parser switch-cases
    errcode_parser -h
"""
import requests
import base36
import docopt

if __name__ == '__main__':
    args = docopt.docopt(__doc__)

    response = requests.get('https://raw.githubusercontent.com/postgres/postgres/master/src/backend/utils/errcodes.txt')

    if response.status_code != 200:
        raise RuntimeError('Could not fetch errcodes.txt from the provided URL')

    sqlstates = {}
    content = response.text.splitlines()
    for line in content:
        line = line.strip()

        if not line or line.startswith('#'):
            continue

        if line.startswith('Section:'):
            continue

        fields = line.split()
        if len(fields) < 3:
            raise RuntimeError('Expected at least 3 field')

        sqlstate = fields[0]
        if len(fields) < 3:
            raise RuntimeError('sqlstate length must be exactly 5')
        sqlstate = base36.loads(sqlstate)

        errcode_macro_name = fields[2]
        if not errcode_macro_name.startswith('ERRCODE_'):
            raise RuntimeError('errcode_macro_name must start with ERRCODE_')

        errcode_macro_name = errcode_macro_name[8:].lower()
        if sqlstate not in sqlstates:
            sqlstates[sqlstate] = errcode_macro_name

    if args['enum-values']:
        for sqlstate, errcode_macro_name in sqlstates.items():
            print('{}={},'.format(errcode_macro_name, sqlstate))

    if args['switch-cases']:
        for sqlstate, errcode_macro_name in sqlstates.items():
            print('case sqlstate::{0}: return "{0}";'.format(errcode_macro_name))
