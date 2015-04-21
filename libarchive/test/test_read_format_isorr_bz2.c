/*-
 * Copyright (c) 2003-2007 Tim Kientzle
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "test.h"
__FBSDID("$FreeBSD: head/lib/libarchive/test/test_read_format_isorr_bz2.c 201247 2009-12-30 05:59:21Z kientzle $");

/*
PLEASE use old cdrtools; mkisofs verion is 2.01.
This version mkisofs made wrong "SL" System Use Entry of RRIP.

Execute the following command to rebuild the data for this program:
   tail -n +34 test_read_format_isorr_bz2.c | /bin/sh

rm -rf /tmp/iso
mkdir /tmp/iso
mkdir /tmp/iso/dir
echo "hello" >/tmp/iso/file
dd if=/dev/zero count=1 bs=12345678 >>/tmp/iso/file
ln /tmp/iso/file /tmp/iso/hardlink
(cd /tmp/iso; ln -s file symlink)
(cd /tmp/iso; ln -s /tmp/ symlink2)
(cd /tmp/iso; ln -s /tmp/../ symlink3)
(cd /tmp/iso; ln -s .././../tmp/ symlink4)
(cd /tmp/iso; ln -s .///file symlink5)
(cd /tmp/iso; ln -s /tmp//../ symlink6)
TZ=utc touch -afhm -t 197001020000.01 /tmp/iso /tmp/iso/file /tmp/iso/dir
TZ=utc touch -afhm -t 197001030000.02 /tmp/iso/symlink /tmp/iso/symlink5
F=test_read_format_iso_rockridge.iso.Z
mkhybrid -R -uid 1 -gid 2 /tmp/iso | compress > $F
uuencode $F $F > $F.uu
exit 1
 */

DEFINE_TEST(test_read_format_isorr_bz2)
{
	const char *refname = "test_read_format_iso_rockridge.iso.Z";
	struct tk_archive_entry *ae;
	struct archive *a;
	const void *p;
	size_t size;
	int64_t offset;
	int i;

	extract_reference_file(refname);
	assert((a = tk_archive_read_new()) != NULL);
	assertEqualInt(0, tk_archive_read_support_filter_all(a));
	assertEqualInt(0, tk_archive_read_support_format_all(a));
	assertEqualInt(ARCHIVE_OK,
	    tk_archive_read_open_filename(a, refname, 10240));

	/* Retrieve each of the 8 files on the ISO image and
	 * verify that each one is what we expect. */
	for (i = 0; i < 10; ++i) {
		assertEqualInt(0, tk_archive_read_next_header(a, &ae));

		if (strcmp(".", tk_archive_entry_pathname(ae)) == 0) {
			/* '.' root directory. */
			assertEqualInt(AE_IFDIR, tk_archive_entry_filetype(ae));
			assertEqualInt(2048, tk_archive_entry_size(ae));
			/* Now, we read timestamp recorded by RRIP "TF". */ 
			assertEqualInt(86401, tk_archive_entry_mtime(ae));
			assertEqualInt(0, tk_archive_entry_mtime_nsec(ae));
			/* Now, we read links recorded by RRIP "PX". */ 
			assertEqualInt(3, tk_archive_entry_stat(ae)->st_nlink);
			assertEqualInt(1, tk_archive_entry_uid(ae));
			assertEqualIntA(a, ARCHIVE_EOF,
			    tk_archive_read_data_block(a, &p, &size, &offset));
			assertEqualInt((int)size, 0);
		} else if (strcmp("dir", tk_archive_entry_pathname(ae)) == 0) {
			/* A directory. */
			assertEqualString("dir", tk_archive_entry_pathname(ae));
			assertEqualInt(AE_IFDIR, tk_archive_entry_filetype(ae));
			assertEqualInt(2048, tk_archive_entry_size(ae));
			assertEqualInt(86401, tk_archive_entry_mtime(ae));
			assertEqualInt(86401, tk_archive_entry_atime(ae));
			assertEqualInt(2, tk_archive_entry_stat(ae)->st_nlink);
			assertEqualInt(1, tk_archive_entry_uid(ae));
			assertEqualInt(2, tk_archive_entry_gid(ae));
		} else if (strcmp("file", tk_archive_entry_pathname(ae)) == 0) {
			/* A regular file. */
			assertEqualString("file", tk_archive_entry_pathname(ae));
			assertEqualInt(AE_IFREG, tk_archive_entry_filetype(ae));
			assertEqualInt(12345684, tk_archive_entry_size(ae));
			assertEqualInt(0,
			    tk_archive_read_data_block(a, &p, &size, &offset));
			assertEqualInt(0, offset);
			assertEqualMem(p, "hello\n", 6);
			assertEqualInt(86401, tk_archive_entry_mtime(ae));
			assertEqualInt(86401, tk_archive_entry_atime(ae));
			assertEqualInt(2, tk_archive_entry_stat(ae)->st_nlink);
			assertEqualInt(1, tk_archive_entry_uid(ae));
			assertEqualInt(2, tk_archive_entry_gid(ae));
		} else if (strcmp("hardlink", tk_archive_entry_pathname(ae)) == 0) {
			/* A hardlink to the regular file. */
			/* Note: If "hardlink" gets returned before "file",
			 * then "hardlink" will get returned as a regular file
			 * and "file" will get returned as the hardlink.
			 * This test should tolerate that, since it's a
			 * perfectly permissible thing for libarchive to do. */
			assertEqualString("hardlink", tk_archive_entry_pathname(ae));
			assertEqualInt(AE_IFREG, tk_archive_entry_filetype(ae));
			assertEqualString("file", tk_archive_entry_hardlink(ae));
			assertEqualInt(0, tk_archive_entry_size_is_set(ae));
			assertEqualInt(0, tk_archive_entry_size(ae));
			assertEqualInt(86401, tk_archive_entry_mtime(ae));
			assertEqualInt(86401, tk_archive_entry_atime(ae));
			assertEqualInt(2, tk_archive_entry_stat(ae)->st_nlink);
			assertEqualInt(1, tk_archive_entry_uid(ae));
			assertEqualInt(2, tk_archive_entry_gid(ae));
		} else if (strcmp("symlink", tk_archive_entry_pathname(ae)) == 0) {
			/* A symlink to the regular file. */
			assertEqualInt(AE_IFLNK, tk_archive_entry_filetype(ae));
			assertEqualString("file", tk_archive_entry_symlink(ae));
			assertEqualInt(0, tk_archive_entry_size(ae));
			assertEqualInt(172802, tk_archive_entry_mtime(ae));
			assertEqualInt(172802, tk_archive_entry_atime(ae));
			assertEqualInt(1, tk_archive_entry_stat(ae)->st_nlink);
			assertEqualInt(1, tk_archive_entry_uid(ae));
			assertEqualInt(2, tk_archive_entry_gid(ae));
		} else if (strcmp("symlink2", tk_archive_entry_pathname(ae)) == 0) {
			/* A symlink to /tmp (an absolute path) */
			assertEqualInt(AE_IFLNK, tk_archive_entry_filetype(ae));
			assertEqualString("/tmp", tk_archive_entry_symlink(ae));
			assertEqualInt(0, tk_archive_entry_size(ae));
			assertEqualInt(1, tk_archive_entry_stat(ae)->st_nlink);
			assertEqualInt(1, tk_archive_entry_uid(ae));
			assertEqualInt(2, tk_archive_entry_gid(ae));
		} else if (strcmp("symlink3", tk_archive_entry_pathname(ae)) == 0) {
			/* A symlink to /tmp/.. (with a ".." component) */
			assertEqualInt(AE_IFLNK, tk_archive_entry_filetype(ae));
			assertEqualString("/tmp/..", tk_archive_entry_symlink(ae));
			assertEqualInt(0, tk_archive_entry_size(ae));
			assertEqualInt(1, tk_archive_entry_stat(ae)->st_nlink);
			assertEqualInt(1, tk_archive_entry_uid(ae));
			assertEqualInt(2, tk_archive_entry_gid(ae));
		} else if (strcmp("symlink4", tk_archive_entry_pathname(ae)) == 0) {
			/* A symlink to a path with ".." and "." components */
			assertEqualInt(AE_IFLNK, tk_archive_entry_filetype(ae));
			assertEqualString(".././../tmp",
			    tk_archive_entry_symlink(ae));
			assertEqualInt(0, tk_archive_entry_size(ae));
			assertEqualInt(1, tk_archive_entry_stat(ae)->st_nlink);
			assertEqualInt(1, tk_archive_entry_uid(ae));
			assertEqualInt(2, tk_archive_entry_gid(ae));
		} else if (strcmp("symlink5", tk_archive_entry_pathname(ae)) == 0) {
			/* A symlink to the regular file with "/" components. */
			assertEqualInt(AE_IFLNK, tk_archive_entry_filetype(ae));
			assertEqualString(".///file", tk_archive_entry_symlink(ae));
			assertEqualInt(0, tk_archive_entry_size(ae));
			assertEqualInt(172802, tk_archive_entry_mtime(ae));
			assertEqualInt(172802, tk_archive_entry_atime(ae));
			assertEqualInt(1, tk_archive_entry_stat(ae)->st_nlink);
			assertEqualInt(1, tk_archive_entry_uid(ae));
			assertEqualInt(2, tk_archive_entry_gid(ae));
		} else if (strcmp("symlink6", tk_archive_entry_pathname(ae)) == 0) {
			/* A symlink to /tmp//..
			 * (with "/" and ".." components) */
			assertEqualInt(AE_IFLNK, tk_archive_entry_filetype(ae));
			assertEqualString("/tmp//..", tk_archive_entry_symlink(ae));
			assertEqualInt(0, tk_archive_entry_size(ae));
			assertEqualInt(1, tk_archive_entry_stat(ae)->st_nlink);
			assertEqualInt(1, tk_archive_entry_uid(ae));
			assertEqualInt(2, tk_archive_entry_gid(ae));
		} else {
			failure("Saw a file that shouldn't have been there");
			assertEqualString(tk_archive_entry_pathname(ae), "");
		}
	}

	/* End of archive. */
	assertEqualInt(ARCHIVE_EOF, tk_archive_read_next_header(a, &ae));

	/* Verify archive format. */
	assertEqualInt(tk_archive_filter_code(a, 0), ARCHIVE_FILTER_COMPRESS);
	assertEqualInt(tk_archive_format(a), ARCHIVE_FORMAT_ISO9660_ROCKRIDGE);

	/* Close the archive. */
	assertEqualIntA(a, ARCHIVE_OK, tk_archive_read_close(a));
	assertEqualInt(ARCHIVE_OK, tk_archive_read_free(a));
}


