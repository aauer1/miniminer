/*
 * Copyright 2013 Andreas Auer
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 3 of the License, or (at your option)
 * any later version.  See COPYING for more details.
 */

/*
 * MiniMiner One with Avalon ASIC
 */

#include "config.h"
#include "miner.h"
#include "fpgautils.h"
#include "logging.h"

#include "deviceapi.h"
#include "sha2.h"

#include "driver-miniminer1.h"

#include <stdio.h>

struct device_drv mmo_drv;

//------------------------------------------------------------------------------
static bool mmo_checkNonce(struct cgpu_info *cgpu,
                            struct work *work,
                            int nonce)
{
	uint32_t *data32 = (uint32_t *)(work->data);
	unsigned char swap[80];
	uint32_t *swap32 = (uint32_t *)swap;
	unsigned char hash1[32];
	unsigned char hash2[32];
	uint32_t *hash2_32 = (uint32_t *)hash2;

	swap32[76/4] = htobe32(nonce);

	swap32yes(swap32, data32, 76 / 4);

	sha256(swap, 80, hash1);
	sha256(hash1, 32, hash2);

	if(hash2_32[7] == 0)
		return true;

	return false;
}

//------------------------------------------------------------------------------
static bool mmo_detect_custom(const char *devpath, struct device_drv *api, struct MMOInfo *info)
{
	int fd = serial_open(devpath, info->baud, 1, true);

	if(fd < 0)
	{
		return false;
	}

	char buf[sizeof(struct MMOIdentity)+1];
	int len;

	write(fd, "I", 1);
	len = serial_read(fd, buf, sizeof(buf));
	serial_close(fd);

	if(len != 14)
	{
		applog(LOG_INFO, "No reply received: %d", len);
		return false;
	}

	info->id.version = buf[1];
	memcpy(info->id.product, buf+2, 8);
	memcpy(&info->id.serial, buf+10, 4);
	applog(LOG_DEBUG, "%d, %s %08x", info->id.version, info->id.product, info->id.serial);

	if(serial_claim(devpath, api))
	{
		const char *claimedby = serial_claim(devpath, api)->dname;
		applog(LOG_DEBUG, "MiniMiner ONE device %s already claimed by other driver: %s", devpath, claimedby);
		return false;
	}

	/* We have a real MiniMiner ONE! */
	struct cgpu_info *mmo;
	mmo = calloc(1, sizeof(struct cgpu_info));
	mmo->drv = api;
	mmo->device_path = strdup(devpath);
	mmo->device_fd = -1;
	mmo->threads = 1;
	add_cgpu(mmo);

	applog(LOG_INFO, "Found %"PRIpreprv" at %s", mmo->proc_repr, devpath);

	applog(LOG_DEBUG, "%"PRIpreprv": Init: baud=%d",
		mmo->proc_repr, info->baud);

	mmo->device_data = info;

	return true;
}

//------------------------------------------------------------------------------
static bool mmo_detect_one(const char *devpath)
{
	struct MMOInfo *info = calloc(1, sizeof(struct MMOInfo));
	if (unlikely(!info))
		quit(1, "Failed to malloc MMOInfo");

	info->baud = MMO_BAUD;

	if (!mmo_detect_custom(devpath, &mmo_drv, info))
	{
		free(info);
		return false;
	}
	return true;
}

//------------------------------------------------------------------------------
static void mmo_detect()
{
	applog(LOG_WARNING, "Searching for MiniMiner ONE devices");
	serial_detect(&mmo_drv, mmo_detect_one);
}

//------------------------------------------------------------------------------
static bool mmo_init(struct thr_info *thr)
{
	applog(LOG_INFO, "MiniMiner ONE init");

	struct cgpu_info *mmo = thr->cgpu;
	struct MMOInfo *info = (struct MMOInfo *)mmo->device_data;

	int fd = serial_open(mmo->device_path, info->baud, 1, true);
	if (unlikely(-1 == fd))
	{
		applog(LOG_ERR, "Failed to open MiniMiner ONE on %s", mmo->device_path);
		return false;
	}

	mmo->device_fd = fd;
	notifier_init(thr->work_restart_notifier);

	applog(LOG_INFO, "Opened MiniMiner ONE on %s", mmo->device_path);

	return true;
}

//------------------------------------------------------------------------------
static int64_t mmo_scanhash(struct thr_info *thr, struct work *work, __maybe_unused int64_t max_nonce)
{
	struct cgpu_info *board = thr->cgpu;
	struct MMOInfo *info = (struct MMOInfo *)board->device_data;

	struct MMOHashData hash_data;

	uint8_t sendbuf[45];
	sendbuf[0] = 'W';
	memcpy(sendbuf + 1, work->midstate, 32);
	memcpy(sendbuf + 33, work->data + 64, 12);
	write(board->device_fd, sendbuf, sizeof(sendbuf));

	applog(LOG_DEBUG, "%"PRIpreprv": sent hashdata", board->proc_repr);

	bool overflow = false;
	int count = 0;
	uint32_t nonce_count = 0;

	uint8_t buffer[128];
	while (!(overflow || thr->work_restart))
	{
		if (!restart_wait(thr, 250))
		{
			applog(LOG_DEBUG, "%"PRIpreprv": New work detected", board->proc_repr);
			break;
		}

		int len = serial_read(board->device_fd, buffer, sizeof(struct MMOState)+1);
		if(len == 7)
		{
			struct MMOState state;
			state.state = buffer[1];
			state.nonce_valid = buffer[2];
			memcpy(&state.nonce, buffer+3, 4);
			applog(LOG_DEBUG, "State: %c, %d, %08x", state.state, state.nonce_valid, state.nonce);

			if(state.nonce_valid == 1)
			{
				nonce_count = state.nonce;
				//if(mmo_checkNonce(board, work, state->nonce))
				{
					applog(LOG_INFO, "Golden nonce found: %08X", state.nonce);
					submit_nonce(thr, work, state.nonce);
				}
				break;
			}
			else if(state.state == 'R')
			{
				nonce_count = 0xFFFFFFFFUL;
				break;
			}
		}

		if (thr->work_restart)
		{
			applog(LOG_DEBUG, "%"PRIpreprv": New work detected", board->proc_repr);
			break;
		}
	}

	work->blk.nonce = 0xffffffff;
	return nonce_count;

}

//------------------------------------------------------------------------------
static bool mmo_statline(char *buf, size_t bufsz, struct cgpu_info *cgpu, bool per_processor)
{
	char before[] = "                ";
	if(cgpu->device_data)
	{
		struct MMOInfo *info = (struct MMOInfo *)cgpu->device_data;

		int len = strlen(info->id.product);
		memcpy(before, info->id.product, len);

		sprintf(before + len + 1, "%08X", info->id.serial);
	}

	tailsprintf(buf, bufsz, "%s", before);
	return true;
}

//------------------------------------------------------------------------------
static void mmo_shutdown(struct thr_info *thr)
{
	struct cgpu_info *cgpu = thr->cgpu;

	serial_close(cgpu->device_fd);
}

//------------------------------------------------------------------------------
struct device_drv mmo_drv = {
	.dname = "MiniMiner One",
	.name = "MMO",
	.drv_detect = mmo_detect,
	.override_statline_temp2 = mmo_statline,
	.scanhash = mmo_scanhash,
	.thread_init = mmo_init,
	.thread_shutdown = mmo_shutdown,
};
