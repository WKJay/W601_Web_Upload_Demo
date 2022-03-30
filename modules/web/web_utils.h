#ifndef __WEB_UTILS_H
#define __WEB_UTILS_H

#include <stdint.h>
#include <wn_module.h>

#define MAX_FILENAME_LENGTH 128

int relative_path_2_absolute(char *path_from, char *path_to, int to_length,
                             char *absolute_prefix);
int create_file_by_path(char *path);
int get_file_crc32(char *path, uint32_t *val);
int web_file_init(void);
int disk_sd0_clean(void);
const char *get_file_name(struct webnet_session *session);

#endif /* __WEB_UTILS_H */
