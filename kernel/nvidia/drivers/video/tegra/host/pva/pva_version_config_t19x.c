/*
 * Copyright (c) 2016-2021, NVIDIA Corporation.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "pva_mailbox_t19x.h"
#include "pva_interface_regs_t19x.h"
#include "pva_version_config_t19x.h"
#include "pva_ccq_t19x.h"

static int submit_sync_t19x(struct pva *pva, struct pva_cmd_s *cmd, u32 nregs,
			    u32 queue_id,
			    struct pva_cmd_status_regs *cmd_status_out)
{
	(void)queue_id;
	return pva_mailbox_send_cmd_sync(pva, cmd, nregs, cmd_status_out);
}

static int submit_sync_locked_t19x(struct pva *pva, struct pva_cmd_s *cmd,
				   u32 nregs, u32 queue_id,
				   struct pva_cmd_status_regs *cmd_status_out)
{
	(void)queue_id;
	return pva_mailbox_send_cmd_sync_locked(pva, cmd, nregs,
						cmd_status_out);
}

struct pva_version_config pva_t19x_config = {
	.read_mailbox = pva_read_mailbox_t19x,
	.write_mailbox = pva_write_mailbox_t19x,
	.read_status_interface = read_status_interface_t19x,
	.ccq_send_task = pva_ccq_send_task_t19x,
	.submit_cmd_sync_locked = submit_sync_locked_t19x,
	.submit_cmd_sync = submit_sync_t19x,
	.irq_count = 1,
};
