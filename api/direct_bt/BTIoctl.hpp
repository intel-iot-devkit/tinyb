/*
 * Copyright (C) 2000-2001  Qualcomm Incorporated
 * Written 2000,2001 by Maxim Krasnyansky <maxk@qualcomm.com>
 *
 * (Intentionally left out Linux Kernel license notice, see below.)
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) AND AUTHOR(S) BE LIABLE FOR ANY
 * CLAIM, OR ANY SPECIAL INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * ALL LIABILITY, INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PATENTS,
 * COPYRIGHTS, TRADEMARKS OR OTHER RIGHTS, RELATING TO USE OF THIS
 * SOFTWARE IS DISCLAIMED.
 *
 * ****************************************************************************************
 * ****************************************************************************************
 * ****************************************************************************************
 *
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
 *
 * ****************************************************************************************
 * ****************************************************************************************
 * ****************************************************************************************
 *
 * This file imports certain information from Linux Kernel's BlueZ protocol stack,
 * originating from Qualcomm's sources under MIT license.
 * This information enables the use of certain kernel services via system calls.
 * Therefore, the license of this file has been aligned with this project.
 *
 * Notice Qualcomm's original MIT license of the respective files.
 *
 * Also notice Linus Torvalds's Linux Kernel license exception regarding kernel syscalls (ioctl):
 * <https://github.com/torvalds/linux/blob/master/LICENSES/exceptions/Linux-syscall-note>
 * and therefor <https://www.kernel.org/doc/html/v4.20/process/license-rules.html>.
 *
 * Given the syscall usage and original file's MIT license,
 * we assume the following SPDX-License-Identifier should be declared:
 * "SPDX-License-Identifier: (GPL-2.0 WITH Linux-syscall-note) AND MIT"
 *
 * See file COPYING in the root folder of this project for more details.
 *
 * Original sources:
 * linux-kernel 4.19 include/net/bluetooth/bluetooth.h (git head 8d368fc58e7aeb42b39d7bec7c585efdfbc49074).
 *
 * Original copyright:
 * BlueZ - Bluetooth protocol stack for Linux
 * Copyright (C) 2000-2001 Qualcomm Incorporated
 * Written 2000,2001 by Maxim Krasnyansky <maxk@qualcomm.com>
 */

#ifndef BT_IOCTL_HPP_
#define BT_IOCTL_HPP_

#include "BTAddress.hpp"

#include "linux_kernel_types.hpp"

extern "C" {
    #include <stdint.h>
    #include <endian.h>
    #include <byteswap.h>
    #include <netinet/in.h> // Already exported named by OS
} /* extern "C" */

/**
 * Information from include/net/bluetooth/bluetooth.h
 */

#ifndef AF_BLUETOOTH
#define AF_BLUETOOTH	31
#define PF_BLUETOOTH	AF_BLUETOOTH
#endif

/* Bluetooth versions */
#define BLUETOOTH_VER_1_1	1
#define BLUETOOTH_VER_1_2	2
#define BLUETOOTH_VER_2_0	3

#define BTPROTO_L2CAP	0
#define BTPROTO_HCI	1
#define BTPROTO_SCO	2
#define BTPROTO_RFCOMM	3
#define BTPROTO_BNEP	4
#define BTPROTO_CMTP	5
#define BTPROTO_HIDP	6
#define BTPROTO_AVDTP	7

#define SOL_HCI		0
#define SOL_L2CAP	6
#define SOL_SCO		17
#define SOL_RFCOMM	18

#define BT_SECURITY	4
struct bt_security {
	__u8 level;
	__u8 key_size;
};
#define BT_SECURITY_SDP		0
#define BT_SECURITY_LOW		1
#define BT_SECURITY_MEDIUM	2
#define BT_SECURITY_HIGH	3
#define BT_SECURITY_FIPS	4

#define BT_DEFER_SETUP	7

#define BT_FLUSHABLE	8

#define BT_FLUSHABLE_OFF	0
#define BT_FLUSHABLE_ON		1

#define BT_POWER	9
struct bt_power {
	__u8 force_active;
};
#define BT_POWER_FORCE_ACTIVE_OFF 0
#define BT_POWER_FORCE_ACTIVE_ON  1

#define BT_CHANNEL_POLICY	10

/* BR/EDR only (default policy)
 *   AMP controllers cannot be used.
 *   Channel move requests from the remote device are denied.
 *   If the L2CAP channel is currently using AMP, move the channel to BR/EDR.
 */
#define BT_CHANNEL_POLICY_BREDR_ONLY		0

/* BR/EDR Preferred
 *   Allow use of AMP controllers.
 *   If the L2CAP channel is currently on AMP, move it to BR/EDR.
 *   Channel move requests from the remote device are allowed.
 */
#define BT_CHANNEL_POLICY_BREDR_PREFERRED	1

/* AMP Preferred
 *   Allow use of AMP controllers
 *   If the L2CAP channel is currently on BR/EDR and AMP controller
 *     resources are available, initiate a channel move to AMP.
 *   Channel move requests from the remote device are allowed.
 *   If the L2CAP socket has not been connected yet, try to create
 *     and configure the channel directly on an AMP controller rather
 *     than BR/EDR.
 */
#define BT_CHANNEL_POLICY_AMP_PREFERRED		2

#define BT_VOICE		11
struct bt_voice {
	__u16 setting;
};

#define BT_VOICE_TRANSPARENT			0x0003
#define BT_VOICE_CVSD_16BIT			0x0060

#define BT_SNDMTU		12
#define BT_RCVMTU		13

/* Connection and socket states */
enum {
	BT_CONNECTED = 1, /* Equal to TCP_ESTABLISHED to make net code happy */
	BT_OPEN,
	BT_BOUND,
	BT_LISTEN,
	BT_CONNECT,
	BT_CONNECT2,
	BT_CONFIG,
	BT_DISCONN,
	BT_CLOSED
};

#endif /* BT_IOCTL_HPP_ */
