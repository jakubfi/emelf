//  Copyright (c) 2014 Jakub Filipowicz <jakubf@gmail.com>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc.,
//  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>

#include "emelf.h"
#include "edh.h"

int emelf_errno;

// -----------------------------------------------------------------------
static void ahtons(uint16_t *t, int len)
{
	int pos = len-1;
	while (pos >= 0) {
		t[pos] = htons(t[pos]);
		pos--;
	}
}

// -----------------------------------------------------------------------
static void antohs(uint16_t *t, int len)
{
	int pos = len-1;
	while (pos >= 0) {
		t[pos] = ntohs(t[pos]);
		pos--;
	}
}

// -----------------------------------------------------------------------
static size_t nfread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	int res = fread(ptr, size, nmemb, stream);
	if (res < 0) {
		return res;
	}

	antohs(ptr, size * nmemb / 2);

	return res;
}

// -----------------------------------------------------------------------
static size_t nfwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	uint16_t *tmp = malloc(size * nmemb);
	if (!tmp) {
		return -1;
	}
	memcpy(tmp, ptr, size * nmemb);
	ahtons(tmp, size * nmemb / 2);

	int res = fwrite(tmp, size, nmemb, stream);

	free(tmp);
	return res;
}

// -----------------------------------------------------------------------
struct emelf * emelf_create(unsigned type, unsigned cpu)
{
	struct emelf *e = NULL;

	if ((type <= EMELF_UNKNOWN) || (type >= EMELF_TYPE_MAX)) {
		goto cleanup;
	}

	e = calloc(1, SIZE_EMELF);
	if (!e) {
		goto cleanup;
	}

	// fill in header
	strncpy(e->eh.magic, EMELF_MAGIC, EMELF_MAGIC_LEN);
	e->eh.version = EMELF_VER;
	e->eh.type = type;
	e->eh.cpu = cpu;

	// update max addr according to CPU
	switch (e->eh.cpu) {
		case EMELF_CPU_MX16:
			e->amax = IMAGE_MAX_MX16;
			break;
		case EMELF_CPU_MERA400:
			e->amax = IMAGE_MAX_MERA400;
			break;
		default:
			goto cleanup;
	}

	return e;

cleanup:
	free(e);
	return NULL;
}

// -----------------------------------------------------------------------
void emelf_destroy(struct emelf *e)
{
	if (!e) {
		return;
	}

	free(e->section);
	free(e->reloc);
	free(e->symbol);
	free(e->symbol_names);
	edh_destroy(e->hsymbol);
	free(e);
}

// -----------------------------------------------------------------------
int emelf_entry_set(struct emelf *e, unsigned a)
{
	assert(e);

	if (a > e->amax) {
		return EMELF_E_ADDR;
	}

	e->eh.entry = a;
	e->eh.flags |= EMELF_FLAG_ENTRY;

	return EMELF_E_OK;
}

// -----------------------------------------------------------------------
int emelf_section_add(struct emelf *e, int type)
{
	assert(e);

	if (e->eh.sec_count >= 65535) {
		return EMELF_E_COUNT;
	}

	// reallocate sections if necessary
	while (e->eh.sec_count >= e->section_slots) {
		e->section = realloc(e->section, ALLOC_SEGMENT * SIZE_SECTION);
		if (!e->section) {
			return EMELF_E_ALLOC;
		}
		e->section_slots += ALLOC_SEGMENT;
	}

	e->section[e->eh.sec_count].type = type;
	e->eh.sec_count++;

	return EMELF_E_OK;
}

// -----------------------------------------------------------------------
int emelf_image_append(struct emelf *e, uint16_t *i, unsigned ilen)
{
	assert(e);

	int res;

	if (!ilen) {
		return EMELF_E_OK;
	}

	// add image section
	if (e->image_size <= 0) {
		res = emelf_section_add(e, EMELF_SEC_IMAGE);
		if (res != EMELF_E_OK) {
			return res;
		}
	}

	if (e->image_size + ilen > e->amax) {
		return EMELF_E_ADDR;
	}

	memcpy(e->image + e->image_size, i, SIZE_WORD * ilen);
	e->image_size += ilen;

	return EMELF_E_OK;
}

// -----------------------------------------------------------------------
int emelf_reloc_add(struct emelf *e, unsigned addr, unsigned flags, int sym_idx)
{
	assert(e);

	int res;

	if (e->reloc_count >= 65535) {
		return EMELF_E_COUNT;
	}

	if (addr > e->amax) {
		return EMELF_E_ADDR;
	}

	// add reloc section
	if (!e->reloc_slots) {
		res = emelf_section_add(e, EMELF_SEC_RELOC);
		if (res != EMELF_E_OK) {
			return res;
		}
	}

	// reallocate relocations if necessary
	while (e->reloc_count >= e->reloc_slots) {
		e->reloc = realloc(e->reloc, ALLOC_SEGMENT * SIZE_RELOC);
		if (!e->reloc) {
			return EMELF_E_ALLOC;
		}
		e->reloc_slots += ALLOC_SEGMENT;
	}

	struct emelf_reloc *r = e->reloc + e->reloc_count;

	r->addr = addr;
	r->flags = flags;
	r->sym_idx = sym_idx;

	e->reloc_count++;

	return EMELF_E_OK;
}

// -----------------------------------------------------------------------
int emelf_symbol_add(struct emelf *e, unsigned flags, char *sym_name, uint16_t value)
{
	assert(e);

	int res;
	struct emelf_symbol *sym;

	if (e->symbol_count >= 65535) {
		return EMELF_E_COUNT;
	}

	// add symbol sections and hash if none
	if (!e->symbol_slots) {
		res = emelf_section_add(e, EMELF_SEC_SYM);
		if (res != EMELF_E_OK) {
			emelf_errno = res;
			return -1;
		}
		res = emelf_section_add(e, EMELF_SEC_SYM_NAMES);
		if (res != EMELF_E_OK) {
			emelf_errno = res;
			return -1;
		}
		e->hsymbol = edh_create(16000);
	}

	// if symbol is defined, return its index
	sym = edh_get(e->hsymbol, sym_name);
	if (sym) {
		return sym - e->symbol;
	}

	// pad symbol names to 16-bit
	int sym_name_len = strlen(sym_name) + 1;
	if (sym_name_len % 2) sym_name_len++;

	// realloc symbol_names if necessary
	while (e->symbol_names_len + sym_name_len >= e->symbol_names_space) {
		e->symbol_names = realloc(e->symbol_names, ALLOC_SEGMENT);
		if (!e->symbol_names) {
			emelf_errno = EMELF_E_ALLOC;
			return -1;
		}
		e->symbol_names_space += ALLOC_SEGMENT;
	}

	// realloc symbols if necessary
	while (e->symbol_count >= e->symbol_slots) {
		e->symbol = realloc(e->symbol, ALLOC_SEGMENT * SIZE_SYMBOL);
		if (!e->symbol) {
			emelf_errno = EMELF_E_ALLOC;
			return -1;
		}
		e->symbol_slots += ALLOC_SEGMENT;
	}

	struct emelf_symbol *s = e->symbol + e->symbol_count;

	// store symbol
	s->flags = flags;
	s->offset = e->symbol_names_len;
	s->value = value;

	// store symbol name (and pad)
	strcpy(e->symbol_names + e->symbol_names_len, sym_name);
	e->symbol_names_len += sym_name_len;
	if (sym_name_len % 2) e->symbol_names[e->symbol_names_len-1] = '\0';

	edh_add(e->hsymbol, sym_name, e->symbol + e->symbol_count);

	e->symbol_count++;

	return e->symbol_count-1;
}

// -----------------------------------------------------------------------
struct emelf_symbol * emelf_symbol_get(struct emelf *e, char *sym_name)
{
	return edh_get(e->hsymbol, sym_name);
}

// -----------------------------------------------------------------------
struct emelf * emelf_load(FILE *f)
{
	int i;
	int res;

	struct emelf *e = calloc(1, SIZE_EMELF);
	if (!e) {
		emelf_errno = EMELF_E_ALLOC;
		goto cleanup;
	}

	// load header
	res = fread(e, EMELF_MAGIC_LEN, 1, f);
	if (res < 0) {
		emelf_errno = EMELF_E_FREAD;
		goto cleanup;
	}
	res = nfread((char*)e + EMELF_MAGIC_LEN, SIZE_HEADER - EMELF_MAGIC_LEN, 1, f);
	if (res < 0) {
		emelf_errno = EMELF_E_FREAD;
		goto cleanup;
	}

	// header checks
	if (strncmp(e->eh.magic, EMELF_MAGIC, EMELF_MAGIC_LEN)) {
		emelf_errno = EMELF_E_MAGIC;
		goto cleanup;
	}
	if (e->eh.version != EMELF_VER) {
		emelf_errno = EMELF_E_VERSION;
		goto cleanup;
	}

	// load section list
	e->section = malloc(SIZE_SECTION * e->eh.sec_count);
	if (!e->section) {
		emelf_errno = EMELF_E_ALLOC;
		goto cleanup;
	}

	int section_hdr = ((int) (e->eh.sec_header_hi) << 16) + e->eh.sec_header_lo;
	res = fseek(f, section_hdr, SEEK_SET);
	if (res < 0) {
		emelf_errno = EMELF_E_FREAD;
		goto cleanup;
	}
	res = nfread(e->section, SIZE_SECTION, e->eh.sec_count, f);
	if (res < 0) {
		emelf_errno = EMELF_E_FREAD;
		goto cleanup;
	}

	// load sections
	for (i=0 ; i<e->eh.sec_count ; i++) {

		struct emelf_section *sec = e->section + i;

		fseek(f, sec->offset, SEEK_SET);

		switch (sec->type) {
			case EMELF_SEC_IMAGE:
				res = nfread(e->image, SIZE_WORD, sec->size, f);
				e->image_size = res;
				break;
			case EMELF_SEC_RELOC:
				e->reloc = malloc(SIZE_RELOC * sec->size);
				res = nfread(e->reloc, SIZE_RELOC, sec->size, f);
				e->reloc_count = e->reloc_slots = res;
				break;
			case EMELF_SEC_SYM:
				e->symbol = malloc(SIZE_SYMBOL * sec->size);
				res = nfread(e->symbol, SIZE_SYMBOL, sec->size, f);
				e->symbol_count = e->symbol_slots = res;
				break;
			case EMELF_SEC_SYM_NAMES:
				e->symbol_names = malloc(SIZE_CHAR * sec->size);
				res = fread(e->symbol_names, SIZE_CHAR, sec->size, f);
				e->symbol_names_len = e->symbol_names_space = res;
				break;
			case EMELF_SEC_DEBUG:
				res = 0;
				break;
			case EMELF_SEC_IDENT:
				res = 0;
				break;
			default:
				res = 0;
				emelf_errno = EMELF_E_SECTION;
				goto cleanup;
		}

		if (res < 0) {
			emelf_errno = EMELF_E_FREAD;
			goto cleanup;
		}
	}

	// update symbol hash
	if (e->symbol_slots) {
		e->hsymbol = edh_create(16000);
		for (i=0 ; i<e->symbol_count ; i++) {
			edh_add(e->hsymbol, e->symbol_names + e->symbol[i].offset, e->symbol + i);
		}
	}

	return e;

cleanup:
	free(e);
	return NULL;
}

// -----------------------------------------------------------------------
int emelf_header_write(struct emelf *e, FILE *f)
{
	assert(e);

	int res;
	
	rewind(f);

	res = fwrite(e, EMELF_MAGIC_LEN, 1, f);
	if (res < 0) {
		return EMELF_E_FWRITE;
	}

	res = nfwrite((char*)e + EMELF_MAGIC_LEN, SIZE_HEADER - EMELF_MAGIC_LEN, 1, f);
	if (res < 0) {
		return EMELF_E_FWRITE;
	}

	return EMELF_E_OK;
}

// -----------------------------------------------------------------------
int emelf_write(struct emelf *e, FILE *f)
{
	assert(e);

	int i;
	int res = 0;

	// write header
	res = emelf_header_write(e, f);
	if (res != EMELF_E_OK) {
		return res;
	}

	// write section contents
	for (i=0 ; i<e->eh.sec_count ; i++) {
		e->section[i].offset = ftell(f);
		switch (e->section[i].type) {
			case EMELF_SEC_IMAGE:
				res = nfwrite(e->image, SIZE_WORD, e->image_size, f);
				break;
			case EMELF_SEC_RELOC:
				res = nfwrite(e->reloc, SIZE_RELOC, e->reloc_count, f);
				break;
			case EMELF_SEC_SYM:
				res = nfwrite(e->symbol, SIZE_SYMBOL, e->symbol_count, f);
				break;
			case EMELF_SEC_SYM_NAMES:
				res = fwrite(e->symbol_names, SIZE_CHAR, e->symbol_names_len, f);
				break;
			case EMELF_SEC_DEBUG:
				res = 0;
				break;
			case EMELF_SEC_IDENT:
				res = 0;
				break;
			default:
				res = 0;
				return EMELF_E_SECTION;
		}

		if (res < 0) {
			return EMELF_E_FWRITE;
		}

		e->section[i].size = res;
	}

	int section_hdr = ftell(f);
	if (section_hdr < 0) {
		return EMELF_E_FWRITE;
	}
	e->eh.sec_header_hi = section_hdr >> 16;
	e->eh.sec_header_lo = section_hdr & 65535;

	// write sections
	res = nfwrite(e->section, SIZE_SECTION, e->eh.sec_count, f);
	if (res < 0) {
		return EMELF_E_FWRITE;
	}

	// rewrite header (updates section location)
	res = emelf_header_write(e, f);
	if (res != EMELF_E_OK) {
		return res;
	}

	return EMELF_E_OK;
}

// -----------------------------------------------------------------------
int emelf_has_entry(struct emelf *e)
{
	return (e->eh.flags & EMELF_FLAG_ENTRY) ? 1 : 0;
}

// vim: tabstop=4 autoindent
