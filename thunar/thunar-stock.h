/* $Id$ */
/*-
 * Copyright (c) 2006 Benedikt Meurer <benny@xfce.org>.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __THUNAR_STOCK_H__
#define __THUNAR_STOCK_H__

G_BEGIN_DECLS;

#define THUNAR_STOCK_DESKTOP      "thunar-desktop"      /* see ThunarLauncher */
#define THUNAR_STOCK_HOME         "thunar-home"         /* see ThunarWindow */
#define THUNAR_STOCK_SHORTCUTS    "thunar-shortcuts"    /* see ThunarShortcutsPane */
#define THUNAR_STOCK_TEMPLATES    "thunar-templates"    /* see ThunarWindow */
#define THUNAR_STOCK_TRASH_EMPTY  "thunar-trash"        /* see ThunarTrashAction */
#define THUNAR_STOCK_TRASH_FULL   "thunar-trash-full"   /* see ThunarTrashAction */
#define THUNAR_STOCK_DIRECTORY    GTK_STOCK_DIRECTORY
#define THUNAR_STOCK_DOWNLOADS    "thunar-downloads"
#define THUNAR_STOCK_DOCUMENTS    "thunar-documents"
#define THUNAR_STOCK_MUSIC        "thunar-music"
#define THUNAR_STOCK_PICTURES     "thunar-pictures"
#define THUNAR_STOCK_VIDEOS       "thunar-videos"
#define THUNAR_STOCK_PUBLIC       "thunar-public"

void thunar_stock_init (void);

G_END_DECLS;

#endif /* !__THUNAR_STOCK_H__ */
