#ifndef __WN_CGI_H
#define __WN_CGI_H

#include <webnet.h>

extern const struct webnet_module_upload_entry upload_bin_upload;

extern void cgi_get_version(struct webnet_session *session);
extern void cgi_check_files(struct webnet_session *session);
extern void cgi_handshake(struct webnet_session *session);
extern void cgi_diskfree(struct webnet_session *session);
#endif
