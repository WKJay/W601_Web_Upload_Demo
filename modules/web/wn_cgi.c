/*************************************************
 Copyright (c) 2022
 All rights reserved.
 File name:     wn_cgi.c
 Description:
 History:
 1. Version:
    Date:       2022-03-21
    Author:     WKJay
    Modify:
*************************************************/

#include "webnet.h"
#include <dfs_posix.h>
#include <wn_module.h>
#include <wn_utils.h>
#include <stdio.h>
#include <wn_cgi.h>
#include "web_utils.h"
#include <cJSON.h>

#define DBG_TAG "wncgi"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#define VERSION "V1.0.2"

/**
 * Name:    json_create_web_response
 * Brief:   创建一个web响应
 * Input:
 *  @code:  响应码
 *  @msg:   响应信息
 * Output:  响应体字符串
 */
static char *json_create_web_response(int code, const char *msg) {
    char *json_data = NULL;
    cJSON *root = cJSON_CreateObject();

    if (msg == RT_NULL) {
        msg = " ";
    }

    cJSON_AddItemToObject(root, "code", cJSON_CreateNumber(code));
    cJSON_AddItemToObject(root, "msg", cJSON_CreateString(msg));

    json_data = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json_data;
}

#define cgi_head(type, code, length)                                      \
    do {                                                                  \
        const char *mimetype;                                             \
        mimetype = mime_get_type(type);                                   \
        session->request->result_code = code;                             \
        webnet_session_set_header(session, mimetype, code, "Ok", length); \
    } while (0)

void cgi_handshake(struct webnet_session *session) {
    char *json_data = NULL;
    cJSON *root = cJSON_CreateObject();
    cJSON *handshake = cJSON_CreateObject();

    cJSON_AddItemToObject(root, "code", cJSON_CreateNumber(0));
    cJSON_AddItemToObject(root, "handshake", handshake);

    cJSON_AddItemToObject(handshake, "version", cJSON_CreateString(VERSION));
    cJSON_AddItemToObject(handshake, "support_firmwareupload", cJSON_CreateBool(1));
    cJSON_AddItemToObject(handshake, "support_fileupload", cJSON_CreateBool(1));
    cJSON_AddItemToObject(handshake, "support_directoryupload", cJSON_CreateBool(1));
    cJSON_AddItemToObject(handshake, "support_diskclean", cJSON_CreateBool(1));
    cJSON_AddItemToObject(handshake, "support_diskfree", cJSON_CreateBool(1));
    cJSON_AddItemToObject(handshake, "support_filecheck", cJSON_CreateBool(1));

    json_data = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    cgi_head("json", 200, strlen(json_data));
    webnet_session_printf(session, json_data);
    if (json_data) rt_free(json_data);
}

void cgi_diskfree(struct webnet_session *session) {
    int rs = -1;
    struct statfs buffer;
    long long total_size, free_size;
    char *json_data = NULL;
    cJSON *root = NULL, *disks = NULL, *disk = NULL;

    rs = dfs_statfs("/", &buffer);
    if (rs != 0) {
        LOG_E("dfs_statfs failed.");
        json_data = json_create_web_response(-1, "get disk free failed.");
        goto exit;
    }

    root = cJSON_CreateObject();
    disks = cJSON_CreateArray();
    disk = cJSON_CreateObject();

    cJSON_AddItemToObject(root, "code", cJSON_CreateNumber(0));
    cJSON_AddItemToObject(root, "disks", disks);
    cJSON_AddItemToArray(disks, disk);

    total_size = ((long long)buffer.f_bsize) * ((long long)buffer.f_blocks);
    free_size = ((long long)buffer.f_bsize) * ((long long)buffer.f_bfree);

    cJSON_AddItemToObject(disk, "name", cJSON_CreateString("SD"));
    cJSON_AddItemToObject(disk, "total", cJSON_CreateNumber(total_size));
    cJSON_AddItemToObject(disk, "free", cJSON_CreateNumber(free_size));

    json_data = cJSON_PrintUnformatted(root);

exit:
    cgi_head("json", 200, strlen(json_data));
    webnet_session_printf(session, json_data);
    if (json_data) rt_free(json_data);
    if (root) cJSON_Delete(root);
}

/**
 * Name:    json_get_obj_string
 * Brief:   从json字符串中获取指定对象的字符串值
 *          注:使用该函数必须确保key不带英文格式下的点
 * Input:
 *  @json_str: json 字符串
 *  @obj_str:  指定对象字符串
 * Output:  获取到的指定对象的字符串，使用后必须释放内存！
 *          获取失败返回NULL
 */
char *json_get_obj_string(char *json_str, char *obj_str) {
    char *delim = ".";
    char *key;
    char *value = NULL;
    char *json_str_copy = NULL;
    cJSON *root = NULL;
    cJSON *target = NULL;

    json_str_copy = strdup(obj_str);
    if (json_str_copy == NULL) {
        LOG_E("copy json string  %s failed", json_str);
        goto copy_fail;
    }
    root = cJSON_Parse(json_str);
    if (root == NULL) {
        LOG_E("cjson parse %s failed", json_str);
        goto parse_root_fail;
    }
    key = strtok(json_str_copy, delim);
    while (key != NULL) {
        target = cJSON_GetObjectItem(root, key);
        key = strtok(NULL, delim);
    }
    if (target->valuestring) value = strdup(target->valuestring);
    cJSON_Delete(root);
parse_root_fail:
    free(json_str_copy);
copy_fail:
    return value;
}
void cgi_diskclean(struct webnet_session *session) {
    char *json_data;
    char *disk_name = NULL;
    if (session->request->query_offset) {
        disk_name = json_get_obj_string(session->request->query, "disk_name");
        if (strcmp(disk_name, "SD") != 0) {
            json_data = json_create_web_response(-1, "invalid disk name");
        } else {
            if (disk_sd0_clean() == 0) {
                json_data = json_create_web_response(0, "ok");
            } else {
                json_data = json_create_web_response(-1, "clean disk failed.");
            }
        }

        if (disk_name) {
            free(disk_name);
            disk_name = NULL;
        }
    } else {
        json_data = json_create_web_response(-1, "no query string");
    }

    cgi_head("json", 200, strlen(json_data));
    webnet_session_printf(session, json_data);
    if (json_data) rt_free(json_data);
}

void cgi_check_files(struct webnet_session *session) {
    char *json_data;
    cJSON *res_root = NULL;
    cJSON *root = NULL;

    res_root = cJSON_CreateObject();
    if (res_root == NULL) {
        json_data = json_create_web_response(-1, "internal error");
        goto exit;
    }

    root = cJSON_Parse((const char *)session->request->query);
    if (root) {
        cJSON *result = cJSON_CreateObject();
        cJSON *files = cJSON_GetObjectItem(root, "files");
        cJSON *iter = NULL;
        if (result == NULL) {
            json_data = json_create_web_response(-1, "internal error");
            goto exit;
        }
        cJSON_AddItemToObject(res_root, "code", cJSON_CreateNumber(0));
        cJSON_ArrayForEach(iter, files) {
            int ret = 0;
            uint32_t crc32_value = 0;
            char crc32_value_str[24];
            cJSON *path = cJSON_GetObjectItem(iter, "path");
            cJSON *id = cJSON_GetObjectItem(iter, "id");
            if (path == NULL || id == NULL) {
                json_data = json_create_web_response(-1, "invalid payload");
                goto exit;
            }
            ret = get_file_crc32(path->valuestring, &crc32_value);
            if (ret == 0) {
                rt_memset(crc32_value_str, 0, sizeof(crc32_value_str));
                rt_snprintf(crc32_value_str, sizeof(crc32_value_str), "%u", crc32_value);
                cJSON_AddItemToObject(result, id->valuestring, cJSON_CreateString(crc32_value_str));
            } else {
                cJSON_AddItemToObject(result, id->valuestring, cJSON_CreateString("error"));
            }
        }
        cJSON_AddItemToObject(res_root, "result", result);
        json_data = cJSON_PrintUnformatted(res_root);
    } else {
        json_data = json_create_web_response(-1, "unknown request payload");
        goto exit;
    }

exit:
    cgi_head("json", 200, strlen(json_data));
    webnet_session_printf(session, json_data);
    if (json_data) rt_free(json_data);
    if (res_root) cJSON_Delete(res_root);
    if (root) cJSON_Delete(root);
}
