#!/usr/bin/python3

import sys
import os
import os.path
import uuid


def main(argv):
    if ((len(sys.argv) - 1) != 6):
        print('Error: fingerprint.py <tarfilename> <tarPath> <directorytoarchive> <uploadurl> <timestart> <timeend>');
        sys.exit(2);

    tarfile = os.path.join(sys.argv[2], sys.argv[1]) + '.tar';

    print('Fingerprint: ' + tarfile);

    # tar the directory
    os.system('tar -zcf ' + tarfile + ' ' + sys.argv[3]);

    # create sha256
    stream = os.popen('sha256sum ' + tarfile);
    sha_result = stream.read().split();

    # create json to send
    message = '{ "data": { "uniqueKey": "' + str(uuid.uuid1()) + '", "sha256": "' + sha_result[0] + '", "filename": "' + \
              sys.argv[1] + '.tar", "begin": "' + sys.argv[5] + '", "end": "' + sys.argv[6] + '"} }'

    # print( message );

    # post message to server
    stream = os.popen(
        'curl --insecure --header "Content-Type: application/json" --request POST --data \'' + message + '\' ' +
        sys.argv[4]);
    output = stream.read();
    print('finished fingerprint upload: ' + output);


if __name__ == "__main__":
    main(sys.argv[1:])
