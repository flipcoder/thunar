/* $Id$ */
/*-
 * Copyright (c) 2005 Benedikt Meurer <benny@xfce.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <thunar-vfs/thunar-vfs.h>



/**
 * thunar_vfs_init:
 *
 * Initializes the ThunarVFS library.
 **/
void
thunar_vfs_init (void)
{
  extern void _thunar_vfs_job_init (void);
  extern void _thunar_vfs_mime_init (void);

  static gboolean initialized = FALSE;

  if (!initialized)
    {
      _thunar_vfs_job_init ();
      _thunar_vfs_mime_init ();

      initialized = TRUE;
    }
}

