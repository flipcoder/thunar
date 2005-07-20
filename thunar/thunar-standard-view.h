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

#ifndef __THUNAR_STANDARD_VIEW_H__
#define __THUNAR_STANDARD_VIEW_H__

#include <thunar/thunar-clipboard-manager.h>
#include <thunar/thunar-list-model.h>
#include <thunar/thunar-view.h>

G_BEGIN_DECLS;

typedef struct _ThunarStandardViewClass ThunarStandardViewClass;
typedef struct _ThunarStandardView      ThunarStandardView;

#define THUNAR_TYPE_STANDARD_VIEW             (thunar_standard_view_get_type ())
#define THUNAR_STANDARD_VIEW(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), THUNAR_TYPE_STANDARD_VIEW, ThunarStandardView))
#define THUNAR_STANDARD_VIEW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), THUNAR_TYPE_STANDARD_VIEW, ThunarStandardViewClass))
#define THUNAR_IS_STANDARD_VIEW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), THUNAR_TYPE_STANDARD_VIEW))
#define THUNAR_IS_STANDARD_VIEW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), THUNAR_TYPE_STANDARD_VIEW))
#define THUNAR_STANDARD_VIEW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), THUNAR_TYPE_STANDARD_VIEW, ThunarStandardViewClass))

struct _ThunarStandardViewClass
{
  GtkScrolledWindowClass __parent__;

  /* Returns the list of currently selected GtkTreePath's, where
   * both the list and the items are owned by the caller. */
  GList *(*get_selected_items) (ThunarStandardView *standard_view);
};

struct _ThunarStandardView
{
  GtkScrolledWindow __parent__;

  ThunarClipboardManager *clipboard;
  ThunarListModel        *model;
  gchar                  *statusbar_text;

  GtkActionGroup         *action_group;
  GtkUIManager           *ui_manager;
  guint                   ui_merge_id;

  ExoBinding             *loading_binding;
  gboolean                loading;
};

GType thunar_standard_view_get_type           (void) G_GNUC_CONST;

void  thunar_standard_view_context_menu       (ThunarStandardView *standard_view,
                                               guint               button,
                                               guint32             time);
void  thunar_standard_view_selection_changed  (ThunarStandardView *standard_view);

G_END_DECLS;

#endif /* !__THUNAR_STANDARD_VIEW_H__ */