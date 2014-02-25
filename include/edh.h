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

#ifndef DH_H
#define DH_H

struct edh_elem {
	char *name;
	struct emelf_symbol *s;
	struct edh_elem *next;
};

struct edh_table {
	int size;
	struct edh_elem **slots;
};

struct edh_table * edh_create(int size);
unsigned edh_hash(struct edh_table *dh, char *name);
struct emelf_symbol * edh_get(struct edh_table *dh, char *name);
struct emelf_symbol * edh_add(struct edh_table *dh, char *name, struct emelf_symbol *s);
int edh_delete(struct edh_table *dh, char *name);
void edh_destroy(struct edh_table *dh);
void edh_dump_stats(struct edh_table *dh);

#endif

// vim: tabstop=4 autoindent
