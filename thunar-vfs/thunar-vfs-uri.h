/* $Id$ */
/*-
 * Copyright (c) 2004-2005 Benedikt Meurer <benny@xfce.org>
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

#ifndef __THUNAR_VFS_URI_H__
#define __THUNAR_VFS_URI_H__

#include <glib-object.h>

G_BEGIN_DECLS;

typedef struct _ThunarVfsURIClass ThunarVfsURIClass;
typedef struct _ThunarVfsURI      ThunarVfsURI;

#define THUNAR_VFS_TYPE_URI             (thunar_vfs_uri_get_type ())
#define THUNAR_VFS_URI(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), THUNAR_VFS_TYPE_URI, ThunarVfsURI))
#define THUNAR_VFS_URI_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), THUNAR_VFS_TYPE_URI, ThunarVfsURIClass))
#define THUNAR_VFS_IS_URI(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), THUNAR_VFS_TYPE_URI))
#define THUNAR_VFS_IS_URI_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), THUNAR_VFS_TYPE_URI))
#define THUNAR_VFS_URI_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), THUNAR_VFS_TYPE_URI, ThunarVfsURIClass))

/**
 * ThunarVfsURIHideOptions:
 * @THUNAR_VFS_URI_HIDE_HOST : Hide the hostname from the URI string.
 *
 * Determines which parts of the uri should be hidden when
 * requesting the string representation of a #ThunarVfsURI 
 * object.
 **/
typedef enum
{
  THUNAR_VFS_URI_HIDE_HOST = 1 << 0,
} ThunarVfsURIHideOptions;

/**
 * ThunarVfsURIScheme:
 * @THUNAR_VFS_URI_SCHEME_COMPUTER : 'computer://' uris
 * @THUNAR_VFS_URI_SCHEME_FILE     : 'file://' uris
 * @THUNAR_VFS_URI_SCHEME_TRASH    : 'trash://' uris
 *
 * Currently supported URI types for #ThunarVfsURI.
 **/
typedef enum
{
  THUNAR_VFS_URI_SCHEME_COMPUTER,
  THUNAR_VFS_URI_SCHEME_FILE,
  THUNAR_VFS_URI_SCHEME_TRASH,
} ThunarVfsURIScheme;

GType              thunar_vfs_uri_get_type          (void) G_GNUC_CONST;

ThunarVfsURI      *thunar_vfs_uri_new               (const gchar            *identifier,
                                                     GError                **error) G_GNUC_MALLOC;
ThunarVfsURI      *thunar_vfs_uri_new_for_path      (const gchar            *path) G_GNUC_MALLOC;

ThunarVfsURI      *thunar_vfs_uri_ref               (ThunarVfsURI           *uri);
void               thunar_vfs_uri_unref             (ThunarVfsURI           *uri);

gboolean           thunar_vfs_uri_is_home           (ThunarVfsURI           *uri);
gboolean           thunar_vfs_uri_is_local          (ThunarVfsURI           *uri);
gboolean           thunar_vfs_uri_is_root           (ThunarVfsURI           *uri);

gchar             *thunar_vfs_uri_get_display_name  (ThunarVfsURI           *uri);
const gchar       *thunar_vfs_uri_get_host          (ThunarVfsURI           *uri);
const gchar       *thunar_vfs_uri_get_name          (ThunarVfsURI           *uri);
const gchar       *thunar_vfs_uri_get_path          (ThunarVfsURI           *uri);
ThunarVfsURIScheme thunar_vfs_uri_get_scheme        (ThunarVfsURI           *uri);

ThunarVfsURI      *thunar_vfs_uri_parent            (ThunarVfsURI           *uri) G_GNUC_MALLOC;
ThunarVfsURI      *thunar_vfs_uri_relative          (ThunarVfsURI           *uri,
                                                     const gchar            *name) G_GNUC_MALLOC;

gchar             *thunar_vfs_uri_to_string         (ThunarVfsURI           *uri,
                                                     ThunarVfsURIHideOptions hide_options);

guint              thunar_vfs_uri_hash              (gconstpointer           uri);
gboolean           thunar_vfs_uri_equal             (gconstpointer           a,
                                                     gconstpointer           b);


GList             *thunar_vfs_uri_list_from_string  (const gchar            *string,
                                                     GError                **error);
gchar             *thunar_vfs_uri_list_to_string    (GList                  *uri_list,
                                                     ThunarVfsURIHideOptions hide_options);
GList             *thunar_vfs_uri_list_copy         (GList                  *uri_list);
void               thunar_vfs_uri_list_free         (GList                  *uri_list);

#define thunar_vfs_uri_list_append(uri_list, uri) \
  g_list_append ((uri_list), thunar_vfs_uri_ref ((uri)))
#define thunar_vfs_uri_list_prepend(uri_list, uri) \
  g_list_prepend ((uri_list), thunar_vfs_uri_ref ((uri)))

G_END_DECLS;

#endif /* !__THUNAR_VFS_URI_H__ */