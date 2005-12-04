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
 *
 * Inspired by the shortcuts list as found in the GtkFileChooser, which was
 * developed for Gtk+ by Red Hat, Inc.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <thunar/thunar-dialogs.h>
#include <thunar/thunar-shortcuts-model.h>
#include <thunar/thunar-shortcuts-view.h>



/* Identifiers for signals */
enum
{
  SHORTCUT_ACTIVATED,
  LAST_SIGNAL,
};

/* Identifiers for DnD target types */
enum
{
  GTK_TREE_MODEL_ROW,
  TEXT_URI_LIST,
};



static void         thunar_shortcuts_view_class_init             (ThunarShortcutsViewClass *klass);
static void         thunar_shortcuts_view_init                   (ThunarShortcutsView      *view);
static gboolean     thunar_shortcuts_view_button_press_event     (GtkWidget                *widget,
                                                                  GdkEventButton           *event);
static void         thunar_shortcuts_view_drag_data_received     (GtkWidget                *widget,
                                                                  GdkDragContext           *context,
                                                                  gint                      x,
                                                                  gint                      y,
                                                                  GtkSelectionData         *selection_data,
                                                                  guint                     info,
                                                                  guint                     time);
static gboolean     thunar_shortcuts_view_drag_drop              (GtkWidget                *widget,
                                                                  GdkDragContext           *context,
                                                                  gint                      x,
                                                                  gint                      y,
                                                                  guint                     time);
static gboolean     thunar_shortcuts_view_drag_motion            (GtkWidget                *widget,
                                                                  GdkDragContext           *context,
                                                                  gint                      x,
                                                                  gint                      y,
                                                                  guint                     time);
static void         thunar_shortcuts_view_row_activated          (GtkTreeView              *tree_view,
                                                                  GtkTreePath              *path,
                                                                  GtkTreeViewColumn        *column);
static void         thunar_shortcuts_view_remove_activated       (GtkWidget                *item,
                                                                  ThunarShortcutsView      *view);
static void         thunar_shortcuts_view_rename_activated       (GtkWidget                *item,
                                                                  ThunarShortcutsView      *view);
static void         thunar_shortcuts_view_rename_canceled        (GtkCellRenderer          *renderer,
                                                                  ThunarShortcutsView      *view);
static void         thunar_shortcuts_view_renamed                (GtkCellRenderer          *renderer,
                                                                  const gchar              *path_string,
                                                                  const gchar              *text,
                                                                  ThunarShortcutsView      *view);
static GtkTreePath *thunar_shortcuts_view_compute_drop_position  (ThunarShortcutsView      *view,
                                                                  gint                      x,
                                                                  gint                      y);
static void         thunar_shortcuts_view_drop_uri_list          (ThunarShortcutsView      *view,
                                                                  const gchar              *uri_list,
                                                                  GtkTreePath              *dst_path);
#if GTK_CHECK_VERSION(2,6,0)
static gboolean     thunar_shortcuts_view_separator_func         (GtkTreeModel             *model,
                                                                  GtkTreeIter              *iter,
                                                                  gpointer                  user_data);
#endif



struct _ThunarShortcutsViewClass
{
  GtkTreeViewClass __parent__;
};

struct _ThunarShortcutsView
{
  GtkTreeView __parent__;
};



/* Target types for dragging from the shortcuts view */
static const GtkTargetEntry drag_targets[] = {
  { "GTK_TREE_MODEL_ROW", GTK_TARGET_SAME_WIDGET, GTK_TREE_MODEL_ROW },
};

/* Target types for dropping into the shortcuts view */
static const GtkTargetEntry drop_targets[] = {
  { "GTK_TREE_MODEL_ROW", GTK_TARGET_SAME_WIDGET, GTK_TREE_MODEL_ROW },
  { "text/uri-list", 0, TEXT_URI_LIST },
};



static GObjectClass *thunar_shortcuts_view_parent_class;
static guint         view_signals[LAST_SIGNAL];



GType
thunar_shortcuts_view_get_type (void)
{
  static GType type = G_TYPE_INVALID;

  if (G_UNLIKELY (type == G_TYPE_INVALID))
    {
      static const GTypeInfo info =
      {
        sizeof (ThunarShortcutsViewClass),
        NULL,
        NULL,
        (GClassInitFunc) thunar_shortcuts_view_class_init,
        NULL,
        NULL,
        sizeof (ThunarShortcutsView),
        0,
        (GInstanceInitFunc) thunar_shortcuts_view_init,
        NULL,
      };

      type = g_type_register_static (GTK_TYPE_TREE_VIEW, I_("ThunarShortcutsView"), &info, 0);
    }

  return type;
}



static void
thunar_shortcuts_view_class_init (ThunarShortcutsViewClass *klass)
{
  GtkTreeViewClass *gtktree_view_class;
  GtkWidgetClass   *gtkwidget_class;

  /* determine the parent type class */
  thunar_shortcuts_view_parent_class = g_type_class_peek_parent (klass);

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->button_press_event = thunar_shortcuts_view_button_press_event;
  gtkwidget_class->drag_data_received = thunar_shortcuts_view_drag_data_received;
  gtkwidget_class->drag_drop = thunar_shortcuts_view_drag_drop;
  gtkwidget_class->drag_motion = thunar_shortcuts_view_drag_motion;

  gtktree_view_class = GTK_TREE_VIEW_CLASS (klass);
  gtktree_view_class->row_activated = thunar_shortcuts_view_row_activated;

  /**
   * ThunarShortcutsView:shortcut-activated:
   *
   * Invoked whenever a shortcut is activated by the user.
   **/
  view_signals[SHORTCUT_ACTIVATED] =
    g_signal_new (I_("shortcut-activated"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1, THUNAR_TYPE_FILE);
}



static void
thunar_shortcuts_view_init (ThunarShortcutsView *view)
{
  GtkTreeViewColumn *column;
  GtkTreeSelection  *selection;
  GtkCellRenderer   *renderer;

  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (view), FALSE);

  column = g_object_new (GTK_TYPE_TREE_VIEW_COLUMN,
                         "reorderable", FALSE,
                         "resizable", FALSE,
                         "sizing", GTK_TREE_VIEW_COLUMN_AUTOSIZE,
                         NULL);
  renderer = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_column_pack_start (column, renderer, FALSE);
  gtk_tree_view_column_set_attributes (column, renderer,
                                       "pixbuf", THUNAR_SHORTCUTS_MODEL_COLUMN_ICON,
                                       NULL);
  renderer = gtk_cell_renderer_text_new ();
  g_signal_connect (G_OBJECT (renderer), "edited", G_CALLBACK (thunar_shortcuts_view_renamed), view);
  g_signal_connect (G_OBJECT (renderer), "editing-canceled", G_CALLBACK (thunar_shortcuts_view_rename_canceled), view);
  gtk_tree_view_column_pack_start (column, renderer, TRUE);
  gtk_tree_view_column_set_attributes (column, renderer,
                                       "text", THUNAR_SHORTCUTS_MODEL_COLUMN_NAME,
                                       NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

  /* enable drag support for the shortcuts view (actually used to support reordering) */
  gtk_tree_view_enable_model_drag_source (GTK_TREE_VIEW (view), GDK_BUTTON1_MASK, drag_targets,
                                          G_N_ELEMENTS (drag_targets), GDK_ACTION_MOVE);

  /* enable drop support for the shortcuts view (both internal reordering
   * and adding new shortcuts from other widgets)
   */
  gtk_drag_dest_set (GTK_WIDGET (view), GTK_DEST_DEFAULT_ALL,
                     drop_targets, G_N_ELEMENTS (drop_targets),
                     GDK_ACTION_COPY | GDK_ACTION_LINK | GDK_ACTION_MOVE);

#if GTK_CHECK_VERSION(2,6,0)
  gtk_tree_view_set_row_separator_func (GTK_TREE_VIEW (view), thunar_shortcuts_view_separator_func, NULL, NULL);
#endif
}



static gboolean
thunar_shortcuts_view_button_press_event (GtkWidget      *widget,
                                          GdkEventButton *event)
{
  GtkTreeModel *model;
  GtkTreePath  *path;
  GtkTreeIter   iter;
  ThunarFile   *file;
  GtkWidget    *window;
  GtkWidget    *image;
  GtkWidget    *menu;
  GtkWidget    *item;
  GMainLoop    *loop;
  gboolean      mutable;
  gboolean      result;
  GList        *actions;
  GList        *lp;

  /* let the widget process the event first (handles focussing and scrolling) */
  result = (*GTK_WIDGET_CLASS (thunar_shortcuts_view_parent_class)->button_press_event) (widget, event);

  /* check if we have a right-click event */
  if (G_LIKELY (event->button != 3))
    return result;

  /* resolve the path to the cursor position */
  if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget), event->x, event->y, &path, NULL, NULL, NULL))
    {
      /* determine the iterator for the selected row */
      model = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));
      gtk_tree_model_get_iter (model, &iter, path);

      /* check whether the shortcut at the given path is mutable */
      gtk_tree_model_get (model, &iter, THUNAR_SHORTCUTS_MODEL_COLUMN_MUTABLE, &mutable, -1);

      /* determine the file for the given path */
      file = thunar_shortcuts_model_file_for_iter (THUNAR_SHORTCUTS_MODEL (model), &iter);

      /* prepare the internal loop */
      loop = g_main_loop_new (NULL, FALSE);

      /* prepare the popup menu */
      menu = gtk_menu_new ();
      gtk_menu_set_screen (GTK_MENU (menu), gtk_widget_get_screen (widget));
      g_signal_connect_swapped (G_OBJECT (menu), "deactivate", G_CALLBACK (g_main_loop_quit), loop);
      exo_gtk_object_ref_sink (GTK_OBJECT (menu));

      /* prepend the custom actions for the selected file (if any) */
      if (G_LIKELY (file != NULL))
        {
          /* determine the toplevel window */
          window = gtk_widget_get_toplevel (widget);

          /* determine the actions for the selected file */
          actions = thunar_file_get_actions (file, window);

          /* check if we have any actions */
          if (G_LIKELY (actions != NULL))
            {
              /* append the actions */
              for (lp = actions; lp != NULL; lp = lp->next)
                {
                  /* append the menu item */
                  item = gtk_action_create_menu_item (GTK_ACTION (lp->data));
                  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
                  gtk_widget_show (item);

                  /* release the reference on the action */
                  g_object_unref (G_OBJECT (lp->data));
                }
              g_list_free (actions);

              /* append a menu separator */
              item = gtk_separator_menu_item_new ();
              gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
              gtk_widget_show (item);
            }
        }

      /* append the remove menu item */
      item = gtk_image_menu_item_new_with_mnemonic (_("_Remove Shortcut"));
      g_object_set_data_full (G_OBJECT (item), I_("thunar-shortcuts-row"),
                              gtk_tree_row_reference_new (model, path),
                              (GDestroyNotify) gtk_tree_row_reference_free);
      g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (thunar_shortcuts_view_remove_activated), widget);
      gtk_widget_set_sensitive (item, mutable);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
      gtk_widget_show (item);

      /* set the remove stock icon */
      image = gtk_image_new_from_stock (GTK_STOCK_REMOVE, GTK_ICON_SIZE_MENU);
      gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
      gtk_widget_show (image);

      /* append the rename menu item */
      item = gtk_image_menu_item_new_with_mnemonic (_("Re_name Shortcut"));
      g_object_set_data_full (G_OBJECT (item), I_("thunar-shortcuts-row"),
                              gtk_tree_row_reference_new (model, path),
                              (GDestroyNotify) gtk_tree_row_reference_free);
      g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (thunar_shortcuts_view_rename_activated), widget);
      gtk_widget_set_sensitive (item, mutable);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
      gtk_widget_show (item);

      /* run the internal loop */
      gtk_grab_add (menu);
      gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 0, event->time);
      g_main_loop_run (loop);
      gtk_grab_remove (menu);

      /* clean up */
      g_object_unref (G_OBJECT (menu));
      gtk_tree_path_free (path);
      g_main_loop_unref (loop);

      /* we effectively handled the event */
      return TRUE;
    }

  return result;
}



static void
thunar_shortcuts_view_drag_data_received (GtkWidget        *widget,
                                          GdkDragContext   *context,
                                          gint              x,
                                          gint              y,
                                          GtkSelectionData *selection_data,
                                          guint             info,
                                          guint             time)
{
  ThunarShortcutsView *view = THUNAR_SHORTCUTS_VIEW (widget);
  GtkTreeSelection     *selection;
  GtkTreeModel         *model;
  GtkTreePath          *dst_path;
  GtkTreePath          *src_path;
  GtkTreeIter           iter;

  g_return_if_fail (THUNAR_IS_SHORTCUTS_VIEW (view));

  /* compute the drop position */
  dst_path = thunar_shortcuts_view_compute_drop_position (view, x, y);

  if (selection_data->target == gdk_atom_intern ("text/uri-list", FALSE))
    {
      thunar_shortcuts_view_drop_uri_list (view, (gchar *) selection_data->data, dst_path);
    }
  else if (selection_data->target == gdk_atom_intern ("GTK_TREE_MODEL_ROW", FALSE))
    {
      selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
      if (gtk_tree_selection_get_selected (selection, &model, &iter))
        {
          /* we need to adjust the destination path here, because the path returned by
           * the drop position computation effectively points after the insert position,
           * which can led to unexpected results.
           */
          gtk_tree_path_prev (dst_path);
          if (!thunar_shortcuts_model_drop_possible (THUNAR_SHORTCUTS_MODEL (model), dst_path))
            gtk_tree_path_next (dst_path);

          /* perform the move */
          src_path = gtk_tree_model_get_path (model, &iter);
          thunar_shortcuts_model_move (THUNAR_SHORTCUTS_MODEL (model), src_path, dst_path);
          gtk_tree_path_free (src_path);
        }
    }

  gtk_tree_path_free (dst_path);
}



static gboolean
thunar_shortcuts_view_drag_drop (GtkWidget      *widget,
                                 GdkDragContext *context,
                                 gint            x,
                                 gint            y,
                                 guint           time)
{
  g_return_val_if_fail (THUNAR_IS_SHORTCUTS_VIEW (widget), FALSE);
  return TRUE;
}



static gboolean
thunar_shortcuts_view_drag_motion (GtkWidget      *widget,
                                   GdkDragContext *context,
                                   gint            x,
                                   gint            y,
                                   guint           time)
{
  GtkTreeViewDropPosition position = GTK_TREE_VIEW_DROP_BEFORE;
  ThunarShortcutsView   *view = THUNAR_SHORTCUTS_VIEW (widget);
  GdkDragAction           action;
  GtkTreeModel           *model;
  GtkTreePath            *path;

  /* check the action that should be performed */
  if (context->suggested_action == GDK_ACTION_LINK || (context->actions & GDK_ACTION_LINK) != 0)
    action = GDK_ACTION_LINK;
  else if (context->suggested_action == GDK_ACTION_COPY || (context->actions & GDK_ACTION_COPY) != 0)
    action = GDK_ACTION_COPY;
  else if (context->suggested_action == GDK_ACTION_MOVE || (context->actions & GDK_ACTION_MOVE) != 0)
    action = GDK_ACTION_MOVE;
  else
    return FALSE;

  /* compute the drop position for the coordinates */
  path = thunar_shortcuts_view_compute_drop_position (view, x, y);

  /* check if path is about to append to the model */
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (view));
  if (gtk_tree_path_get_indices (path)[0] >= gtk_tree_model_iter_n_children (model, NULL))
    {
      /* set the position to "after" and move the path to
       * point to the previous row instead; required to
       * get the highlighting in GtkTreeView correct.
       */
      position = GTK_TREE_VIEW_DROP_AFTER;
      gtk_tree_path_prev (path);
    }

  /* highlight the appropriate row */
  gtk_tree_view_set_drag_dest_row (GTK_TREE_VIEW (view), path, position);
  gtk_tree_path_free (path);

  gdk_drag_status (context, action, time);
  return TRUE;
}



static void
thunar_shortcuts_view_row_activated (GtkTreeView       *tree_view,
                                     GtkTreePath       *path,
                                     GtkTreeViewColumn *column)
{
  ThunarShortcutsView *view = THUNAR_SHORTCUTS_VIEW (tree_view);
  GtkTreeModel         *model;
  GtkTreeIter           iter;
  ThunarFile           *file;

  g_return_if_fail (THUNAR_IS_SHORTCUTS_VIEW (view));
  g_return_if_fail (path != NULL);

  /* determine the iter for the path */
  model = gtk_tree_view_get_model (tree_view);
  gtk_tree_model_get_iter (model, &iter, path);

  /* determine the file for the shortcut and invoke the signal */
  file = thunar_shortcuts_model_file_for_iter (THUNAR_SHORTCUTS_MODEL (model), &iter);
  if (G_LIKELY (file != NULL))
    g_signal_emit (G_OBJECT (view), view_signals[SHORTCUT_ACTIVATED], 0, file);

  /* call the row-activated method in the parent class */
  if (GTK_TREE_VIEW_CLASS (thunar_shortcuts_view_parent_class)->row_activated != NULL)
    (*GTK_TREE_VIEW_CLASS (thunar_shortcuts_view_parent_class)->row_activated) (tree_view, path, column);
}



static void
thunar_shortcuts_view_remove_activated (GtkWidget           *item,
                                        ThunarShortcutsView *view)
{
  GtkTreeRowReference *row;
  GtkTreeModel        *model;
  GtkTreePath         *path;

  row = g_object_get_data (G_OBJECT (item), I_("thunar-shortcuts-row"));
  path = gtk_tree_row_reference_get_path (row);
  if (G_LIKELY (path != NULL))
    {
      model = gtk_tree_view_get_model (GTK_TREE_VIEW (view));
      thunar_shortcuts_model_remove (THUNAR_SHORTCUTS_MODEL (model), path);
      gtk_tree_path_free (path);
    }
}



static void
thunar_shortcuts_view_rename_activated (GtkWidget           *item,
                                        ThunarShortcutsView *view)
{
  GtkTreeRowReference *row;
  GtkTreeViewColumn   *column;
  GtkCellRenderer     *renderer;
  GtkTreePath         *path;
  GList               *renderers;

  row = g_object_get_data (G_OBJECT (item), I_("thunar-shortcuts-row"));
  path = gtk_tree_row_reference_get_path (row);
  if (G_LIKELY (path != NULL))
    {
      column = gtk_tree_view_get_column (GTK_TREE_VIEW (view), 0);
      renderers = gtk_tree_view_column_get_cell_renderers (column);
      renderer = g_list_nth_data (renderers, 1);
      g_object_set (G_OBJECT (renderer), "editable", TRUE, NULL);
      gtk_tree_view_set_cursor_on_cell (GTK_TREE_VIEW (view), path, column, renderer, TRUE);
      gtk_tree_path_free (path);
      g_list_free (renderers);
    }
}



static void
thunar_shortcuts_view_rename_canceled (GtkCellRenderer     *renderer,
                                       ThunarShortcutsView *view)
{
  /* disable the editing support on the name cell again */
  g_object_set (G_OBJECT (renderer), "editable", FALSE, NULL);
}



static void
thunar_shortcuts_view_renamed (GtkCellRenderer     *renderer,
                               const gchar         *path_string,
                               const gchar         *text,
                               ThunarShortcutsView *view)
{
  GtkTreeModel *model;
  GtkTreePath  *path;

  /* disable the editing support on the name cell again */
  g_object_set (G_OBJECT (renderer), "editable", FALSE, NULL);

  /* perform the rename */
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (view));
  path = gtk_tree_path_new_from_string (path_string);
  thunar_shortcuts_model_rename (THUNAR_SHORTCUTS_MODEL (model), path, text);
  gtk_tree_path_free (path);
}



static GtkTreePath*
thunar_shortcuts_view_compute_drop_position (ThunarShortcutsView *view,
                                             gint                 x,
                                             gint                 y)
{
  GtkTreeViewColumn *column;
  GtkTreeModel      *model;
  GdkRectangle       area;
  GtkTreePath       *path;
  gint               n_rows;

  g_return_val_if_fail (gtk_tree_view_get_model (GTK_TREE_VIEW (view)) != NULL, NULL);
  g_return_val_if_fail (THUNAR_IS_SHORTCUTS_VIEW (view), NULL);

  /* query the number of rows in the model */
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (view));
  n_rows = gtk_tree_model_iter_n_children (model, NULL);

  if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (view), x, y,
                                     &path, &column, &x, &y))
    {
      /* determine the exact path of the row the user is trying to drop 
       * (taking into account the relative y position)
       */
      gtk_tree_view_get_background_area (GTK_TREE_VIEW (view), path, column, &area);
      if (y >= area.height / 2)
        gtk_tree_path_next (path);

      /* find a suitable drop path (we cannot drop into the default shortcuts list) */
      for (; gtk_tree_path_get_indices (path)[0] < n_rows; gtk_tree_path_next (path))
        if (thunar_shortcuts_model_drop_possible (THUNAR_SHORTCUTS_MODEL (model), path))
          return path;
    }
  else
    {
      /* we'll append to the shortcuts list */
      path = gtk_tree_path_new_from_indices (n_rows, -1);
    }

  return path;
}



static void
thunar_shortcuts_view_drop_uri_list (ThunarShortcutsView *view,
                                     const gchar         *uri_list,
                                     GtkTreePath         *dst_path)
{
  GtkTreeModel *model;
  ThunarFile   *file;
  GError       *error = NULL;
  gchar        *display_string;
  gchar        *uri_string;
  GList        *path_list;
  GList        *lp;

  path_list = thunar_vfs_path_list_from_string (uri_list, &error);
  if (G_LIKELY (error == NULL))
    {
      /* process the URIs one-by-one and stop on error */
      model = gtk_tree_view_get_model (GTK_TREE_VIEW (view));
      for (lp = path_list; lp != NULL; lp = lp->next)
        {
          file = thunar_file_get_for_path (lp->data, &error);
          if (G_UNLIKELY (file == NULL))
            break;

          /* make sure, that only directories gets added to the shortcuts list */
          if (G_UNLIKELY (!thunar_file_is_directory (file)))
            {
              uri_string = thunar_vfs_path_dup_string (lp->data);
              display_string = g_filename_display_name (uri_string);
              g_set_error (&error, G_FILE_ERROR, G_FILE_ERROR_NOTDIR,
                           _("The path '%s' does not refer to a directory"),
                           display_string);
              g_object_unref (G_OBJECT (file));
              g_free (display_string);
              g_free (uri_string);
              break;
            }

          thunar_shortcuts_model_add (THUNAR_SHORTCUTS_MODEL (model), dst_path, file);
          g_object_unref (G_OBJECT (file));
          gtk_tree_path_next (dst_path);
        }

      thunar_vfs_path_list_free (path_list);
    }

  if (G_UNLIKELY (error != NULL))
    {
      /* display an error message to the user */
      thunar_dialogs_show_error (GTK_WIDGET (view), error, _("Failed to add new shortcut"));

      /* release the error */
      g_error_free (error);
    }
}



#if GTK_CHECK_VERSION(2,6,0)
static gboolean
thunar_shortcuts_view_separator_func (GtkTreeModel *model,
                                      GtkTreeIter  *iter,
                                      gpointer      user_data)
{
  gboolean separator;
  gtk_tree_model_get (model, iter, THUNAR_SHORTCUTS_MODEL_COLUMN_SEPARATOR, &separator, -1);
  return separator;
}
#endif



/**
 * thunar_shortcuts_view_new:
 *
 * Allocates a new #ThunarShortcutsView instance and associates
 * it with the default #ThunarShortcutsModel instance.
 *
 * Return value: the newly allocated #ThunarShortcutsView instance.
 **/
GtkWidget*
thunar_shortcuts_view_new (void)
{
  ThunarShortcutsModel *model;
  GtkWidget             *view;

  model = thunar_shortcuts_model_get_default ();
  view = g_object_new (THUNAR_TYPE_SHORTCUTS_VIEW, "model", model, NULL);
  g_object_unref (G_OBJECT (model));

  return view;
}



/**
 * thunar_shortcuts_view_select_by_file:
 * @view : a #ThunarShortcutsView instance.
 * @file : a #ThunarFile instance.
 *
 * Looks up the first shortcut that refers to @file in @view and selects it.
 * If @file is not present in the underlying #ThunarShortcutsModel, no
 * shortcut will be selected afterwards.
 **/
void
thunar_shortcuts_view_select_by_file (ThunarShortcutsView *view,
                                      ThunarFile          *file)
{
  GtkTreeSelection *selection;
  GtkTreeModel     *model;
  GtkTreeIter       iter;

  g_return_if_fail (THUNAR_IS_SHORTCUTS_VIEW (view));
  g_return_if_fail (THUNAR_IS_FILE (file));

  /* clear the selection */
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
  gtk_tree_selection_unselect_all (selection);

  /* try to lookup a tree iter for the given file */
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (view));
  if (thunar_shortcuts_model_iter_for_file (THUNAR_SHORTCUTS_MODEL (model), file, &iter))
    gtk_tree_selection_select_iter (selection, &iter);
}

