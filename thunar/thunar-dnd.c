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

#include <thunar/thunar-application.h>
#include <thunar/thunar-dnd.h>



static void
action_selected (GtkWidget     *item,
                 GdkDragAction *action)
{
  *action = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (item), "action"));
}



/**
 * thunar_dnd_ask:
 * @widget  : the widget on which the drop was performed.
 * @time    : the time of the drop event.
 * @actions : the list of actions supported for the drop.
 *
 * Pops up a menu that asks the user to choose one of the
 * @actions or to cancel the drop. If the user chooses a
 * valid #GdkDragAction from @actions, then this action is
 * returned. Else if the user cancels the drop, 0 will be
 * returned.
 *
 * This method can be used to implement a response to the
 * #GDK_ACTION_ASK action on drops.
 *
 * Return value: the selected #GdkDragAction or 0 to cancel.
 **/
GdkDragAction
thunar_dnd_ask (GtkWidget    *widget,
                guint         time,
                GdkDragAction actions)
{
  static const GdkDragAction action_items[] = { GDK_ACTION_COPY, GDK_ACTION_MOVE, GDK_ACTION_LINK };
  static const gchar        *action_names[] = { N_ ("_Copy here"), N_ ("_Move here"), N_ ("_Link here") };

  GdkDragAction action = 0;
  GtkWidget    *menu;
  GtkWidget    *item;
  GMainLoop    *loop;
  guint         n;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), 0);

  /* prepare the internal loop */
  loop = g_main_loop_new (NULL, FALSE);

  /* prepare the popup menu */
  menu = gtk_menu_new ();
  gtk_menu_set_screen (GTK_MENU (menu), gtk_widget_get_screen (widget));
  g_signal_connect_swapped (G_OBJECT (menu), "deactivate", G_CALLBACK (g_main_loop_quit), loop);

  /* append the various items */
  for (n = 0; n < G_N_ELEMENTS (action_items); ++n)
    if (G_LIKELY ((actions & action_items[n]) != 0))
      {
        item = gtk_image_menu_item_new_with_mnemonic (_(action_names[n]));
        g_object_set_data (G_OBJECT (item), "action", GUINT_TO_POINTER (action_items[n]));
        g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (action_selected), &action);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
        gtk_widget_show (item);
      }

  /* append the separator */
  item = gtk_separator_menu_item_new ();
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  gtk_widget_show (item);

  /* append the cancel item */
  item = gtk_image_menu_item_new_from_stock (GTK_STOCK_CANCEL, NULL);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  gtk_widget_show (item);

  /* run the internal loop */
  gtk_grab_add (menu);
  gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 3, time);
  g_main_loop_run (loop);
  gtk_grab_remove (menu);

  /* clean up */
  gtk_object_sink (GTK_OBJECT (menu));
  g_main_loop_unref (loop);

  return action;
}



/**
 * thunar_dnd_perform:
 * @widget   : the #GtkWidget on which the drop was done.
 * @file     : the #ThunarFile on which the @uri_list was dropped.
 * @uri_list : the list of #ThunarVfsURI<!---->s that was dropped.
 * @action   : the #GdkDragAction that was performed.
 *
 * Performs the drop of @uri_list on @file in @widget, as given in
 * @action and returns %TRUE if the drop was started successfully
 * (or even completed successfully), else %FALSE.
 *
 * Return value: %TRUE if the DnD operation was started
 *               successfully, else %FALSE.
 **/
gboolean
thunar_dnd_perform (GtkWidget    *widget,
                    ThunarFile   *file,
                    GList        *uri_list,
                    GdkDragAction action)
{
  ThunarApplication *application;
  GtkWidget         *message;
  GtkWidget         *window;
  gboolean           succeed = TRUE;
  GError            *error = NULL;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);
  g_return_val_if_fail (THUNAR_IS_FILE (file), FALSE);

  /* query a reference on the application object */
  application = thunar_application_get ();

  /* determine the toplevel window for the widget */
  window = gtk_widget_get_toplevel (widget);

  /* check if the file is a directory */
  if (thunar_file_is_directory (file))
    {
      /* perform the given directory operation */
      switch (action)
        {
        case GDK_ACTION_COPY:
          thunar_application_copy_uris (application, GTK_WINDOW (window), uri_list, thunar_file_get_uri (file));
          break;

        case GDK_ACTION_MOVE:
          thunar_application_move_uris (application, GTK_WINDOW (window), uri_list, thunar_file_get_uri (file));
          break;

        case GDK_ACTION_LINK:
          // FIXME
          message = gtk_message_dialog_new (GTK_WINDOW (window),
                                            GTK_DIALOG_MODAL
                                            | GTK_DIALOG_DESTROY_WITH_PARENT,
                                            GTK_MESSAGE_ERROR,
                                            GTK_BUTTONS_OK,
                                            _("Creating links is not yet supported. This will be fixed soon!"));
          gtk_dialog_run (GTK_DIALOG (message));
          gtk_widget_destroy (message);
          break;

        default:
          succeed = FALSE;
        }
    }
  else if (thunar_file_is_executable (file))
    {
      succeed = thunar_file_execute (file, gtk_widget_get_screen (widget), uri_list, &error);
      if (G_UNLIKELY (!succeed))
        {
          message = gtk_message_dialog_new (GTK_WINDOW (window),
                                            GTK_DIALOG_MODAL
                                            | GTK_DIALOG_DESTROY_WITH_PARENT,
                                            GTK_MESSAGE_ERROR,
                                            GTK_BUTTONS_OK,
                                            _("Unable to execute file \"%s\"."),
                                            thunar_file_get_display_name (file));
          gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (message), "%s.", error->message);
          gtk_dialog_run (GTK_DIALOG (message));
          gtk_widget_destroy (message);
          g_error_free (error);
        }
    }
  else
    {
      succeed = FALSE;
    }

  /* release the application reference */
  g_object_unref (G_OBJECT (application));

  return succeed;
}


