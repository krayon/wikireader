//
// Authors:	Wolfgang Spraul <wolfgang@openmoko.com>
//
//		This program is free software; you can redistribute it and/or
//		modify it under the terms of the GNU General Public License
//		as published by the Free Software Foundation; either version
//		3 of the License, or (at your option) any later version.
//           

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "file-io.h"
#include "wom_reader.h"
#include "tff.h"

#define DBG_WOM_READER 0

struct wom_file
{
	FIL fileh;
	uint8_t page_buffer[WOM_PAGE_SIZE];
	wom_header_t hdr;
	uint32_t cur_search_page, next_search_offset;
};

wom_file_t* wom_open(const char* filename)
{
	wom_file_t* womh;
	UINT num_read;

	womh = (wom_file_t*) malloc(sizeof(*womh));
	if (!womh) goto xout;
	if (f_open(&womh->fileh, filename, FA_READ) != FR_OK) goto xout_womh;
	if (f_read(&womh->fileh, &womh->hdr, sizeof(womh->hdr), &num_read) != FR_OK) goto xout_fileh;
	if (num_read != sizeof(womh->hdr)) goto xout_fileh;
	
	DP(DBG_WOM_READER, (MSG_INFO, "O wom\twom_open() '%s' succeeded.\n", filename));
	return womh;

xout_fileh:
	f_close(&womh->fileh);
xout_womh:
	free(womh);
xout:
	DX();
	return 0;
}

void wom_close(wom_file_t* womh)
{
	if (!womh) return;
	f_close(&womh->fileh);
	free(womh);
}

const wom_index_entry_t* wom_find_article(wom_file_t* womh, const char* search_string, size_t search_str_len)
{
	wom_index_entry_t* idx;
	FRESULT fr;
	UINT num_read;

	DP(DBG_WOM_READER, (MSG_INFO, "O wom_find_article('%.*s')\n", search_str_len, search_string));
	for (womh->cur_search_page = womh->hdr.index_first_page;
		womh->cur_search_page < womh->hdr.index_first_page + womh->hdr.index_num_pages; womh->cur_search_page++) {
		if (f_lseek(&womh->fileh, womh->cur_search_page * WOM_PAGE_SIZE) != FR_OK) goto xout;
		fr = f_read(&womh->fileh, womh->page_buffer, WOM_PAGE_SIZE, &num_read);
		if (fr != FR_OK || num_read != WOM_PAGE_SIZE) goto xout;
		womh->next_search_offset = 0;
		while (womh->next_search_offset <= WOM_PAGE_SIZE-6) {
			idx = (wom_index_entry_t*) &womh->page_buffer[womh->next_search_offset];
			if (idx->offset_into_articles == END_OF_INDEX_PAGE)
				break;
			womh->next_search_offset += 6 + idx->uri_len;
			if (!strncasecmp(idx->abbreviated_uri, search_string, min(idx->uri_len, search_str_len)))
				return idx;
		}
	}
	return 0;
xout:
	DX();
	return 0;
}

const wom_index_entry_t* wom_get_next_article(wom_file_t* womh)
{
	wom_index_entry_t* idx;
	UINT num_read;
	FRESULT fr;

	// Upon entry, womh->page_buffer is assumed to contain womh->cur_search_page, and
	// womh->next_search_offset points to where the next index should be.
	while (womh->cur_search_page < womh->hdr.index_first_page + womh->hdr.index_num_pages) {
		while (womh->next_search_offset <= WOM_PAGE_SIZE-6) {
			idx = (wom_index_entry_t*) &womh->page_buffer[womh->next_search_offset];
			if (idx->offset_into_articles == END_OF_INDEX_PAGE)
				break;
			womh->next_search_offset += 6 + idx->uri_len;
			return idx;
		}
		womh->cur_search_page++;
		if (f_lseek(&womh->fileh, womh->cur_search_page * WOM_PAGE_SIZE) != FR_OK) goto xout;
		fr = f_read(&womh->fileh, womh->page_buffer, WOM_PAGE_SIZE, &num_read);
		if (fr != FR_OK || num_read != WOM_PAGE_SIZE) goto xout;
		womh->next_search_offset = 0;
	}
	return 0;
xout:
	DX();
	return 0;
}

void wom_draw(wom_file_t* womh, uint32_t offset_into_articles, uint8_t* frame_buffer_dest, int32_t y_start_in_article, int32_t lines_to_draw)
{
}