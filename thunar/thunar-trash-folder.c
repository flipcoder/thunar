/* $Id$ */
/*-
 * Copyright (c) 2005 Benedikt Meurer <benny@xfce.org>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <thunar/thunar-folder.h>
#include <thunar/thunar-trash-folder.h>



enum
{
  PROP_0,
  PROP_CORRESPONDING_FILE,
  PROP_FILES,
};



static void               thunar_trash_folder_class_init              (ThunarTrashFolderClass *klass);
static void               thunar_trash_folder_folder_init             (ThunarFolderIface      *iface);
static void               thunar_trash_folder_init                    (ThunarTrashFolder      *trash_folder);
static void               thunar_trash_folder_finalize                (GObject                *object);
static void               thunar_trash_folder_get_property            (GObject                *object,
                                                                       guint                   prop_id,
                                                                       GValue                 *value,
                                                                       GParamSpec             *pspec);
static ThunarFolder      *thunar_trash_folder_open_as_folder          (ThunarFile             *file,
                                                                       GError                **error);
static ThunarVfsURI      *thunar_trash_folder_get_uri                 (ThunarFile             *file);
static ExoMimeInfo       *thunar_trash_folder_get_mime_info           (ThunarFile             *file);
static const gchar       *thunar_trash_folder_get_display_name        (ThunarFile             *file);
static ThunarVfsFileType  thunar_trash_folder_get_kind                (ThunarFile             *file);
static ThunarVfsFileMode  thunar_trash_folder_get_mode                (ThunarFile             *file);
static ThunarVfsFileSize  thunar_trash_folder_get_size                (ThunarFile             *file);
static const gchar       *thunar_trash_folder_get_icon_name           (ThunarFile             *file,
                                                                       GtkIconTheme           *icon_theme);
static ThunarFile        *thunar_trash_folder_get_corresponding_file  (ThunarFolder           *folder);
static GSList            *thunar_trash_folder_get_files               (ThunarFolder           *folder);



struct _ThunarTrashFolderClass
{
  ThunarFileClass __parent__;
};

struct _ThunarTrashFolder
{
  ThunarFile __parent__;

  ThunarVfsTrashManager *manager;
  ThunarVfsURI          *uri;
};



G_DEFINE_TYPE_WITH_CODE (ThunarTrashFolder,
                         thunar_trash_folder,
                         THUNAR_TYPE_FILE,
                         G_IMPLEMENT_INTERFACE (THUNAR_TYPE_FOLDER,
                                                thunar_trash_folder_folder_init));



static void
thunar_trash_folder_class_init (ThunarTrashFolderClass *klass)
{
  ThunarFileClass *thunarfile_class;
  GObjectClass    *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = thunar_trash_folder_finalize;
  gobject_class->get_property = thunar_trash_folder_get_property;

  thunarfile_class = THUNAR_FILE_CLASS (klass);
  thunarfile_class->open_as_folder = thunar_trash_folder_open_as_folder;
  thunarfile_class->get_uri = thunar_trash_folder_get_uri;
  thunarfile_class->get_mime_info = thunar_trash_folder_get_mime_info;
  thunarfile_class->get_display_name = thunar_trash_folder_get_display_name;
  thunarfile_class->get_kind = thunar_trash_folder_get_kind;
  thunarfile_class->get_mode = thunar_trash_folder_get_mode;
  thunarfile_class->get_size = thunar_trash_folder_get_size;
  thunarfile_class->get_icon_name = thunar_trash_folder_get_icon_name;

  g_object_class_override_property (gobject_class,
                                    PROP_CORRESPONDING_FILE,
                                    "corresponding-file");

  g_object_class_override_property (gobject_class,
                                    PROP_FILES,
                                    "files");
}



static void
thunar_trash_folder_folder_init (ThunarFolderIface *iface)
{
  iface->get_corresponding_file = thunar_trash_folder_get_corresponding_file;
  iface->get_files = thunar_trash_folder_get_files;
}



static void
thunar_trash_folder_init (ThunarTrashFolder *trash_folder)
{
  /* register with the trash manager */
  trash_folder->manager = thunar_vfs_trash_manager_get_default ();
  g_signal_connect_swapped (G_OBJECT (trash_folder->manager), "notify::empty",
                            G_CALLBACK (thunar_file_changed), trash_folder);
}



static void
thunar_trash_folder_finalize (GObject *object)
{
  ThunarTrashFolder *trash_folder = THUNAR_TRASH_FOLDER (object);

  /* unregister from the trash manager */
  g_signal_handlers_disconnect_by_func (G_OBJECT (trash_folder->manager), thunar_file_changed, trash_folder);
  g_object_unref (G_OBJECT (trash_folder->manager));

  G_OBJECT_CLASS (thunar_trash_folder_parent_class)->finalize (object);
}



static void
thunar_trash_folder_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  ThunarFolder *folder = THUNAR_FOLDER (object);

  switch (prop_id)
    {
    case PROP_CORRESPONDING_FILE:
      g_value_set_object (value, thunar_folder_get_corresponding_file (folder));
      break;

    case PROP_FILES:
      g_value_set_pointer (value, thunar_folder_get_files (folder));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static ThunarFolder*
thunar_trash_folder_open_as_folder (ThunarFile *file,
                                    GError    **error)
{
  return THUNAR_FOLDER (g_object_ref (G_OBJECT (file)));
}



static ThunarVfsURI*
thunar_trash_folder_get_uri (ThunarFile *file)
{
  return THUNAR_TRASH_FOLDER (file)->uri;
}



static ExoMimeInfo*
thunar_trash_folder_get_mime_info (ThunarFile *file)
{
  return NULL;
}



static const gchar*
thunar_trash_folder_get_display_name (ThunarFile *file)
{
  return _("Trash");
}



static ThunarVfsFileType
thunar_trash_folder_get_kind (ThunarFile *file)
{
  return THUNAR_VFS_FILE_TYPE_DIRECTORY;
}



static ThunarVfsFileMode
thunar_trash_folder_get_mode (ThunarFile *file)
{
  return THUNAR_VFS_FILE_MODE_USR_ALL;
}



static ThunarVfsFileSize
thunar_trash_folder_get_size (ThunarFile *file)
{
  return 0;
}



static const gchar*
thunar_trash_folder_get_icon_name (ThunarFile   *file,
                                 GtkIconTheme *icon_theme)
{
  ThunarTrashFolder *trash_folder = THUNAR_TRASH_FOLDER (file);
  const gchar     *icon_name;

  /* determine the proper icon for the trash state */
  icon_name = thunar_vfs_trash_manager_is_empty (trash_folder->manager)
            ? "gnome-fs-trash-empty" : "gnome-fs-trash-full";

  /* check if the icon is present in the icon theme */
  if (gtk_icon_theme_has_icon (icon_theme, icon_name))
    return icon_name;

  return NULL;
}



static ThunarFile*
thunar_trash_folder_get_corresponding_file (ThunarFolder *folder)
{
  return THUNAR_FILE (folder);
}



static GSList*
thunar_trash_folder_get_files (ThunarFolder *folder)
{
  return NULL;
}



/**
 * thunar_trash_folder_new:
 * @uri   : the #ThunarVfsURI referrring to the trash file.
 * @error : return location for errors or %NULL.
 *
 * Allocates a new #ThunarTrashFolder object for the given @uri.
 * Returns %NULL on error, and sets @error to point to an
 * #GError object describing the cause, if the operation fails.
 *
 * You should not ever call this function directly. Instead
 * use the #thunar_file_get_for_uri method, which performs
 * some caching of #ThunarFile objects.
 *
 * Return value: the newly allocated #ThunarTrashFolder instance
 *               or %NULL on error.
 **/
ThunarFile*
thunar_trash_folder_new (ThunarVfsURI *uri,
                       GError      **error)
{
  ThunarTrashFolder *trash_folder;

  g_return_val_if_fail (THUNAR_VFS_IS_URI (uri), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);
  g_return_val_if_fail (thunar_vfs_uri_get_scheme (uri) == THUNAR_VFS_URI_SCHEME_TRASH, NULL);

  /* take an additional reference on the uri */
  g_object_ref (G_OBJECT (uri));

  /* allocate the new object */
  trash_folder = g_object_new (THUNAR_TYPE_TRASH_FOLDER, NULL);
  trash_folder->uri = uri;

  return THUNAR_FILE (trash_folder);
}




