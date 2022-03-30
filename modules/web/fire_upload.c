/*************************************************
 Copyright (c) 2022
 All rights reserved.
 File name:     fire_upload.c
 Description:
 History:
 1. Version:
    Date:       2022-03-30
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

#ifdef WEBNET_USING_UPLOAD
static int file_size = 0;

static int upload_file_open(struct webnet_session *session) {
    int fd = RT_NULL;
    const char *file_name = RT_NULL;
    const char *base_path;

    base_path = webnet_request_get_query(session->request, "path");
    if (base_path == NULL) base_path = "/webnet/data";
    file_name = get_file_name(session);
    rt_kprintf("Upload FileName: %s\n", file_name);
    rt_kprintf("Content-Type   : %s\n", webnet_upload_get_content_type(session));

    if (webnet_upload_get_filename(session) != RT_NULL) {
        int path_size;
        char *file_path;

        path_size = strlen(base_path) + strlen(file_name);

        path_size += 4;
        file_path = (char *)rt_malloc(path_size + 1);
        rt_memset(file_path, 0, path_size + 1);

        if (file_path == RT_NULL) {
            fd = RT_NULL;
            goto _exit;
        }

        sprintf(file_path, "%s/%s", base_path, file_name);

        fd = create_file_by_path(file_path);
        if (fd < 0) {
            rt_free(file_path);
            rt_kprintf("open %s failed.\r\n", file_path);
            goto _exit;
        }
        rt_kprintf("save to: %s\r\n", file_path);
        rt_free(file_path);
    }

    file_size = 0;

_exit:
    return (int)fd;
}

static int upload_close(struct webnet_session *session) {
    int fd;

    fd = (int)webnet_upload_get_userdata(session);
    if (fd < 0) return 0;

    close(fd);
    rt_kprintf("Upload FileSize: %d\n", file_size);
    return 0;
}

static int upload_write(struct webnet_session *session, const void *data, rt_size_t length) {
    int fd;

    fd = (int)webnet_upload_get_userdata(session);
    if (fd < 0) return 0;

    write(fd, data, length);
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
    webnet_session_printf(session, status);

    return 0;
}

const struct webnet_module_upload_entry upload_file_upload = {
    "/upload/file", /*upload interface, has no connection with file direction*/
    upload_file_open, upload_close, upload_write, upload_done};

#endif /* WEBNET_USING_UPLOAD */
