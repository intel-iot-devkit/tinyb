/*
 * Author: Sven Gothel <sgothel@jausoft.com>
 * Copyright (c) 2020 Gothel Software e.K.
 * Copyright (c) 2020 ZAFENA AB
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "HCITypes.hpp"

int main(int argc, char *argv[])
{
    int err = 0;
    std::string sensormac;

    for(int i=1; i<argc; i++) {
        if( !strcmp("-mac", argv[i]) && argc > (i+1) ) {
            sensormac = std::string(argv[++i]);
        }
    }
    if( 17 != sensormac.length() ) {
        fprintf(stderr, "Usage: %s -mac 00:00:00:00:00:00\n", argv[0]); 
        exit(1);
    }

    tinyb_hci::HCIAdapter adapter; // default
    if( !adapter.hasDevId() ) {
        fprintf(stderr, "Default adapter not available.\n");
        exit(1);
    }
    if( !adapter.isValid() ) {
        fprintf(stderr, "Adapter invalid.\n");
        exit(1);
    }
    fprintf(stderr, "Adapter: device %s, address %s\n", 
        adapter.getName().c_str(), adapter.getAddress().c_str());

    while( !err ) {
        std::shared_ptr<tinyb_hci::HCISession> session = adapter.startDiscovery();
        if( nullptr == session ) {
            fprintf(stderr, "Adapter start discovery failed.\n");
            exit(1);
        }

        // do something 
        if( !adapter.discoverDevices(*session, 10000) ) {
            fprintf(stderr, "Adapter discovery failed.\n");
            err = 1;
        }

        if( !adapter.stopDiscovery(*session) ) {
            fprintf(stderr, "Adapter stop discovery failed.\n");
            err = 1;
        }
    }

out:
    return err;
}

