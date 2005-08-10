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

#ifndef __THUNAR_VFS_MIME_DATABASE_H__
#define __THUNAR_VFS_MIME_DATABASE_H__

#include <thunar-vfs/thunar-vfs-mime-application.h>
#include <thunar-vfs/thunar-vfs-mime-info.h>

G_BEGIN_DECLS;

typedef struct _ThunarVfsMimeDatabaseClass ThunarVfsMimeDatabaseClass;
typedef struct _ThunarVfsMimeDatabase      ThunarVfsMimeDatabase;

#define THUNAR_VFS_TYPE_MIME_DATABASE             (thunar_vfs_mime_database_get_type ())
#define THUNAR_VFS_MIME_DATABASE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), THUNAR_VFS_TYPE_MIME_DATABASE, ThunarVfsMimeDatabase))
#define THUNAR_VFS_MIME_DATABASE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), THUNAR_VFS_TYPE_MIME_DATABASE, ThunarVfsMimeDatabaseClass))
#define THUNAR_VFS_IS_MIME_DATABASE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), THUNAR_VFS_TYPE_MIME_DATABASE))
#define THUNAR_VFS_IS_MIME_DATABASE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), THUNAR_VFS_TYPE_MIME_DATABASE))
#define THUNAR_VFS_MIME_DATABASE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), THUNAR_VFS_TYPE_MIME_DATABASE, ThunarVfsMimeDatabaseClass))

GType                     thunar_vfs_mime_database_get_type                 (void) G_GNUC_CONST;

ThunarVfsMimeDatabase    *thunar_vfs_mime_database_get                      (void);

ThunarVfsMimeInfo        *thunar_vfs_mime_database_get_info                 (ThunarVfsMimeDatabase *database,
                                                                             const gchar           *mime_type);
ThunarVfsMimeInfo        *thunar_vfs_mime_database_get_info_for_data        (ThunarVfsMimeDatabase *database,
                                                                             gconstpointer          data,
                                                                             gsize                  length);
ThunarVfsMimeInfo        *thunar_vfs_mime_database_get_info_for_name        (ThunarVfsMimeDatabase *database,
                                                                             const gchar           *name);
ThunarVfsMimeInfo        *thunar_vfs_mime_database_get_info_for_file        (ThunarVfsMimeDatabase *database,
                                                                             const gchar           *path,
                                                                             const gchar           *name);

GList                    *thunar_vfs_mime_database_get_applications         (ThunarVfsMimeDatabase *database,
                                                                             ThunarVfsMimeInfo     *info);
ThunarVfsMimeApplication *thunar_vfs_mime_database_get_default_application  (ThunarVfsMimeDatabase *database,
                                                                             ThunarVfsMimeInfo     *info);

G_END_DECLS;

#endif /* !__THUNAR_VFS_MIME_DATABASE_H__ */