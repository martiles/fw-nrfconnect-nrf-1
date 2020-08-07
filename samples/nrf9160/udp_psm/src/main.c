/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <stdio.h>
#include <modem/lte_lc.h>
#include <net/socket.h>
#include <logging/log.h>

#define HOST "129.240.2.6"
#define PORT 123
#define RECV_BUF_SIZE 1024
#define SEND_BUF_SIZE 1024

LOG_MODULE_REGISTER(udp_psm, CONFIG_UDP_PSM_LOG_LEVEL);


static K_SEM_DEFINE(lte_connected, 0, 1);

uint8_t  recv_buf[RECV_BUF_SIZE];


static void lte_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS:
		if ((evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
		     (evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING)) {
			break;
		}

		LOG_INF("Network registration status: %s",
			evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ?
			"Connected - home network" : "Connected - roaming");
		k_sem_give(&lte_connected);
		break;
	case LTE_LC_EVT_PSM_UPDATE:
		LOG_DBG("PSM parameter update: TAU: %d, Active time: %d",
			evt->psm_cfg.tau, evt->psm_cfg.active_time);
		break;
	case LTE_LC_EVT_EDRX_UPDATE: {
		char log_buf[60];
		ssize_t len;

		len = snprintf(log_buf, sizeof(log_buf),
			       "eDRX parameter update: eDRX: %f, PTW: %f",
			       evt->edrx_cfg.edrx, evt->edrx_cfg.ptw);
		if (len > 0) {
			LOG_DBG("%s", log_strdup(log_buf));
		}
		break;
	}
	case LTE_LC_EVT_RRC_UPDATE:
		LOG_DBG("RRC mode: %s",
			evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED ?
			"Connected" : "Idle");
		break;
	case LTE_LC_EVT_CELL_UPDATE:
		LOG_DBG("LTE cell changed: Cell ID: %d, Tracking area: %d",
			evt->cell.id, evt->cell.tac);
		break;
	default:
		break;
	}
}


static void modem_configure(void)
{
#if defined(CONFIG_BSD_LIBRARY)
	if (IS_ENABLED(CONFIG_LTE_AUTO_INIT_AND_CONNECT)) {
		/* Do nothing, modem is already configured and LTE connected. */
	} else {
		int err;
		err = lte_lc_init();
		if (err) {
			LOG_ERR("Failed to set init lte lc, error: %d",
				err);
		}

		//Default PSM mode is set to OFF
		err = lte_lc_psm_req(false);
		if (err) {
			LOG_ERR("Failed to set PSM parameters, error: %d",
				err);
		}
#if defined(CONFIG_POWER_SAVING_MODE_ENABLE)
		/* Requesting PSM before connecting allows the modem to inform
		 * the network about our wish for certain PSM configuration
		 * already in the connection procedure instead of in a separate
		 * request after the connection is in place, which may be
		 * rejected in some networks.
		 */
		err = lte_lc_psm_req(true);
		if (err) {
			LOG_ERR("Failed to set PSM parameters, error: %d",
				err);
		}

		LOG_INF("PSM mode requested");
#endif
#if defined(CONFIG_EDRX_INTERVAL_ENABLE)
		/* Requesting eDRX before connecting allows the modem to inform
		 * the network about our wish for certain eDRX configuration
		 * already in the connection procedure instead of in a separate
		 * request after the connection is in place, which may be
		 * rejected in some networks.
		 */
		err = lte_lc_edrx_param_set(CONFIG_LTE_EDRX_REQ_VALUE);
			if (err) {
			LOG_ERR("Failed to set eDRX parameters, error: %d",
				err);
		}
		err = lte_lc_edrx_req(true);
		if (err) {
			LOG_ERR("Failed to set eDRX parameters, error: %d",
				err);
		}

		LOG_INF("eDRX interval requested");
#endif

		err = lte_lc_connect_async(lte_handler);
		if (err) {
			LOG_ERR("Modem could not be configured, error: %d",
				err);
			return;
		}

		/* Check LTE events of type LTE_LC_EVT_NW_REG_STATUS in
		 * lte_handler() to determine when the LTE link is up.
		 */
	}
#endif
}

/* As it is close to impossible to find
 * a proper UDP server, we just connect
 * to a ntp server and read the response
 */
struct ntp_format {
	uint8_t  flags;
	uint8_t  stratum; /* stratum */
	uint8_t  poll; /* poll interval */
	int8_t precision; /* precision */
	uint32_t  rootdelay; /* root delay */
	uint32_t  rootdisp; /* root dispersion */
	uint32_t  refid; /* reference ID */
	uint32_t  reftime_sec; /* reference time */
	uint32_t  reftime_frac; /* reference time */
	uint32_t  org_sec; /* origin timestamp */
	uint32_t  org_frac; /* origin timestamp */
	uint32_t  rec_sec; /* receive timestamp */
	uint32_t  rec_frac; /* receive timestamp */
	uint32_t  xmt_sec; /* transmit timestamp */
	uint32_t  xmt_frac; /* transmit timestamp */
};


void app_udp_start(void)
{
	struct addrinfo *res;

	struct sockaddr_in remote_addr;
     remote_addr.sin_family = AF_INET;
     remote_addr.sin_port = htons(PORT);
     inet_pton(AF_INET, HOST, &(remote_addr.sin_addr));

	int err = getaddrinfo(HOST, NULL, NULL, &res);
	if (err < 0) {
		LOG_INF("getaddrinfo err: %d\r", err);
		return;
	}

	((struct sockaddr_in *)res->ai_addr)->sin_port = htons(PORT);
	struct sockaddr_in local_addr;

	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(0);
	local_addr.sin_addr.s_addr = 0;

	int client_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (client_fd < 0) {
		LOG_INF("ERROR: client_fd: %d", client_fd);
		goto error;
	}
	LOG_INF("SUCCESS!!: client_fd: %d", client_fd);

	err = bind(client_fd, (struct sockaddr *)&local_addr,
		   sizeof(local_addr));
	if (err < 0) {
		LOG_INF("ERROR: bind err: %d errno: %d", err, errno);
		goto error;
	}
	LOG_INF("SUCCESS!!: bind err: %d errno: %d", err, errno);

	err = connect(client_fd, (struct sockaddr *)&remote_addr,
		      sizeof(struct sockaddr_in));
	if (err < 0) {
		LOG_INF("ERROR: connect err: %d errno: %d", err, errno);
		goto error;
	}
	LOG_INF("SUCCESS!!: connect err: %d errno: %d", err, errno);

	/* Just hard code the packet format */
	uint8_t  send_buf[sizeof(struct ntp_format)] = { 0xe3 };

	err = send(client_fd, send_buf, sizeof(send_buf), 0);
	LOG_INF("sendto ret: %d\r", err);
	if (err < 0) {
		LOG_INF("ERROR: sendto err: %d errno: %d\r", err, errno);
		goto error;
	}
	LOG_INF("SUCCESS!!: sendto err: %d errno: %d\r", err, errno);

error:
	freeaddrinfo(res);
	LOG_INF("app_udp_start Finished -- Closing socket");
	close(client_fd);
}


int main(void)
{
	LOG_INF("UDP test with PSM and/or eDRX enabled");
	modem_configure();
	k_sem_take(&lte_connected, K_FOREVER);
	app_udp_start();
	while (1) {
		k_sleep(K_MSEC(900000)); // 900000 = 15 minutes
			LOG_INF("-------- Run UDP --------------");
				app_udp_start();
	}
	return 1;
}
