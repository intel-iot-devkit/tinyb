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

#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <inttypes.h>
#include <poll.h>

#include  <algorithm>

#include "HCITypes.hpp"

using namespace tinyb_hci;

#define AD_FLAGS_LIMITED_MODE_BIT 0x01
#define AD_FLAGS_GENERAL_MODE_BIT 0x02

#define AD_TYPE_FLAGS                   0x01  /* flags */
#define AD_TYPE_UUID16_SOME             0x02  /* 16-bit UUID, more available */
#define AD_TYPE_UUID16_ALL              0x03  /* 16-bit UUID, all listed */
#define AD_TYPE_UUID32_SOME             0x04  /* 32-bit UUID, more available */
#define AD_TYPE_UUID32_ALL              0x05  /* 32-bit UUID, all listed */
#define AD_TYPE_UUID128_SOME            0x06  /* 128-bit UUID, more available */
#define AD_TYPE_UUID128_ALL             0x07  /* 128-bit UUID, all listed */

#define AD_TYPE_NAME_SHORT              0x08  /* shortened local name */
#define AD_TYPE_NAME_COMPLETE           0x09  /* complete local name */
#define AD_TYPE_TX_POWER                0x0A  /* transmit power level */
#define AD_TYPE_DEVICE_ID               0x10  /* device ID */
#define AD_TYPE_MANUFACTURE_SPECIFIC    0xFF

#define HCI_LE_Advertising_Report   	0x3E

/**
 * See Bluetooth Core Specification V5.2 [Vol. 3, Part C, 11, p 1392]
 * and Bluetooth Core Specification Supplement V9, Part A: 1, p 9 + 2 Examples, p25..
 * and Assigned Numbers <https://www.bluetooth.com/specifications/assigned-numbers/>
 * <p>
 * https://www.bluetooth.com/specifications/archived-specifications/
 * </p>
 */
static int read_ad_struct_elem(uint8_t *ad_len, uint8_t *ad_type, uint8_t **ad_data, 
                               uint8_t *data, int offset, int size)
{
    if (offset < size) {
        uint8_t len = data[offset]; // covers: type + data, less len field itself

        if (len == 0) {
            return 0; // end of significant part
        }

        if (len + offset > size) {
            return -ENOENT;
        }

        *ad_type = data[offset + 1];
        *ad_data = data + offset + 2; // net data ptr
        *ad_len = len - 1; // less type -> net data length

        return offset + 1 + len; // next ad_struct offset: + len + type + data
    }
    return -ENOENT;
}

/**
 * See Bluetooth Core Specification V5.2 [Vol. 4, Part E, 7.7.65.2, p 2382] 
 * <p>
 * https://www.bluetooth.com/specifications/archived-specifications/
 * </p>
 */
bool HCIAdapter::discoverDevices(HCISession& session, int timeoutMS)
{
    bool ok = true;
    uint8_t buf[HCI_MAX_EVENT_SIZE];
    struct hci_filter nf, of;
    socklen_t olen;
    int len_read = -1;
    const int64_t t0 = getCurrentMilliseconds();

    if( !session.isOpen() ) {
        fprintf(stderr, "Session not open\n");
        return false;
    }

    olen = sizeof(of);
    if (getsockopt(session.dd(), SOL_HCI, HCI_FILTER, &of, &olen) < 0) {
        fprintf(stderr, "Could not get socket options\n");
        return false;
    }

    hci_filter_clear(&nf);
    hci_filter_set_ptype(HCI_EVENT_PKT, &nf);
    hci_filter_set_event(EVT_LE_META_EVENT, &nf);

    if (setsockopt(session.dd(), SOL_HCI, HCI_FILTER, &nf, sizeof(nf)) < 0) {
        fprintf(stderr, "Could not set socket options\n");
        return false;
    }

    int64_t t1, td;

    while ( ( ( t1 = getCurrentMilliseconds() ) - t0 ) < timeoutMS ) {
        uint8_t hci_type;
        hci_event_hdr *ehdr;
        evt_le_meta_event *meta;
        int num_reports, i;
        uint8_t *i_octets;
        uint8_t ra_evt_type[0x19];
        uint8_t ra_bdaddr_type[0x19];
        bdaddr_t ra_bdaddr[0x19];
        uint8_t ra_length_data[0x19];
        uint8_t *ra_data[0x19];
        uint8_t ra_rssi[0x19];
        std::string ra_address[0x19];
        std::string ra_name_short[0x19];
        std::string ra_name_complete[0x19];
        std::vector<std::shared_ptr<UUID>> services[0x19];

        if( timeoutMS ) {
            struct pollfd p;
            int n;

            p.fd = session.dd(); p.events = POLLIN;
            while ((n = poll(&p, 1, timeoutMS)) < 0) {
                if (errno == EAGAIN /* || errno == EINTR */ ) {
                    // cont temp unavail, but end on interruption
                    continue;
                }
                ok = false;
                goto done;
            }
            if (!n) {
                goto done; // timeout
            }
        }

        while ((len_read = read(session.dd(), buf, sizeof(buf))) < 0) {
            if (errno == EAGAIN /* || errno == EINTR */ ) {
                // cont temp unavail, but end on interruption
                continue;
            }
            ok = false;
            goto done;
        }

        t1 = getCurrentMilliseconds();
        td = t1 - t0;

        // HCI_LE_Advertising_Report == 0x3E == EVT_LE_META_EVENT
        hci_type = buf[0];
        fprintf(stderr, "[%7.7" PRId64"] hci-type 0x%.2X\n", td, hci_type);

        ehdr = (hci_event_hdr*)(void*) ( buf + HCI_TYPE_LEN );
        fprintf(stderr, "[%7.7" PRId64"] hci-event-hdr event 0x%.2X, plen %d\n",
            td, ehdr->evt, ehdr->plen);

        len_read -= (1 + HCI_EVENT_HDR_SIZE);
        meta = (evt_le_meta_event*)(void *) ( buf + ( HCI_TYPE_LEN + HCI_EVENT_HDR_SIZE ) );

        fprintf(stderr, "[%7.7" PRId64"] hci-subevent 0x%.2X, remaining-len %d\n",
            td, meta->subevent, len_read);

        //        0x3E                                                           0x02
        if ( HCI_LE_Advertising_Report != ehdr->evt || meta->subevent != EVT_LE_ADVERTISING_REPORT ) {
            continue; // next ..
        }

        num_reports = (int) meta->data[0];
        i_octets = meta->data + 1;
        fprintf(stderr, "[%7.7" PRId64"] num_reports %d\n", td, num_reports);

        if( 0 >= num_reports || num_reports > 0x19 ) {
            ok = false;
            continue; // oops ?
        }

        for(i = 0; i < num_reports && i < 0x19; i++) {
            ra_evt_type[i] = *i_octets++;
        }
        for(i = 0; i < num_reports && i < 0x19; i++) {
            ra_bdaddr_type[i] = *i_octets++;
        }
        for(i = 0; i < num_reports && i < 0x19; i++) {
            char addr[18];
            bacpy( &ra_bdaddr[i], (const bdaddr_t *)i_octets );
            i_octets += 6;

            ba2str( &ra_bdaddr[i], addr );
            ra_address[i] = std::string(addr);
            fprintf(stderr, "[%7.7" PRId64"]   Address[%d] %s\n", td, i, ra_address[i].c_str());
        }
        for(i = 0; i < num_reports && i < 0x19; i++) {
            ra_length_data[i] = *i_octets++;
        }
        for(i = 0; i < num_reports && i < 0x19; i++) {
            int offset = 0;
            uint8_t ad_len, ad_type, *ad_data;
            char name[30];
            size_t name_len;
            ra_data[i] = i_octets;
            i_octets += ra_length_data[i];
            fprintf(stderr, "[%7.7" PRId64"]   AD-Struct[%d] start: size %d\n", td, i, ra_length_data[i]);

            while( 0 < ( offset = read_ad_struct_elem( &ad_len, &ad_type, &ad_data, 
                               ra_data[i], offset, ra_length_data[i] ) ) )
            {
                fprintf(stderr, "read_ad_struct_elem: offset %d @ size %d: net ad_len %d, ad_type 0x%.2X\n",
                    offset, ra_length_data[i], ad_len, ad_type);

                // Guaranteed: ad_len >= 0!
                switch ( ad_type ) {
                    case AD_TYPE_UUID16_SOME:
                    case AD_TYPE_UUID16_ALL:
                        for(int j=0; j<ad_len/2; j++) {
                            const std::shared_ptr<UUID> uuid(new UUID16(ad_data, j*2, true));
                            services[i].push_back(std::move(uuid));
                        }
                        break;
                    case AD_TYPE_UUID32_SOME:
                    case AD_TYPE_UUID32_ALL:
                        for(int j=0; j<ad_len/4; j++) {
                            const std::shared_ptr<UUID> uuid(new UUID32(ad_data, j*4, true));
                            services[i].push_back(std::move(uuid));
                        }
                        break;
                    case AD_TYPE_UUID128_SOME:
                    case AD_TYPE_UUID128_ALL:
                        for(int j=0; j<ad_len/16; j++) {
                            const std::shared_ptr<UUID> uuid(new UUID128(ad_data, j*16, true));
                            services[i].push_back(std::move(uuid));
                        }
                        break;
                    case AD_TYPE_NAME_SHORT:
                    case AD_TYPE_NAME_COMPLETE:
                        memset(name, 0, sizeof(name));
                        name_len = std::min((size_t)ad_len, sizeof(name) - 1); // less EOS
                        memcpy(name, ad_data, name_len);
                        if( AD_TYPE_NAME_COMPLETE == ad_type ) {
                            ra_name_complete[i] = std::string(name);
                        } else {
                            ra_name_short[i] = std::string(name);
                        }
                        break;
                    case AD_TYPE_TX_POWER:
                    case AD_TYPE_DEVICE_ID:
                        // ???
                        break;
                }
            }
        }
        for(i = 0; i < num_reports && i < 0x19; i++) {
            ra_rssi[i] = *i_octets++;
        }
        for(i = 0; i < num_reports && i < 0x19; i++) {
            std::string & use_name = ra_name_complete[i].length()>0 ? 
                                     ra_name_complete[i] : ra_name_short[i];

            fprintf(stderr, "[%7.7" PRId64"] Report %d/%d, event-type 0x%.2X\n", td, i, num_reports, ra_evt_type[i]);
            fprintf(stderr, "[%7.7" PRId64"]   Address %s, name '%s' (short '%s')\n", 
                td, ra_address[i].c_str(), ra_name_complete[i].c_str(), ra_name_short[i].c_str());
            fprintf(stderr, "[%7.7" PRId64"]   RSSI %d\n", td, ra_rssi[i]);

            for(int j=0; j<services[i].size(); j++) {
                const std::shared_ptr<UUID> uuid = services[i].at(j);
                fprintf(stderr, "[%7.7" PRId64"]   UUID[%d] (%d bytes) %s\n", td, j, uuid->type, uuid->toUUID128String().c_str());
            }

            int idx = findDevice(ra_bdaddr[i]);
            std::shared_ptr<HCIDevice> dev;
            if( 0 > idx ) {
            	dev = std::shared_ptr<HCIDevice>(new HCIDevice(ra_bdaddr[i], use_name, ra_rssi[i]));
            	addDevice(dev);
            } else {
            	dev = getDevice(idx);
            	if( ra_name_complete[i].length() > 0 ) {
            		dev->setName(ra_name_complete[i]);
            	} else if( !dev->name.length() && ra_name_short[i].length() > 0 ) {
            		dev->setName(ra_name_short[i]);
            	}
                dev->setLastRSSI(ra_rssi[i]);
            }
            for(int j=0; j<services[i].size(); j++) {
                const std::shared_ptr<UUID> uuid = services[i].at(j);
                dev->addService(uuid);
            }
        }
    }

done:
    setsockopt(session.dd(), SOL_HCI, HCI_FILTER, &of, sizeof(of));
    return ok;
}

