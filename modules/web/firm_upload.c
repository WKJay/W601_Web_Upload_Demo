/*************************************************
 Copyright (c) 2022
 All rights reserved.
 File name:     firm_upload.c
 Description:   
 History:
 1. Version:    
    Date:       2022-03-21
    Author:     WKJay
    Modify:     
*************************************************/

#include <stdlib.h>
#include <string.h>

#include <rtthread.h>
#include <dfs_posix.h>

#include <webnet.h>
#include <wn_module.h>

#include "web_utils.h"

#include "fal.h"

uint8_t firm_upload_done = 0;

#ifdef WEBNET_USING_UPLOAD

/**
 * upload file.
 */
static int file_size = 0;

static int upload_open(struct webnet_session *session) {
    const struct fal_partition *part = RT_NULL;
    const char *file_name = RT_NULL;

    file_name = get_file_name(session);
    rt_kprintf("Upload file: %s\n", file_name);

    if (webnet_upload_get_filename(session) != RT_NULL) {
        part = fal_partition_find("download");
        if (part == RT_NULL) {
            goto _exit;
        }
        fal_partition_erase_all(part);
    }

    file_size = 0;

_exit:
    return (int)part;
}

static int upload_close(struct webnet_session *session) {
    rt_kprintf("Upload FileSize: %d\n", file_size);
    return 0;
}

static int upload_write(struct webnet_session *session, const void *data, rt_size_t length) {
    const struct fal_partition *part = RT_NULL;

    part = (const struct fal_partition *)webnet_upload_get_userdata(session);
    if (part == RT_NULL) return 0;

    fal_partition_write(part, file_size, data, length);

    file_size += length;

    return length;
}

static int upload_done(struct webnet_session *session) {
    const char *mimetype;
    static char status[100];

    /* get mimetype */
    mimetype = mime_get_type("json");

    /* set http header */
    session->request->result_code = 200;

    rt_memset(status, 0, sizeof(status));
    rt_snprintf(status, sizeof(status), "{\"code\":0,\"filesize\":%d}", file_size);

    webnet_session_set_header(session, mimetype, 200, "Ok", rt_strlen(status));
    webnet_session_printf(session, status, file_size);

    firm_upload_done = 1;

    return 0;
}

const struct webnet_module_upload_entry upload_bin_upload = {
    "/upload/app", upload_open, upload_close, upload_write, upload_done};

#endif /* WEBNET_USING_UPLOAD */
