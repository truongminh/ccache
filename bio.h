/* bio.h
 *
 * Copyright (c) 2013, Nguyen Truong Minh <nguyentrminh at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of CCACHE nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef BIO_H
#define BIO_H

#include "sds.h"
#include "ccache_config.h"

#define BIO_ZOOM_IMAGE 16
#define BIO_REMOVE_FILE 8
#define BIO_WRITE_FILE 4
#define BIO_READ_FILE 2
#define BIO_GENERAL BIO_READ_FILE
#define BIO_FINISHED 1

#define CCACHE_THREAD_STACK_SIZE (1024*1024*4)

/* This structure represents a background Job. It is only used locally to this
 * file as the API deos not expose the internals at all. */
struct bio_job {
    time_t time; /* Time at which the job was created. */
    int type;
    sds name;
    sds result;
};

void bioInit(void);
void bioPushGeneralJob(sds name); /* reserved for master  */
void bioPushRemoveFileJob(sds name);
void bioPushWriteFileJob(sds name);
void bioCreateBackgroundJob(int tid, sds name, int type) ;
unsigned int bioPendingJobsOfThread(int tid);
int bioGetResult(int tid, sds *name, sds *result);

#endif // BIO_H
