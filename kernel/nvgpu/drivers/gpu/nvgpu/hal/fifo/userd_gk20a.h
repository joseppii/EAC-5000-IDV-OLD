/*
 * Copyright (c) 2011-2019, NVIDIA CORPORATION.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef USERD_GK20A_H
#define USERD_GK20A_H

struct gk20a;
#ifdef CONFIG_NVGPU_USERD
struct nvgpu_channel;

void gk20a_userd_init_mem(struct gk20a *g, struct nvgpu_channel *c);
#ifdef CONFIG_NVGPU_KERNEL_MODE_SUBMIT
u32 gk20a_userd_gp_get(struct gk20a *g, struct nvgpu_channel *c);
u64 gk20a_userd_pb_get(struct gk20a *g, struct nvgpu_channel *c);
void gk20a_userd_gp_put(struct gk20a *g, struct nvgpu_channel *c);
#endif
#endif /* CONFIG_NVGPU_USERD */
u32 gk20a_userd_entry_size(struct gk20a *g);

#endif /* USERD_GK20A_H */
