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
__FBSDID("$FreeBSD: head/lib/libarchive/test/test_write_format_cpio.c 185672 2008-12-06 06:02:26Z kientzle $");

static void
test_format(int	(*set_format)(struct archive *))
{
	char filedata[64];
	struct tk_archive_entry *ae;
	struct archive *a;
	char *p;
	size_t used;
	size_t buffsize = 1000000;
	char *buff;
	int damaged = 0;

	buff = malloc(buffsize);

	/* Create a new archive in memory. */
	assert((a = tk_archive_write_new()) != NULL);
	assertA(0 == (*set_format)(a));
	assertA(0 == tk_archive_write_add_filter_none(a));
	assertA(0 == tk_archive_write_open_memory(a, buff, buffsize, &used));

	/*
	 * Write a file to it.
	 */
	assert((ae = tk_archive_entry_new()) != NULL);
	tk_archive_entry_set_mtime(ae, 1, 10);
	assert(1 == tk_archive_entry_mtime(ae));
	assert(10 == tk_archive_entry_mtime_nsec(ae));
	p = strdup("file");
	tk_archive_entry_copy_pathname(ae, p);
	strcpy(p, "XXXX");
	free(p);
	assertEqualString("file", tk_archive_entry_pathname(ae));
	tk_archive_entry_set_mode(ae, S_IFREG | 0755);
	assert((S_IFREG | 0755) == tk_archive_entry_mode(ae));
	tk_archive_entry_set_size(ae, 8);

	assertA(0 == tk_archive_write_header(a, ae));
	tk_archive_entry_free(ae);
	assertA(8 == tk_archive_write_data(a, "12345678", 9));

	/*
	 * Write another file to it.
	 */
	assert((ae = tk_archive_entry_new()) != NULL);
	tk_archive_entry_set_mtime(ae, 1, 10);
	assert(1 == tk_archive_entry_mtime(ae));
	assert(10 == tk_archive_entry_mtime_nsec(ae));
	p = strdup("file2");
	tk_archive_entry_copy_pathname(ae, p);
	strcpy(p, "XXXX");
	free(p);
	assertEqualString("file2", tk_archive_entry_pathname(ae));
	tk_archive_entry_set_mode(ae, S_IFREG | 0755);
	assert((S_IFREG | 0755) == tk_archive_entry_mode(ae));
	tk_archive_entry_set_size(ae, 4);

	assertA(0 == tk_archive_write_header(a, ae));
	tk_archive_entry_free(ae);
	assertA(4 == tk_archive_write_data(a, "1234", 5));

	/*
	 * Write a file with a name, filetype, and size.
	 */
	assert((ae = tk_archive_entry_new()) != NULL);
	tk_archive_entry_copy_pathname(ae, "name");
	tk_archive_entry_set_size(ae, 0);
	tk_archive_entry_set_filetype(ae, AE_IFREG);
	assertEqualInt(ARCHIVE_OK, tk_archive_write_header(a, ae));
	assert(tk_archive_error_string(a) == NULL);
	tk_archive_entry_free(ae);

	/*
	 * Write a file with a name and filetype but no size.
	 */
	assert((ae = tk_archive_entry_new()) != NULL);
	tk_archive_entry_copy_pathname(ae, "name");
	tk_archive_entry_unset_size(ae);
	tk_archive_entry_set_filetype(ae, AE_IFREG);
	assertEqualInt(ARCHIVE_FAILED, tk_archive_write_header(a, ae));
	assert(tk_archive_error_string(a) != NULL);
	tk_archive_entry_free(ae);

	/*
	 * Write a file with a name and size but no filetype.
	 */
	assert((ae = tk_archive_entry_new()) != NULL);
	tk_archive_entry_copy_pathname(ae, "name");
	tk_archive_entry_set_size(ae, 0);
	assertEqualInt(ARCHIVE_FAILED, tk_archive_write_header(a, ae));
	assert(tk_archive_error_string(a) != NULL);
	tk_archive_entry_free(ae);

	/*
	 * Write a file with a size and filetype but no name.
	 */
	assert((ae = tk_archive_entry_new()) != NULL);
	tk_archive_entry_set_size(ae, 0);
	tk_archive_entry_set_filetype(ae, AE_IFREG);
	assertEqualInt(ARCHIVE_FAILED, tk_archive_write_header(a, ae));
	assert(tk_archive_error_string(a) != NULL);
	tk_archive_entry_free(ae);

	/*
	 * Write a directory to it.
	 */
	assert((ae = tk_archive_entry_new()) != NULL);
	tk_archive_entry_set_mtime(ae, 11, 110);
	tk_archive_entry_copy_pathname(ae, "dir");
	tk_archive_entry_set_mode(ae, S_IFDIR | 0755);
	tk_archive_entry_set_size(ae, 512);

	assertA(0 == tk_archive_write_header(a, ae));
	assertEqualInt(0, tk_archive_entry_size(ae));
	tk_archive_entry_free(ae);
	assertEqualIntA(a, 0, tk_archive_write_data(a, "12345678", 9));


	/* Close out the archive. */
	assertEqualIntA(a, ARCHIVE_OK, tk_archive_write_close(a));
	assertEqualInt(ARCHIVE_OK, tk_archive_write_free(a));

	/*
	 * Damage the second entry to test the search-ahead recovery.
	 * TODO: Move the damage-recovery checking to a separate test;
	 * it doesn't really belong in this write test.
	 */
	{
		int i;
		for (i = 80; i < 150; i++) {
			if (memcmp(buff + i, "07070", 5) == 0) {
				damaged = 1;
				buff[i] = 'X';
				break;
			}
		}
	}
	failure("Unable to locate the second header for damage-recovery test.");
	assert(damaged == 1);

	/*
	 * Now, read the data back.
	 */
	assert((a = tk_archive_read_new()) != NULL);
	assertA(0 == tk_archive_read_support_format_all(a));
	assertA(0 == tk_archive_read_support_filter_all(a));
	assertA(0 == tk_archive_read_open_memory(a, buff, used));

	if (!assertEqualIntA(a, 0, tk_archive_read_next_header(a, &ae))) {
		tk_archive_read_free(a);
		return;
	}

	assertEqualInt(1, tk_archive_entry_mtime(ae));
	/* Not the same as above: cpio doesn't store hi-res times. */
	assert(0 == tk_archive_entry_mtime_nsec(ae));
	assert(0 == tk_archive_entry_atime(ae));
	assert(0 == tk_archive_entry_ctime(ae));
	assertEqualString("file", tk_archive_entry_pathname(ae));
	assertEqualInt((S_IFREG | 0755), tk_archive_entry_mode(ae));
	assertEqualInt(8, tk_archive_entry_size(ae));
	assertA(8 == tk_archive_read_data(a, filedata, 10));
	assertEqualMem(filedata, "12345678", 8);

	/*
	 * The second file can't be read because we damaged its header.
	 */

	/*
	 * Read the third file back.
	 * ARCHIVE_WARN here because the damaged entry was skipped.
	 */
	assertEqualIntA(a, ARCHIVE_WARN, tk_archive_read_next_header(a, &ae));
	assertEqualString("name", tk_archive_entry_pathname(ae));

	/*
	 * Read the dir entry back.
	 */
	assertEqualIntA(a, ARCHIVE_OK, tk_archive_read_next_header(a, &ae));
	assertEqualInt(11, tk_archive_entry_mtime(ae));
	assert(0 == tk_archive_entry_mtime_nsec(ae));
	assert(0 == tk_archive_entry_atime(ae));
	assert(0 == tk_archive_entry_ctime(ae));
	assertEqualString("dir", tk_archive_entry_pathname(ae));
	assertEqualInt((S_IFDIR | 0755), tk_archive_entry_mode(ae));
	assertEqualInt(0, tk_archive_entry_size(ae));
	assertEqualIntA(a, 0, tk_archive_read_data(a, filedata, 10));

	/* Verify the end of the archive. */
	assertEqualIntA(a, 1, tk_archive_read_next_header(a, &ae));
	assertEqualIntA(a, ARCHIVE_OK, tk_archive_read_close(a));
	assertEqualInt(ARCHIVE_OK, tk_archive_read_free(a));

	free(buff);
}

static void
test_big_entries(int (*set_format)(struct archive *), int64_t size, int expected)
{
	struct tk_archive_entry *ae;
	struct archive *a;
	size_t buffsize = 1000000;
	size_t used;
	char *buff;

	buff = malloc(buffsize);

	/* Create a new archive in memory. */
	assert((a = tk_archive_write_new()) != NULL);
	assertA(0 == (*set_format)(a));
	assertA(0 == tk_archive_write_add_filter_none(a));
	assertA(0 == tk_archive_write_open_memory(a, buff, buffsize, &used));

	assert((ae = tk_archive_entry_new()) != NULL);
	tk_archive_entry_copy_pathname(ae, "file");
	tk_archive_entry_set_size(ae, size);
	tk_archive_entry_set_filetype(ae, AE_IFREG);
	assertEqualInt(expected, tk_archive_write_header(a, ae));
	if (expected != ARCHIVE_OK)
		assert(tk_archive_error_string(a) != NULL);

	tk_archive_entry_free(ae);
	tk_archive_write_free(a);
	free(buff);
}


DEFINE_TEST(test_write_format_cpio)
{
	int64_t size_4g = ((int64_t)1) << 32;
	int64_t size_8g = ((int64_t)1) << 33;

	test_format(tk_archive_write_set_format_cpio);
	test_format(tk_archive_write_set_format_cpio_newc);

	test_big_entries(tk_archive_write_set_format_cpio,
	    size_8g - 1, ARCHIVE_OK);
	test_big_entries(tk_archive_write_set_format_cpio,
	    size_8g, ARCHIVE_FAILED);
	test_big_entries(tk_archive_write_set_format_cpio_newc,
	    size_4g - 1, ARCHIVE_OK);
	test_big_entries(tk_archive_write_set_format_cpio_newc,
	    size_4g, ARCHIVE_FAILED);
}
