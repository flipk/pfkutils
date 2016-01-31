/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#include <inttypes.h>
#include <string>

#ifndef __PFKBAK_TARFILE_H__
#define __PFKBAK_TARFILE_H__

void tarfile_emit_fileheader(int fd, const std::string &path,
                             uint64_t filesize);

void tarfile_emit_padding(int fd, uint64_t filesize);

void tarfile_emit_footer(int fd);

#endif /*__PFKBAK_TARFILE_H__*/
