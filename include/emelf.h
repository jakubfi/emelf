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

#ifndef EMELF_H
#define EMELF_H

#include <stdio.h>
#include <inttypes.h>

#define ALLOC_SEGMENT 1024

#define IMAGE_MAX_MERA400 32 * 1024
#define IMAGE_MAX_MX16 64 * 1024
#define IMAGE_MAX IMAGE_MAX_MX16

#define EMELF_MAGIC "\376EMELF"
#define EMELF_MAGIC_LEN 6

#define EMELF_VER 0

#define SIZE_EMELF sizeof(struct emelf)
#define SIZE_WORD sizeof(uint16_t)
#define SIZE_CHAR sizeof(char)
#define SIZE_HEADER sizeof(struct emelf_header)
#define SIZE_SECTION sizeof(struct emelf_section)
#define SIZE_SYMBOL sizeof(struct emelf_symbol)
#define SIZE_RELOC sizeof(struct emelf_reloc)

extern int emelf_errno;

enum emelf_errors {
	EMELF_E_OK = 0,
	EMELF_E_ALLOC,
	EMELF_E_ADDR,
	EMELF_E_COUNT,
	EMELF_E_FREAD,
	EMELF_E_FWRITE,
	EMELF_E_SECTION,
	EMELF_E_MAGIC,
	EMELF_E_VERSION,
};

enum emelf_types {
	EMELF_UNKNOWN,			// unknown object
	EMELF_EXEC,				// executable object
	EMELF_RELOC,			// linkable, relocatable object
	EMELF_CORE,				// core file
	EMELF_TYPE_MAX,
};

enum emelf_flags {
	EMELF_FLAG_NONE		= 0,
	EMELF_FLAG_ENTRY	= 1 << 0,
};

enum emelf_cpu_types {
	EMELF_CPU_UNKNOWN,
	EMELF_CPU_MERA400,
	EMELF_CPU_MX16,
};

enum emelf_abi_types {
	EMELF_ABI_UNKNOWN,
	EMELF_ABI_NONE,
};

enum emelf_section_types {
	EMELF_SEC_UNKNOWN,
	EMELF_SEC_IMAGE,
	EMELF_SEC_RELOC,
	EMELF_SEC_SYM,
	EMELF_SEC_SYM_NAMES,
	EMELF_SEC_DEBUG,
	EMELF_SEC_IDENT,
};

enum emelf_symbol_flags {
	EMELF_SYM_NOFLAGS	= 0,
	EMELF_SYM_GLOBAL	= 1 << 0,
	EMELF_SYM_RELATIVE	= 1 << 1,
};

enum emelf_reloc_flags {
	EMELF_RELOC_NONE	= 0,
	EMELF_RELOC_BASE	= 1 << 0,
	EMELF_RELOC_SYM		= 1 << 1,
	EMELF_RELOC_SYM_NEG	= 1 << 2,
};

struct emelf_header {
	char magic[6];
	uint16_t version;
	uint16_t type;
	uint16_t flags;
	uint16_t cpu;
	uint16_t abi;
	uint16_t abi_version;
	uint16_t entry;
	uint16_t sec_count;
	uint16_t sec_header;
	uint16_t reserved[32];
};

struct emelf_section {
	uint16_t type;
	uint16_t offset;
	uint16_t size;
};

struct emelf_symbol {
	uint16_t value;
	uint16_t flags;
	uint16_t offset;
};

struct emelf_reloc {
	uint16_t addr;
	uint16_t flags;
	uint16_t sym_idx;
};

struct emelf {
	struct emelf_header eh;

	struct emelf_section *section;
	int section_slots;

	unsigned amax;
	uint16_t image[IMAGE_MAX];
	unsigned image_pos;

	struct emelf_reloc *reloc;
	int reloc_slots;
	int reloc_count;

	struct edh_table *hsymbol;
	struct emelf_symbol *symbol;
	char *symbol_names;
	int symbol_slots;
	int symbol_count;
	int symbol_names_space;
	int symbol_names_len;
};

struct emelf * emelf_create(unsigned type, unsigned cpu);
void emelf_destroy(struct emelf *e);

int emelf_section_add(struct emelf *e, int type);

int emelf_entry_set(struct emelf *e, unsigned a);
int emelf_image_append(struct emelf *e, uint16_t *i, unsigned ilen);

int emelf_reloc_add(struct emelf *e, unsigned addr, unsigned flags, int sym_idx);
int emelf_symbol_add(struct emelf *e, unsigned flags, char *sym_name, uint16_t value);
struct emelf_symbol * emelf_symbol_get(struct emelf *e, char *sym_name);

struct emelf * emelf_load(FILE *f);
int emelf_write(struct emelf *e, FILE *f);

int emelf_has_entry(struct emelf *e);

#endif

// vim: tabstop=4 autoindent
