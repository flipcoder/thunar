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

#include <thunar/thunar-local-file.h>
#include <thunar/thunar-local-folder.h>



static void               thunar_local_file_class_init        (ThunarLocalFileClass   *klass);
static void               thunar_local_file_init              (ThunarLocalFile        *local_file);
static void               thunar_local_file_finalize          (GObject                *object);
static ThunarFile        *thunar_local_file_get_parent        (ThunarFile             *file,
                                                               GError                **error);
static ThunarFolder      *thunar_local_file_open_as_folder    (ThunarFile             *file,
                                                               GError                **error);
static ThunarVfsURI      *thunar_local_file_get_uri           (ThunarFile             *file);
static ThunarVfsMimeInfo *thunar_local_file_get_mime_info     (ThunarFile             *file);
static const gchar       *thunar_local_file_get_display_name  (ThunarFile             *file);
static const gchar       *thunar_local_file_get_special_name  (ThunarFile             *file);
static gboolean           thunar_local_file_get_date          (ThunarFile             *file,
                                                               ThunarFileDateType      date_type,
                                                               ThunarVfsFileTime      *date_return);
static ThunarVfsFileType  thunar_local_file_get_kind          (ThunarFile             *file);
static ThunarVfsFileMode  thunar_local_file_get_mode          (ThunarFile             *file);
static gboolean           thunar_local_file_get_size          (ThunarFile             *file,
                                                               ThunarVfsFileSize      *size_return);
static ThunarVfsVolume   *thunar_local_file_get_volume        (ThunarFile             *file,
                                                               ThunarVfsVolumeManager *volume_manager);
static ThunarVfsGroup    *thunar_local_file_get_group         (ThunarFile             *file);
static ThunarVfsUser     *thunar_local_file_get_user          (ThunarFile             *file);
static GList             *thunar_local_file_get_emblem_names  (ThunarFile             *file);
static const gchar       *thunar_local_file_get_icon_name     (ThunarFile             *file,
                                                               GtkIconTheme           *icon_theme);
static void               thunar_local_file_watch             (ThunarFile             *file);
static void               thunar_local_file_unwatch           (ThunarFile             *file);
static void               thunar_local_file_monitor           (ThunarVfsMonitor       *monitor,
                                                               ThunarVfsMonitorEvent   event,
                                                               ThunarVfsInfo          *info,
                                                               gpointer                user_data);



struct _ThunarLocalFileClass
{
  ThunarFileClass __parent__;

  ThunarVfsUserManager *user_manager;
};

struct _ThunarLocalFile
{
  ThunarFile __parent__;

  gchar         *display_name;
  ThunarVfsInfo *info;

  gint           watch_id;
};



static ThunarVfsMonitor *monitor = NULL;

G_DEFINE_TYPE (ThunarLocalFile, thunar_local_file, THUNAR_TYPE_FILE);



static void
thunar_local_file_class_init (ThunarLocalFileClass *klass)
{
  ThunarFileClass *thunarfile_class;
  GObjectClass    *gobject_class;

  /* query the user manager instance */
  klass->user_manager = thunar_vfs_user_manager_get_default ();

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = thunar_local_file_finalize;

  thunarfile_class = THUNAR_FILE_CLASS (klass);
  thunarfile_class->get_parent = thunar_local_file_get_parent;
  thunarfile_class->open_as_folder = thunar_local_file_open_as_folder;
  thunarfile_class->get_uri = thunar_local_file_get_uri;
  thunarfile_class->get_mime_info = thunar_local_file_get_mime_info;
  thunarfile_class->get_display_name = thunar_local_file_get_display_name;
  thunarfile_class->get_special_name = thunar_local_file_get_special_name;
  thunarfile_class->get_date = thunar_local_file_get_date;
  thunarfile_class->get_kind = thunar_local_file_get_kind;
  thunarfile_class->get_mode = thunar_local_file_get_mode;
  thunarfile_class->get_size = thunar_local_file_get_size;
  thunarfile_class->get_volume = thunar_local_file_get_volume;
  thunarfile_class->get_group = thunar_local_file_get_group;
  thunarfile_class->get_user = thunar_local_file_get_user;
  thunarfile_class->get_emblem_names = thunar_local_file_get_emblem_names;
  thunarfile_class->get_icon_name = thunar_local_file_get_icon_name;
  thunarfile_class->watch = thunar_local_file_watch;
  thunarfile_class->unwatch = thunar_local_file_unwatch;
}



static void
thunar_local_file_init (ThunarLocalFile *local_file)
{
  local_file->info = NULL;
}



static void
thunar_local_file_finalize (GObject *object)
{
  ThunarLocalFile *local_file = THUNAR_LOCAL_FILE (object);

#ifndef G_DISABLE_CHECKS
  if (G_UNLIKELY (local_file->watch_id != 0))
    {
      g_error ("Attempt to finalize a ThunarLocalFile, which "
               "is still being watched for changes");
    }
#endif

  /* reset the vfs info (freeing the specific data) */
  if (G_LIKELY (local_file->info != NULL))
    thunar_vfs_info_unref (local_file->info);

  /* free the display name */
  g_free (local_file->display_name);

  G_OBJECT_CLASS (thunar_local_file_parent_class)->finalize (object);
}



static ThunarFile*
thunar_local_file_get_parent (ThunarFile *file,
                              GError    **error)
{
  ThunarLocalFile *local_file = THUNAR_LOCAL_FILE (file);
  ThunarVfsURI    *computer_uri;
  ThunarFile      *computer_file;

  /* the root file system node has 'computer:' as its parent */
  if (G_UNLIKELY (thunar_vfs_uri_is_root (local_file->info->uri)))
    {
      computer_uri = thunar_vfs_uri_new ("computer:", error);
      if (G_LIKELY (computer_uri != NULL))
        {
          computer_file = thunar_file_get_for_uri (computer_uri, error);
          thunar_vfs_uri_unref (computer_uri);
        }
      else
        {
          computer_file = NULL;
        }
      return computer_file;
    }

  /* other file uris are handled by the generic get_parent() method */
  return THUNAR_FILE_CLASS (thunar_local_file_parent_class)->get_parent (file, error);
}



static ThunarFolder*
thunar_local_file_open_as_folder (ThunarFile *file,
                                  GError    **error)
{
  return thunar_local_folder_get_for_file (THUNAR_LOCAL_FILE (file), error);
}



static ThunarVfsURI*
thunar_local_file_get_uri (ThunarFile *file)
{
  return THUNAR_LOCAL_FILE (file)->info->uri;
}



static ThunarVfsMimeInfo*
thunar_local_file_get_mime_info (ThunarFile *file)
{
  return thunar_vfs_mime_info_ref (THUNAR_LOCAL_FILE (file)->info->mime_info);
}



static const gchar*
thunar_local_file_get_display_name (ThunarFile *file)
{
  ThunarLocalFile *local_file = THUNAR_LOCAL_FILE (file);

  /* root directory is always displayed as 'Filesystem' */
  if (thunar_vfs_uri_is_root (local_file->info->uri))
    return _("Filesystem");

  /* determine the display name on-demand */
  if (G_UNLIKELY (local_file->display_name == NULL))
    local_file->display_name = thunar_vfs_uri_get_display_name (local_file->info->uri);

  return local_file->display_name;
}



static const gchar*
thunar_local_file_get_special_name (ThunarFile *file)
{
  ThunarLocalFile *local_file = THUNAR_LOCAL_FILE (file);

  /* root and home dir have special names */
  if (thunar_vfs_uri_is_root (local_file->info->uri))
    return _("Filesystem");
  else if (thunar_vfs_uri_is_home (local_file->info->uri))
    return _("Home");

  return thunar_file_get_display_name (file);
}



static gboolean
thunar_local_file_get_date (ThunarFile        *file,
                            ThunarFileDateType date_type,
                            ThunarVfsFileTime *date_return)
{
  ThunarLocalFile *local_file = THUNAR_LOCAL_FILE (file);

  switch (date_type)
    {
    case THUNAR_FILE_DATE_ACCESSED:
      *date_return = local_file->info->atime;
      break;

    case THUNAR_FILE_DATE_CHANGED:
      *date_return = local_file->info->ctime;
      break;

    case THUNAR_FILE_DATE_MODIFIED:
      *date_return = local_file->info->mtime;
      break;

    default:
      return THUNAR_FILE_GET_CLASS (file)->get_date (file, date_type, date_return);
    }

  return TRUE;
}



static ThunarVfsFileMode
thunar_local_file_get_mode (ThunarFile *file)
{
  return THUNAR_LOCAL_FILE (file)->info->mode;
}



static gboolean
thunar_local_file_get_size (ThunarFile        *file,
                            ThunarVfsFileSize *size_return)
{
  /* we don't return files sizes for directories, as that's confusing */
  if (THUNAR_LOCAL_FILE (file)->info->type == THUNAR_VFS_FILE_TYPE_DIRECTORY)
    return FALSE;

  *size_return = THUNAR_LOCAL_FILE (file)->info->size;
  return TRUE;
}



static ThunarVfsFileType
thunar_local_file_get_kind (ThunarFile *file)
{
  return THUNAR_LOCAL_FILE (file)->info->type;
}



static ThunarVfsVolume*
thunar_local_file_get_volume (ThunarFile             *file,
                              ThunarVfsVolumeManager *volume_manager)
{
  return thunar_vfs_volume_manager_get_volume_by_info (volume_manager, THUNAR_LOCAL_FILE (file)->info);
}



static ThunarVfsGroup*
thunar_local_file_get_group (ThunarFile *file)
{
  return thunar_vfs_user_manager_get_group_by_id (THUNAR_LOCAL_FILE_GET_CLASS (file)->user_manager,
                                                  THUNAR_LOCAL_FILE (file)->info->gid);
}



static ThunarVfsUser*
thunar_local_file_get_user (ThunarFile *file)
{
  return thunar_vfs_user_manager_get_user_by_id (THUNAR_LOCAL_FILE_GET_CLASS (file)->user_manager,
                                                 THUNAR_LOCAL_FILE (file)->info->uid);
}



static GList*
thunar_local_file_get_emblem_names (ThunarFile *file)
{
  ThunarLocalFile *local_file = THUNAR_LOCAL_FILE (file);
  ThunarVfsInfo   *info = local_file->info;
  GList           *emblems = NULL;

  if ((info->flags & THUNAR_VFS_FILE_FLAGS_SYMLINK) != 0)
    emblems = g_list_prepend (emblems, THUNAR_FILE_EMBLEM_NAME_SYMBOLIC_LINK);

  if (!thunar_file_can_read (file))
    emblems = g_list_prepend (emblems, THUNAR_FILE_EMBLEM_NAME_CANT_READ);

  return emblems;
}



static const gchar*
thunar_local_file_get_icon_name (ThunarFile   *file,
                                 GtkIconTheme *icon_theme)
{
  ThunarLocalFile *local_file = THUNAR_LOCAL_FILE (file);
  const gchar     *icon_name;

  /* special icon for the root node */
  if (G_UNLIKELY (thunar_vfs_uri_is_root (local_file->info->uri))
      && gtk_icon_theme_has_icon (icon_theme, "gnome-dev-harddisk"))
    {
      return "gnome-dev-harddisk";
    }

  /* special icon for the home node */
  if (G_UNLIKELY (thunar_vfs_uri_is_home (local_file->info->uri))
      && gtk_icon_theme_has_icon (icon_theme, "gnome-fs-home"))
    {
      return "gnome-fs-home";
    }

  /* default is the mime type icon */
  icon_name = thunar_vfs_mime_info_lookup_icon_name (local_file->info->mime_info, icon_theme);

  return icon_name;
}



static void
thunar_local_file_watch (ThunarFile *file)
{
  ThunarLocalFile *local_file = THUNAR_LOCAL_FILE (file);

  g_return_if_fail (local_file->watch_id == 0);

  /* take a reference on the VFS monitor for this instance */
  if (G_UNLIKELY (monitor == NULL))
    {
      monitor = thunar_vfs_monitor_get_default ();
      g_object_add_weak_pointer (G_OBJECT (monitor), (gpointer) &monitor);
    }
  else
    {
      g_object_ref (G_OBJECT (monitor));
    }

#if 0
  /* add our VFS info to the monitor */
  local_file->watch_id = thunar_vfs_monitor_add_info (monitor, local_file->info,
                                                      thunar_local_file_monitor,
                                                      local_file);
#else
  (void) &thunar_local_file_monitor;
#endif
}



static void
thunar_local_file_unwatch (ThunarFile *file)
{
#if 0
  ThunarLocalFile *local_file = THUNAR_LOCAL_FILE (file);

  g_return_if_fail (local_file->watch_id != 0);

  /* remove our VFS info from the monitor */
  thunar_vfs_monitor_remove (monitor, local_file->watch_id);
  local_file->watch_id = 0;
#endif

  /* release our reference on the VFS monitor */
  g_object_unref (G_OBJECT (monitor));
}



static void
thunar_local_file_monitor (ThunarVfsMonitor     *monitor,
                           ThunarVfsMonitorEvent event,
                           ThunarVfsInfo        *info,
                           gpointer              user_data)
{
  switch (event)
    {
    case THUNAR_VFS_MONITOR_CHANGED:
      thunar_file_changed (THUNAR_FILE (user_data));
      break;

    case THUNAR_VFS_MONITOR_DELETED:
      gtk_object_destroy (GTK_OBJECT (user_data));
      break;

    default:
      break;
    }
}



/**
 * thunar_local_file_new:
 * @uri   : the #ThunarVfsURI referrring to the local file.
 * @error : return location for errors or %NULL.
 *
 * Allocates a new #ThunarLocalFile object for the given @uri.
 * Returns %NULL on error, and sets @error to point to an
 * #GError object describing the cause, if the operation fails.
 *
 * You should not ever call this function directly. Instead
 * use the #thunar_file_get_for_uri method, which performs
 * some caching of #ThunarFile objects.
 *
 * Return value: the newly allocated #ThunarLocalFile instance
 *               or %NULL on error.
 **/
ThunarFile*
thunar_local_file_new (ThunarVfsURI *uri,
                       GError      **error)
{
  ThunarLocalFile *local_file;
  ThunarVfsInfo   *info;

  g_return_val_if_fail (THUNAR_VFS_IS_URI (uri), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);
  g_return_val_if_fail (thunar_vfs_uri_get_scheme (uri) == THUNAR_VFS_URI_SCHEME_FILE, NULL);

  /* query the file info */
  info = thunar_vfs_info_new_for_uri (uri, error);
  if (G_UNLIKELY (info == NULL))
    return NULL;

  /* allocate the new object */
  local_file = g_object_new (THUNAR_TYPE_LOCAL_FILE, NULL);
  local_file->info = info;

  return THUNAR_FILE (local_file);
}



/**
 * thunar_local_file_get_for_info:
 * @info : a #ThunarVfsInfo.
 *
 * Looks up the #ThunarLocalFile corresponding to
 * @info. Use this function if want to specify the
 * #ThunarVfsInfo to use for a #ThunarLocalFile instead
 * of #thunar_file_get_for_uri(), which would determine
 * the #ThunarVfsInfo once again.
 *
 * The caller is responsible to call #g_object_unref()
 * when done with the returned object.
 *
 * Return value: the #ThunarLocalFile for @info.
 **/
ThunarFile*
thunar_local_file_get_for_info (ThunarVfsInfo *info)
{
  ThunarLocalFile *local_file;

  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (THUNAR_VFS_IS_URI (info->uri), NULL);

  /* check if we already have a cached version of that file */
  local_file = THUNAR_LOCAL_FILE (_thunar_file_cache_lookup (info->uri));
  if (G_UNLIKELY (local_file != NULL))
    {
      /* take a reference for the caller */
      g_object_ref (G_OBJECT (local_file));

      /* apply the new info */
      if (!thunar_vfs_info_matches (local_file->info, info))
        {
          thunar_vfs_info_unref (local_file->info);
          local_file->info = thunar_vfs_info_ref (info);
          thunar_file_changed (THUNAR_FILE (local_file));
        }
    }
  else
    {
      /* allocate a new object */
      local_file = g_object_new (THUNAR_TYPE_LOCAL_FILE, NULL);
      local_file->info = thunar_vfs_info_ref (info);

      /* insert the file into the cache (also strips the
       * floating reference from the object).
       */
      _thunar_file_cache_insert (THUNAR_FILE (local_file));
    }

  return THUNAR_FILE (local_file);
}
