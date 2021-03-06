/* Copyright (c) 2008 The Board of Trustees of The Leland Stanford
 * Junior University
 * 
 * We are making the OpenFlow specification and associated documentation
 * (Software) available for public use and benefit with the expectation
 * that others will use, modify and enhance the Software and contribute
 * those enhancements back to the community. However, since we would
 * like to make the Software available for broadest use, with as few
 * restrictions as possible permission is hereby granted, free of
 * charge, to any person obtaining a copy of this Software to deal in
 * the Software under the copyrights without restriction, including
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
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * The name and trademarks of copyright holder(s) may NOT be used in
 * advertising or publicity pertaining to the Software or any
 * derivatives without specific, written prior permission.
 */

#ifndef OPENFLOW_BRCOMPAT_NETLINK_H
#define OPENFLOW_BRCOMPAT_NETLINK_H 1

#define BRC_GENL_FAMILY_NAME "brcompat"

/* Attributes that can be attached to the datapath's netlink messages. */
enum {
	BRC_GENL_A_UNSPEC,
	BRC_GENL_A_DP_NAME,	    /* Datapath name. */
	BRC_GENL_A_PORT_NAME,   /* Interface name. */
	BRC_GENL_A_ERR_CODE,    /* Positive error code. */
	BRC_GENL_A_MC_GROUP,    /* Generic netlink multicast group. */

	__BRC_GENL_A_MAX,
	BRC_GENL_A_MAX = __BRC_GENL_A_MAX - 1
};

/* Commands that can be executed on the datapath's netlink interface. */
enum brc_genl_command {
	BRC_GENL_C_UNSPEC,
	BRC_GENL_C_DP_ADD,      /* Datapath created. */
	BRC_GENL_C_DP_DEL,      /* Datapath destroyed. */
	BRC_GENL_C_DP_RESULT,   /* Result of datapath command from vswitchd. */
	BRC_GENL_C_PORT_ADD,    /* Port added to datapath. */
	BRC_GENL_C_PORT_DEL,    /* Port removed from datapath. */
	BRC_GENL_C_QUERY_MC,    /* Get multicast group for brcompat. */

	__BRC_GENL_C_MAX,
	BRC_GENL_C_MAX = __BRC_GENL_C_MAX - 1
};
#endif /* openflow/brcompat-netlink.h */
