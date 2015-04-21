/*-
 * Copyright (c) 2012 Michihiro NAKAJIMA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in this position and unchanged.
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
__FBSDID("$FreeBSD$");

static char buff[4096];

DEFINE_TEST(test_write_format_mtree_absolute_path)
{
	struct tk_archive_entry *ae;
	struct archive* a;
	size_t used;

	/* Create a mtree format archive. */
	assert((a = tk_archive_write_new()) != NULL);
	assertEqualIntA(a, ARCHIVE_OK, tk_archive_write_set_format_mtree(a));
	assertEqualIntA(a, ARCHIVE_OK,
	    tk_archive_write_open_memory(a, buff, sizeof(buff)-1, &used));

	/* Write "." file.  */
	assert((ae = tk_archive_entry_new()) != NULL);
	tk_archive_entry_copy_pathname(ae, ".");
	tk_archive_entry_set_mode(ae, AE_IFDIR | 0755);
	assertEqualIntA(a, ARCHIVE_OK, tk_archive_write_header(a, ae));
	tk_archive_entry_free(ae);

	/* Write "/file" file.  */
	assert((ae = tk_archive_entry_new()) != NULL);
	tk_archive_entry_copy_pathname(ae, "/file");
	tk_archive_entry_set_size(ae, 0);
	tk_archive_entry_set_mode(ae, AE_IFREG | 0644);
	assertEqualIntA(a, ARCHIVE_OK, tk_archive_write_header(a, ae));
	tk_archive_entry_free(ae);

	/* Write "/dir" directory.  */
	assert((ae = tk_archive_entry_new()) != NULL);
	tk_archive_entry_copy_pathname(ae, "/dir");
	tk_archive_entry_set_mode(ae, AE_IFDIR | 0755);
	assertEqualIntA(a, ARCHIVE_OK, tk_archive_write_header(a, ae));
	tk_archive_entry_free(ae);

	assertEqualIntA(a, ARCHIVE_OK, tk_archive_write_close(a));
        assertEqualInt(ARCHIVE_OK, tk_archive_write_free(a));

	/*
	 * Read the data and check it.
	 */
	assert((a = tk_archive_read_new()) != NULL);
	assertEqualIntA(a, ARCHIVE_OK, tk_archive_read_support_format_all(a));
	assertEqualIntA(a, ARCHIVE_OK, tk_archive_read_support_filter_all(a));
	assertEqualIntA(a, ARCHIVE_OK, tk_archive_read_open_memory(a, buff, used));

	/* Read "." file. */
	assertEqualIntA(a, ARCHIVE_OK, tk_archive_read_next_header(a, &ae));
	failure("The path should be just \".\"");
	assertEqualString(tk_archive_entry_pathname(ae), ".");
	assertEqualInt(tk_archive_entry_mode(ae), AE_IFDIR | 0755);

	/* Read "/file" file. */
	assertEqualIntA(a, ARCHIVE_OK, tk_archive_read_next_header(a, &ae));
	failure("The path should have \"./\" prefix");
	assertEqualString(tk_archive_entry_pathname(ae), "./file");
	assertEqualInt(tk_archive_entry_size(ae), 0);
	assertEqualInt(tk_archive_entry_mode(ae), AE_IFREG | 0644);

	/* Read "/dir" file. */
	assertEqualIntA(a, ARCHIVE_OK, tk_archive_read_next_header(a, &ae));
	failure("The path should have \"./\" prefix");
	assertEqualString(tk_archive_entry_pathname(ae), "./dir");
	assertEqualInt(tk_archive_entry_mode(ae), AE_IFDIR | 0755);

	assertEqualIntA(a, ARCHIVE_OK, tk_archive_read_close(a));
	assertEqualInt(ARCHIVE_OK, tk_archive_read_free(a));
}

