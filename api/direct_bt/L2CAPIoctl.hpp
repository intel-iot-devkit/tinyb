/*
 * Copyright (C) 2000-2001 Qualcomm Incorporated
 * Copyright (C) 2009-2010 Gustavo F. Padovan <gustavo@padovan.org>
 * Copyright (C) 2010 Google Inc.
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
 * linux-kernel 4.19 include/net/bluetooth/l2cap.h (git head d8edd9ed156a1a840f1b1c2dbbf458684d6eea6e).
 *
 * Original copyright:
 * BlueZ - Bluetooth protocol stack for Linux
 * Copyright (C) 2000-2001 Qualcomm Incorporated
 * Copyright (C) 2009-2010 Gustavo F. Padovan <gustavo@padovan.org>
 * Copyright (C) 2010 Google Inc.
 * Written 2000,2001 by Maxim Krasnyansky <maxk@qualcomm.com>
 */

#ifndef L2CAP_IOCTL_HPP_
#define L2CAP_IOCTL_HPP_

#include "BTAddress.hpp"

#include "linux_kernel_types.hpp"

/**
 * BT Core Spec v5.2: Vol 3, Part A: BT Logical Link Control and Adaption Protocol (L2CAP)
 */

extern "C" {
    #include <stdint.h>
    #include <sys/socket.h>
} /* extern "C" */

/**
 * Information from include/net/bluetooth/l2cap.h
 * Mixed with own comments.
 */

/* L2CAP defaults */
#define L2CAP_DEFAULT_MTU		672
#define L2CAP_DEFAULT_MIN_MTU		48
#define L2CAP_DEFAULT_FLUSH_TO		0xFFFF
#define L2CAP_EFS_DEFAULT_FLUSH_TO	0xFFFFFFFF
#define L2CAP_DEFAULT_TX_WINDOW		63
#define L2CAP_DEFAULT_EXT_WINDOW	0x3FFF
#define L2CAP_DEFAULT_MAX_TX		3
#define L2CAP_DEFAULT_RETRANS_TO	2000    /* 2 seconds */
#define L2CAP_DEFAULT_MONITOR_TO	12000   /* 12 seconds */
#define L2CAP_DEFAULT_MAX_PDU_SIZE	1492    /* Sized for AMP packet */
#define L2CAP_DEFAULT_ACK_TO		200
#define L2CAP_DEFAULT_MAX_SDU_SIZE	0xFFFF
#define L2CAP_DEFAULT_SDU_ITIME		0xFFFFFFFF
#define L2CAP_DEFAULT_ACC_LAT		0xFFFFFFFF
#define L2CAP_BREDR_MAX_PAYLOAD		1019    /* 3-DH5 packet */
#define L2CAP_LE_MIN_MTU		23

/**
 * L2CAP socket address
 * <p>
 * BT Core Spec v5.2: Vol 3, Part A: L2CAP_CONNECTION_REQ
 * </p>
 */
struct sockaddr_l2 {
	sa_family_t	l2_family;
    /** Protocol Service Multiplexers */
	__le16		l2_psm;
	bdaddr_t	l2_bdaddr;
    /** Channel ID */
	__le16		l2_cid;
	__u8		l2_bdaddr_type;
};


/* L2CAP socket options */
#define L2CAP_OPTIONS   0x01
struct l2cap_options {
	__u16 omtu;
	__u16 imtu;
	__u16 flush_to;
	__u8  mode;
	__u8  fcs;
	__u8  max_tx;
	__u16 txwin_size;
};

#define L2CAP_CONNINFO	0x02
struct l2cap_conninfo {
	__u16 hci_handle;
	__u8  dev_class[3];
};

#define L2CAP_LM	0x03
#define L2CAP_LM_MASTER		0x0001
#define L2CAP_LM_AUTH		0x0002
#define L2CAP_LM_ENCRYPT	0x0004
#define L2CAP_LM_TRUSTED	0x0008
#define L2CAP_LM_RELIABLE	0x0010
#define L2CAP_LM_SECURE		0x0020
#define L2CAP_LM_FIPS		0x0040

/* L2CAP command codes */
#define L2CAP_COMMAND_REJ	0x01
#define L2CAP_CONN_REQ		0x02
#define L2CAP_CONN_RSP		0x03
#define L2CAP_CONF_REQ		0x04
#define L2CAP_CONF_RSP		0x05
#define L2CAP_DISCONN_REQ	0x06
#define L2CAP_DISCONN_RSP	0x07
#define L2CAP_ECHO_REQ		0x08
#define L2CAP_ECHO_RSP		0x09
#define L2CAP_INFO_REQ		0x0a
#define L2CAP_INFO_RSP		0x0b
#define L2CAP_CREATE_CHAN_REQ	0x0c
#define L2CAP_CREATE_CHAN_RSP	0x0d
#define L2CAP_MOVE_CHAN_REQ	0x0e
#define L2CAP_MOVE_CHAN_RSP	0x0f
#define L2CAP_MOVE_CHAN_CFM	0x10
#define L2CAP_MOVE_CHAN_CFM_RSP	0x11
#define L2CAP_CONN_PARAM_UPDATE_REQ	0x12
#define L2CAP_CONN_PARAM_UPDATE_RSP	0x13
#define L2CAP_LE_CONN_REQ	0x14
#define L2CAP_LE_CONN_RSP	0x15
#define L2CAP_LE_CREDITS	0x16

/* L2CAP extended feature mask */
#define L2CAP_FEAT_FLOWCTL	0x00000001
#define L2CAP_FEAT_RETRANS	0x00000002
#define L2CAP_FEAT_BIDIR_QOS	0x00000004
#define L2CAP_FEAT_ERTM		0x00000008
#define L2CAP_FEAT_STREAMING	0x00000010
#define L2CAP_FEAT_FCS		0x00000020
#define L2CAP_FEAT_EXT_FLOW	0x00000040
#define L2CAP_FEAT_FIXED_CHAN	0x00000080
#define L2CAP_FEAT_EXT_WINDOW	0x00000100
#define L2CAP_FEAT_UCD		0x00000200

/* L2CAP checksum option */
#define L2CAP_FCS_NONE		0x00
#define L2CAP_FCS_CRC16		0x01

/* L2CAP fixed channels */
#define L2CAP_FC_SIG_BREDR	0x02
#define L2CAP_FC_CONNLESS	0x04
#define L2CAP_FC_A2MP		0x08
#define L2CAP_FC_ATT		0x10
#define L2CAP_FC_SIG_LE		0x20
#define L2CAP_FC_SMP_LE		0x40
#define L2CAP_FC_SMP_BREDR	0x80

/* L2CAP Control Field bit masks */
#define L2CAP_CTRL_SAR			0xC000
#define L2CAP_CTRL_REQSEQ		0x3F00
#define L2CAP_CTRL_TXSEQ		0x007E
#define L2CAP_CTRL_SUPERVISE		0x000C

#define L2CAP_CTRL_RETRANS		0x0080
#define L2CAP_CTRL_FINAL		0x0080
#define L2CAP_CTRL_POLL			0x0010
#define L2CAP_CTRL_FRAME_TYPE		0x0001 /* I- or S-Frame */

#define L2CAP_CTRL_TXSEQ_SHIFT		1
#define L2CAP_CTRL_SUPER_SHIFT		2
#define L2CAP_CTRL_POLL_SHIFT		4
#define L2CAP_CTRL_FINAL_SHIFT		7
#define L2CAP_CTRL_REQSEQ_SHIFT		8
#define L2CAP_CTRL_SAR_SHIFT		14

/* L2CAP Extended Control Field bit mask */
#define L2CAP_EXT_CTRL_TXSEQ		0xFFFC0000
#define L2CAP_EXT_CTRL_SAR		0x00030000
#define L2CAP_EXT_CTRL_SUPERVISE	0x00030000
#define L2CAP_EXT_CTRL_REQSEQ		0x0000FFFC

#define L2CAP_EXT_CTRL_POLL		0x00040000
#define L2CAP_EXT_CTRL_FINAL		0x00000002
#define L2CAP_EXT_CTRL_FRAME_TYPE	0x00000001 /* I- or S-Frame */

#define L2CAP_EXT_CTRL_FINAL_SHIFT	1
#define L2CAP_EXT_CTRL_REQSEQ_SHIFT	2
#define L2CAP_EXT_CTRL_SAR_SHIFT	16
#define L2CAP_EXT_CTRL_SUPER_SHIFT	16
#define L2CAP_EXT_CTRL_POLL_SHIFT	18
#define L2CAP_EXT_CTRL_TXSEQ_SHIFT	18

/* L2CAP Supervisory Function */
#define L2CAP_SUPER_RR		0x00
#define L2CAP_SUPER_REJ		0x01
#define L2CAP_SUPER_RNR		0x02
#define L2CAP_SUPER_SREJ	0x03

/* L2CAP Segmentation and Reassembly */
#define L2CAP_SAR_UNSEGMENTED	0x00
#define L2CAP_SAR_START		0x01
#define L2CAP_SAR_END		0x02
#define L2CAP_SAR_CONTINUE	0x03

/* L2CAP Command rej. reasons */
#define L2CAP_REJ_NOT_UNDERSTOOD	0x0000
#define L2CAP_REJ_MTU_EXCEEDED		0x0001
#define L2CAP_REJ_INVALID_CID		0x0002

/* L2CAP structures */
struct l2cap_hdr {
	__le16     len;
	__le16     cid;
} __packed;
#define L2CAP_HDR_SIZE		4
#define L2CAP_ENH_HDR_SIZE	6
#define L2CAP_EXT_HDR_SIZE	8

#define L2CAP_FCS_SIZE		2
#define L2CAP_SDULEN_SIZE	2
#define L2CAP_PSMLEN_SIZE	2
#define L2CAP_ENH_CTRL_SIZE	2
#define L2CAP_EXT_CTRL_SIZE	4

struct l2cap_cmd_hdr {
	__u8       code;
	__u8       ident;
	__le16     len;
} __packed;
#define L2CAP_CMD_HDR_SIZE	4

struct l2cap_cmd_rej_unk {
	__le16     reason;
} __packed;

struct l2cap_cmd_rej_mtu {
	__le16     reason;
	__le16     max_mtu;
} __packed;

struct l2cap_cmd_rej_cid {
	__le16     reason;
	__le16     scid;
	__le16     dcid;
} __packed;

struct l2cap_conn_req {
	__le16     psm;
	__le16     scid;
} __packed;

struct l2cap_conn_rsp {
	__le16     dcid;
	__le16     scid;
	__le16     result;
	__le16     status;
} __packed;

/* protocol/service multiplexer (PSM) */
#if 0 /* BTIoctl.hpp */
#define L2CAP_PSM_SDP		0x0001
#define L2CAP_PSM_RFCOMM	0x0003
#define L2CAP_PSM_3DSP		0x0021
#define L2CAP_PSM_IPSP		0x0023 /* 6LoWPAN */

#define L2CAP_PSM_DYN_START	0x1001
#define L2CAP_PSM_DYN_END	0xffff
#define L2CAP_PSM_AUTO_END	0x10ff
#define L2CAP_PSM_LE_DYN_START  0x0080
#define L2CAP_PSM_LE_DYN_END	0x00ff


/* channel identifier */
#define L2CAP_CID_SIGNALING	0x0001
#define L2CAP_CID_CONN_LESS	0x0002
#define L2CAP_CID_A2MP		0x0003
#define L2CAP_CID_ATT		0x0004
#define L2CAP_CID_LE_SIGNALING	0x0005
#define L2CAP_CID_SMP		0x0006
#define L2CAP_CID_SMP_BREDR	0x0007
#define L2CAP_CID_DYN_START	0x0040
#define L2CAP_CID_DYN_END	0xffff
#define L2CAP_CID_LE_DYN_END	0x007f
#endif /* BTIoctl.hpp */

/* connect/create channel results */
#define L2CAP_CR_SUCCESS	0x0000
#define L2CAP_CR_PEND		0x0001
#define L2CAP_CR_BAD_PSM	0x0002
#define L2CAP_CR_SEC_BLOCK	0x0003
#define L2CAP_CR_NO_MEM		0x0004
#define L2CAP_CR_BAD_AMP	0x0005
#define L2CAP_CR_AUTHENTICATION	0x0005
#define L2CAP_CR_AUTHORIZATION	0x0006
#define L2CAP_CR_BAD_KEY_SIZE	0x0007
#define L2CAP_CR_ENCRYPTION	0x0008
#define L2CAP_CR_INVALID_SCID	0x0009
#define L2CAP_CR_SCID_IN_USE	0x000A

/* connect/create channel status */
#define L2CAP_CS_NO_INFO	0x0000
#define L2CAP_CS_AUTHEN_PEND	0x0001
#define L2CAP_CS_AUTHOR_PEND	0x0002

struct l2cap_conf_req {
	__le16     dcid;
	__le16     flags;
	__u8       data[0];
} __packed;

struct l2cap_conf_rsp {
	__le16     scid;
	__le16     flags;
	__le16     result;
	__u8       data[0];
} __packed;

#define L2CAP_CONF_SUCCESS	0x0000
#define L2CAP_CONF_UNACCEPT	0x0001
#define L2CAP_CONF_REJECT	0x0002
#define L2CAP_CONF_UNKNOWN	0x0003
#define L2CAP_CONF_PENDING	0x0004
#define L2CAP_CONF_EFS_REJECT	0x0005

/* configuration req/rsp continuation flag */
#define L2CAP_CONF_FLAG_CONTINUATION	0x0001

struct l2cap_conf_opt {
	__u8       type;
	__u8       len;
	__u8       val[0];
} __packed;
#define L2CAP_CONF_OPT_SIZE	2

#define L2CAP_CONF_HINT		0x80
#define L2CAP_CONF_MASK		0x7f

#define L2CAP_CONF_MTU		0x01
#define L2CAP_CONF_FLUSH_TO	0x02
#define L2CAP_CONF_QOS		0x03
#define L2CAP_CONF_RFC		0x04
#define L2CAP_CONF_FCS		0x05
#define L2CAP_CONF_EFS		0x06
#define L2CAP_CONF_EWS		0x07

#define L2CAP_CONF_MAX_SIZE	22

struct l2cap_conf_rfc {
	__u8       mode;
	__u8       txwin_size;
	__u8       max_transmit;
	__le16     retrans_timeout;
	__le16     monitor_timeout;
	__le16     max_pdu_size;
} __packed;

#define L2CAP_MODE_BASIC	0x00
#define L2CAP_MODE_RETRANS	0x01
#define L2CAP_MODE_FLOWCTL	0x02
#define L2CAP_MODE_ERTM		0x03
#define L2CAP_MODE_STREAMING	0x04

/* Unlike the above this one doesn't actually map to anything that would
 * ever be sent over the air. Therefore, use a value that's unlikely to
 * ever be used in the BR/EDR configuration phase.
 */
#define L2CAP_MODE_LE_FLOWCTL	0x80

struct l2cap_conf_efs {
	__u8	id;
	__u8	stype;
	__le16	msdu;
	__le32	sdu_itime;
	__le32	acc_lat;
	__le32	flush_to;
} __packed;

#define L2CAP_SERV_NOTRAFIC	0x00
#define L2CAP_SERV_BESTEFFORT	0x01
#define L2CAP_SERV_GUARANTEED	0x02

#define L2CAP_BESTEFFORT_ID	0x01

struct l2cap_disconn_req {
	__le16     dcid;
	__le16     scid;
} __packed;

struct l2cap_disconn_rsp {
	__le16     dcid;
	__le16     scid;
} __packed;

struct l2cap_info_req {
	__le16      type;
} __packed;

struct l2cap_info_rsp {
	__le16      type;
	__le16      result;
	__u8        data[0];
} __packed;

struct l2cap_create_chan_req {
	__le16      psm;
	__le16      scid;
	__u8        amp_id;
} __packed;

struct l2cap_create_chan_rsp {
	__le16      dcid;
	__le16      scid;
	__le16      result;
	__le16      status;
} __packed;

struct l2cap_move_chan_req {
	__le16      icid;
	__u8        dest_amp_id;
} __packed;

struct l2cap_move_chan_rsp {
	__le16      icid;
	__le16      result;
} __packed;

#define L2CAP_MR_SUCCESS	0x0000
#define L2CAP_MR_PEND		0x0001
#define L2CAP_MR_BAD_ID		0x0002
#define L2CAP_MR_SAME_ID	0x0003
#define L2CAP_MR_NOT_SUPP	0x0004
#define L2CAP_MR_COLLISION	0x0005
#define L2CAP_MR_NOT_ALLOWED	0x0006

struct l2cap_move_chan_cfm {
	__le16      icid;
	__le16      result;
} __packed;

#define L2CAP_MC_CONFIRMED	0x0000
#define L2CAP_MC_UNCONFIRMED	0x0001

struct l2cap_move_chan_cfm_rsp {
	__le16      icid;
} __packed;

/* info type */
#define L2CAP_IT_CL_MTU		0x0001
#define L2CAP_IT_FEAT_MASK	0x0002
#define L2CAP_IT_FIXED_CHAN	0x0003

/* info result */
#define L2CAP_IR_SUCCESS	0x0000
#define L2CAP_IR_NOTSUPP	0x0001

struct l2cap_conn_param_update_req {
	__le16      min;
	__le16      max;
	__le16      latency;
	__le16      to_multiplier;
} __packed;

struct l2cap_conn_param_update_rsp {
	__le16      result;
} __packed;

/* Connection Parameters result */
#define L2CAP_CONN_PARAM_ACCEPTED	0x0000
#define L2CAP_CONN_PARAM_REJECTED	0x0001

#define L2CAP_LE_MAX_CREDITS		10
#define L2CAP_LE_DEFAULT_MPS		230

struct l2cap_le_conn_req {
	__le16     psm;
	__le16     scid;
	__le16     mtu;
	__le16     mps;
	__le16     credits;
} __packed;

struct l2cap_le_conn_rsp {
	__le16     dcid;
	__le16     mtu;
	__le16     mps;
	__le16     credits;
	__le16     result;
} __packed;

struct l2cap_le_credits {
	__le16     cid;
	__le16     credits;
} __packed;

#endif /* L2CAP_IOCTL_HPP_ */
