
/** \file FileBlockLocal_internal.H
 * \brief private implementation details of FileBlockLocal
 * \author Phillip F Knaack
 *
 * This file implements private details for the
 *  FileBlockLocal implementation. */

#include "dll2.H"

/** A unique signature stored in the file, so the constructor
 *  can verify a file contains the required structures. */
#define FILE_BLOCK_SIGNATURE  0x43206810

