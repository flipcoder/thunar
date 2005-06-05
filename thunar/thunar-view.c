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

#include <thunar/thunar-view.h>



static void thunar_view_class_init (gpointer klass);



GType
thunar_view_get_type (void)
{
  static GType type = G_TYPE_INVALID;

  if (G_UNLIKELY (type == G_TYPE_INVALID))
    {
      static const GTypeInfo info =
      {
        sizeof (ThunarViewIface),
        NULL,
        NULL,
        (GClassInitFunc) thunar_view_class_init,
        NULL,
        NULL,
        0,
        0,
        NULL,
      };

      type = g_type_register_static (G_TYPE_INTERFACE,
                                     "ThunarView",
                                     &info, 0);

      g_type_interface_add_prerequisite (type, GTK_TYPE_WIDGET);
    }

  return type;
}



static void
thunar_view_class_init (gpointer klass)
{
  /**
   * ThunarView:list-model:
   *
   * The #ThunarListModel currently displayed by this #ThunarView. If
   * the property is %NULL, nothing should be displayed.
   **/
  g_object_interface_install_property (klass,
                                       g_param_spec_object ("list-model",
                                                            _("List model"),
                                                            _("The model currently displayed by this view"),
                                                            THUNAR_TYPE_LIST_MODEL,
                                                            G_PARAM_READWRITE));

  /**
   * ThunarView:statusbar-text:
   *
   * The text to be displayed in the status bar, which is associated
   * with this #ThunarView instance. Implementations should invoke
   * #g_object_notify() on this property, whenever they have a new
   * text to be display in the status bar (e.g. the selection changed
   * or similar).
   **/
  g_object_interface_install_property (klass,
                                       g_param_spec_string ("statusbar-text",
                                                            _("Statusbar text"),
                                                            _("Text to be displayed in the statusbar associated with this view"),
                                                            NULL,
                                                            G_PARAM_READABLE));
}



/**
 * thunar_view_get_list_model:
 * @view : a #ThunarView instance.
 *
 * Returns the #ThunarListModel currently displayed by the @view or %NULL
 * if @view does not display any model currently.
 *
 * Return value: the currently displayed model of @view or %NULL.
 **/
ThunarListModel*
thunar_view_get_list_model (ThunarView *view)
{
  g_return_val_if_fail (THUNAR_IS_VIEW (view), NULL);
  return THUNAR_VIEW_GET_IFACE (view)->get_list_model (view);
}



/**
 * thunar_view_set_list_model:
 * @view       : a #ThunarView instance.
 * @list_model : the new directory to display or %NULL.
 *
 * Instructs @view to display the folder associated with 
 * @current_directory. If @current_directory is %NULL, the @view
 * should display nothing.
 **/
void
thunar_view_set_list_model (ThunarView      *view,
                            ThunarListModel *model)
{
  g_return_if_fail (THUNAR_IS_VIEW (view));
  g_return_if_fail (model == NULL || THUNAR_IS_LIST_MODEL (model));
  THUNAR_VIEW_GET_IFACE (view)->set_list_model (view, model);
}



/**
 * thunar_view_get_statusbar_text:
 * @view : a #ThunarView instance.
 *
 * Queries the text that should be displayed in the status bar
 * associated with @view.
 *
 * Return value: the text to be displayed in the status bar
 *               asssociated with @view.
 **/
const gchar*
thunar_view_get_statusbar_text (ThunarView *view)
{
  g_return_val_if_fail (THUNAR_IS_VIEW (view), NULL);
  return THUNAR_VIEW_GET_IFACE (view)->get_statusbar_text (view);
}



