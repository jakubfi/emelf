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

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <arpa/inet.h>

#include "emelf.h"

char *input_file;
char *output_image;
int show_header, show_sections, show_relocs, show_symbols;

char *emelf_types_n[] = {
	"UNKNOWN",
	"EXEC",
	"RELOC",
	"CORE"
};

char *emelf_cpu_n[] = {
	"UNKNOWN",
	"MERA-400",
	"MX-16"
};

char *emelf_abi_types_n[] = {
	"UNKNOWN",
	"NONE",
	"V1",
};

char *emelf_section_types_n[] = {
	"UNKNOWN",
	"IMAGE",
	"RELOC",
	"SYM",
	"SYM_NAMES",
	"DEBUG",
	"IDENT"
};

int emelf_elem_sizes[] = {
	0,
	SIZE_WORD,
	SIZE_RELOC,
	SIZE_SYMBOL,
	SIZE_CHAR,
	0,
	SIZE_CHAR
};

// -----------------------------------------------------------------------
void emelf_print_header(struct emelf *e)
{
	printf("EMELF header\n");
	printf("  Magic : \\%o %s\n", *(e->eh.magic)&255, e->eh.magic+1);
	printf("  Ver.  : %i\n", e->eh.version);
	printf("  Type  : %i (%s)\n", e->eh.type, emelf_types_n[e->eh.type]);
	printf("  Flags : %i\n", e->eh.flags);
	printf("  CPU   : %i (%s)\n", e->eh.cpu, emelf_cpu_n[e->eh.cpu]);
	printf("  ABI   : %i (%s)\n", e->eh.abi, emelf_abi_types_n[e->eh.abi]);
	if (e->eh.flags & EMELF_FLAG_ENTRY) {
		printf("  Entry : 0x%04x\n", e->eh.entry);
	} else {
		printf("  Entry : not set\n");
	}
}

// -----------------------------------------------------------------------
void emelf_print_sections(struct emelf *e)
{
	int i;

	if (e->eh.sec_count <= 0) {
		printf("No sections\n");
		return;
	}

	printf("Sections\n");
	printf("      Type       Offset  Chunk  Elems  Bytes\n");
	for (i=0 ; i<e->eh.sec_count ; i++) {
		struct emelf_section *sec = e->section + i;
		printf("  %-3i %-10s %-7i %-6i %-6i %-6i\n",
			i,
			emelf_section_types_n[sec->type],
			sec->offset,
			emelf_elem_sizes[sec->type],
			sec->size,
			emelf_elem_sizes[sec->type] * sec->size
		);
	}
}

// -----------------------------------------------------------------------
void emelf_print_relocs(struct emelf *e)
{
	int i;

	if (e->reloc_count <= 0) {
		printf("No relocations\n");
		return;
	}

	printf("Relocations\n");
	printf("  Addr    Value   Reloc\n");
	for (i=0 ; i<e->reloc_count ; i++) {
		struct emelf_reloc *rel = e->reloc + i;
		printf("  0x%04x  %-7i ", rel->addr, (int16_t) e->image[rel->addr]);
		if (rel->flags & EMELF_RELOC_BASE) {
			printf("@start ");
			if (rel->flags & EMELF_RELOC_SYM) printf("%s %s",
				(rel->flags & EMELF_RELOC_SYM_NEG) ? "-" : "+",
				e->symbol_names + e->symbol[rel->sym_idx].offset
			);
		} else {
			if (rel->flags & EMELF_RELOC_SYM) printf("%s%s",
				(rel->flags & EMELF_RELOC_SYM_NEG) ? "-" : "",
				e->symbol_names + e->symbol[rel->sym_idx].offset
			);
		}
		printf("\n");
	}
}

// -----------------------------------------------------------------------
void emelf_print_symbols(struct emelf *e)
{
	int i;

	if (e->symbol_count <= 0) {
		printf("No symbols\n");
		return;
	}

	printf("Symbols\n");
	for (i=0 ; i<e->symbol_count; i++) {
		struct emelf_symbol *sym = e->symbol + i;
		printf("  %-10s = ", e->symbol_names + sym->offset);
		if (sym->flags & EMELF_SYM_GLOBAL) {
			printf("%i", sym->value);
			if (sym->flags & EMELF_SYM_RELATIVE) printf(" + @start");
		} else {
			printf("?");
		}
		printf("\n");
	}
}

// -----------------------------------------------------------------------
void usage()
{
	printf("Usage: emelfread options file\n");
	printf("Where options are one or more of:\n");
	printf("   -e        : show EMELF header\n");
	printf("   -s        : show sections\n");
	printf("   -r        : show relocations\n");
	printf("   -n        : show symbol names\n");
	printf("   -a        : show all (same as -esrn)\n");
	printf("   -o output : dump image to output file\n");
	printf("   -v        : print version end exit\n");
	printf("   -h        : print help and exit\n");
}

// -----------------------------------------------------------------------
int parse_args(int argc, char **argv)
{
	int option;
	while ((option = getopt(argc, argv,"esrnao:vh")) != -1) {
		switch (option) {
			case 'e':
				show_header = 1;
				break;
			case 's':
				show_sections = 1;
				break;
			case 'r':
				show_relocs = 1;
				break;
			case 'n':
				show_symbols = 1;
				break;
			case 'a':
				show_header = 1;
				show_sections = 1;
				show_relocs = 1;
				show_symbols = 1;
				break;
			case 'o':
				output_image = optarg;
				break;
			case 'h':
				usage();
				exit(0);
				break;
			case 'v':
				printf("EMELFREAD v%s - EMELF reader\n", EMELF_VERSION);
				exit(0);
				break;
			default:
				return -1;
		}
	}

	if (optind == argc-1) {
		input_file = argv[optind];
	} else {
		printf("Wrong usage.\n");
		usage();
		return -1;
	}

	return 0;
}

// -----------------------------------------------------------------------
int main(int argc, char **argv)
{
	int res;
	FILE *f;
	struct emelf *e;

	res = parse_args(argc, argv);
	if (res < 0) {
		exit(res);
	}

	if (show_header+show_sections+show_relocs+show_symbols+output_image == 0) {
		printf("Nothing to do, specify at least one of options: -esrnao\n");
		usage();
		exit(-1);
	}

	f = fopen(input_file, "r");
	if (!f) {
		printf("Cannot open input file '%s'.\n", input_file);
		exit(-1);
	}

	e = emelf_load(f);
	fclose(f);

	if (!e) {
		printf("Cannot read EMELF contents.\n");
		exit(-1);
	}

	if (show_header) {
		emelf_print_header(e);
	}

	if (show_sections) {
		printf("\n");
		emelf_print_sections(e);
	}

	if (show_relocs) {
		printf("\n");
		emelf_print_relocs(e);
	}

	if (show_symbols) {
		printf("\n");
		emelf_print_symbols(e);
	}

	if (output_image) {
		f = fopen(output_image, "w");
		int pos = e->image_size;
		while (pos >= 0) {
			e->image[pos] = htons(e->image[pos]);
			pos--;
		}
		fwrite(e->image, sizeof(uint16_t), e->image_size, f);
		fclose(f);
	}

	emelf_destroy(e);

	return 0;
}

// vim: tabstop=4 autoindent
