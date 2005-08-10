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

#include <thunar/thunar-progress-dialog.h>



enum
{
  PROP_0,
  PROP_JOB,
};



static void                             thunar_progress_dialog_class_init   (ThunarProgressDialogClass      *klass);
static void                             thunar_progress_dialog_init         (ThunarProgressDialog           *dialog);
static void                             thunar_progress_dialog_dispose      (GObject                        *object);
static void                             thunar_progress_dialog_get_property (GObject                        *object,
                                                                             guint                           prop_id,
                                                                             GValue                         *value,
                                                                             GParamSpec                     *pspec);
static void                             thunar_progress_dialog_set_property (GObject                        *object,
                                                                             guint                           prop_id,
                                                                             const GValue                   *value,
                                                                             GParamSpec                     *pspec);
static void                             thunar_progress_dialog_response     (GtkDialog                      *dialog,
                                                                             gint                            response);
static ThunarVfsInteractiveJobResponse  thunar_progress_dialog_ask          (ThunarProgressDialog           *dialog,
                                                                             const gchar                    *message,
                                                                             ThunarVfsInteractiveJobResponse choices,
                                                                             ThunarVfsJob                   *job);
static void                             thunar_progress_dialog_error        (ThunarProgressDialog           *dialog,
                                                                             GError                         *error,
                                                                             ThunarVfsJob                   *job);
static void                             thunar_progress_dialog_finished     (ThunarProgressDialog           *dialog,
                                                                             ThunarVfsJob                   *job);
static void                             thunar_progress_dialog_info_message (ThunarProgressDialog           *dialog,
                                                                             const gchar                    *message,
                                                                             ThunarVfsJob                   *job);
static void                             thunar_progress_dialog_percent      (ThunarProgressDialog           *dialog,
                                                                             gdouble                         percent,
                                                                             ThunarVfsJob                   *job);



struct _ThunarProgressDialogClass
{
  GtkDialogClass __parent__;
};

struct _ThunarProgressDialog
{
  GtkDialog __parent__;

  ThunarVfsJob *job;

  GtkWidget    *progress_bar;
  GtkWidget    *progress_label;
};



G_DEFINE_TYPE (ThunarProgressDialog, thunar_progress_dialog, GTK_TYPE_DIALOG);



static gboolean
transform_to_markup (const GValue *src_value,
                     GValue       *dst_value,
                     gpointer      user_data)
{
  g_value_take_string (dst_value, g_strdup_printf ("<big>%s</big>", g_value_get_string (src_value)));
  return TRUE;
}



static void
thunar_progress_dialog_class_init (ThunarProgressDialogClass *klass)
{
  GtkDialogClass *gtkdialog_class;
  GObjectClass   *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = thunar_progress_dialog_dispose;
  gobject_class->get_property = thunar_progress_dialog_get_property;
  gobject_class->set_property = thunar_progress_dialog_set_property;

  gtkdialog_class = GTK_DIALOG_CLASS (klass);
  gtkdialog_class->response = thunar_progress_dialog_response;

  /**
   * ThunarProgressDialog:job:
   *
   * The #ThunarVfsInteractiveJob, whose progress is displayed by
   * this dialog, or %NULL if no job is set.
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_JOB,
                                   exo_param_spec_object ("job",
                                                          _("Job"),
                                                          _("The job whose progress to display"),
                                                          THUNAR_VFS_TYPE_INTERACTIVE_JOB,
                                                          EXO_PARAM_READWRITE));
}



static void
thunar_progress_dialog_init (ThunarProgressDialog *dialog)
{
  GtkWidget *table;
  GtkWidget *image;
  GtkWidget *label;

  gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
  gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);

  gtk_window_set_default_size (GTK_WINDOW (dialog), 300, -1);

  table = g_object_new (GTK_TYPE_TABLE,
                        "border-width", 6,
                        "n-columns", 2,
                        "n-rows", 3,
                        "row-spacing", 6,
                        "column-spacing", 5,
                        NULL);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), table, TRUE, TRUE, 0);
  gtk_widget_show (table);

  image = g_object_new (GTK_TYPE_IMAGE, "icon-size", GTK_ICON_SIZE_BUTTON, NULL);
  gtk_table_attach (GTK_TABLE (table), image, 0, 1, 0, 1,
                    GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 6);
  gtk_widget_show (image);

  label = g_object_new (GTK_TYPE_LABEL, "use-markup", TRUE, "xalign", 0.0f, NULL);
  gtk_table_attach (GTK_TABLE (table), label, 1, 2, 0, 1,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 6);
  gtk_widget_show (label);

  dialog->progress_label = g_object_new (EXO_TYPE_ELLIPSIZED_LABEL, "ellipsize", EXO_PANGO_ELLIPSIZE_START, "xalign", 0.0f, NULL);
  gtk_table_attach (GTK_TABLE (table), dialog->progress_label, 0, 2, 1, 2,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (dialog->progress_label);

  dialog->progress_bar = g_object_new (GTK_TYPE_PROGRESS_BAR, "text", "0.00 %", NULL);
  gtk_table_attach (GTK_TABLE (table), dialog->progress_bar, 0, 2, 2, 3,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (dialog->progress_bar);

  /* connect the window icon name to the action image */
  exo_binding_new (G_OBJECT (dialog), "icon-name",
                   G_OBJECT (image), "icon-name");

  /* connect the window title to the action label */
  exo_binding_new_full (G_OBJECT (dialog), "title",
                        G_OBJECT (label), "label",
                        transform_to_markup, NULL, NULL);
}



static void
thunar_progress_dialog_dispose (GObject *object)
{
  ThunarProgressDialog *dialog = THUNAR_PROGRESS_DIALOG (object);

  /* disconnect from the job (if any) */
  thunar_progress_dialog_set_job (dialog, NULL);

  (*G_OBJECT_CLASS (thunar_progress_dialog_parent_class)->dispose) (object);
}



static void
thunar_progress_dialog_get_property (GObject    *object,
                                     guint       prop_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  ThunarProgressDialog *dialog = THUNAR_PROGRESS_DIALOG (object);

  switch (prop_id)
    {
    case PROP_JOB:
      exo_value_set_object (value, thunar_progress_dialog_get_job (dialog));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
thunar_progress_dialog_set_property (GObject      *object,
                                     guint         prop_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  ThunarProgressDialog *dialog = THUNAR_PROGRESS_DIALOG (object);

  switch (prop_id)
    {
    case PROP_JOB:
      thunar_progress_dialog_set_job (dialog, exo_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static ThunarVfsInteractiveJobResponse
thunar_progress_dialog_ask (ThunarProgressDialog           *dialog,
                            const gchar                    *message,
                            ThunarVfsInteractiveJobResponse choices,
                            ThunarVfsJob                   *job)
{
  const gchar *mnemonic;
  GtkWidget   *question;
  GtkWidget   *hbox;
  GtkWidget   *image;
  GtkWidget   *label;
  GtkWidget   *button;
  gint         response;
  gint         n;

  g_return_val_if_fail (THUNAR_IS_PROGRESS_DIALOG (dialog), THUNAR_VFS_INTERACTIVE_JOB_RESPONSE_CANCEL);
  g_return_val_if_fail (g_utf8_validate (message, -1, NULL), THUNAR_VFS_INTERACTIVE_JOB_RESPONSE_CANCEL);
  g_return_val_if_fail (THUNAR_VFS_IS_INTERACTIVE_JOB (job), THUNAR_VFS_INTERACTIVE_JOB_RESPONSE_CANCEL);
  g_return_val_if_fail (dialog->job == job, THUNAR_VFS_INTERACTIVE_JOB_RESPONSE_CANCEL);

  question = g_object_new (GTK_TYPE_DIALOG,
                           "has-separator", FALSE,
                           "resizable", FALSE,
                           "title", _("Question"),
                           NULL);
  gtk_window_set_transient_for (GTK_WINDOW (question), GTK_WINDOW (dialog));

  hbox = g_object_new (GTK_TYPE_HBOX, "border-width", 12, "spacing", 12, NULL);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (question)->vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_QUESTION, GTK_ICON_SIZE_DIALOG);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  gtk_widget_show (image);

  label = g_object_new (GTK_TYPE_LABEL, "label", message, "xalign", 0, "yalign", 0, NULL);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
  gtk_widget_show (label);

  /* add the buttons based on the possible choices */
  for (n = 3; n >= 0; --n)
    {
      response = choices & (1 << n);
      if (response == 0)
        continue;

      switch (response)
        {
        case THUNAR_VFS_INTERACTIVE_JOB_RESPONSE_YES:
          mnemonic = _("_Yes");
          break;

        case THUNAR_VFS_INTERACTIVE_JOB_RESPONSE_YES_ALL:
          mnemonic = _("Yes to _all");
          break;

        case THUNAR_VFS_INTERACTIVE_JOB_RESPONSE_NO:
          mnemonic = _("_No");
          break;

        case THUNAR_VFS_INTERACTIVE_JOB_RESPONSE_CANCEL:
          mnemonic = _("_Cancel");
          break;

        default:
          g_assert_not_reached ();
          break;
        }

      button = gtk_button_new_with_mnemonic (mnemonic);
      GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
      gtk_dialog_add_action_widget (GTK_DIALOG (question), button, response);
      gtk_widget_show (button);

      gtk_dialog_set_default_response (GTK_DIALOG (question), response);
    }

  /* run the question */
  response = gtk_dialog_run (GTK_DIALOG (question));
  gtk_widget_destroy (question);

  /* transform the result as required */
  if (G_UNLIKELY (response <= 0))
    response = THUNAR_VFS_INTERACTIVE_JOB_RESPONSE_CANCEL;

  return response;
}



static void
thunar_progress_dialog_error (ThunarProgressDialog *dialog,
                              GError               *error,
                              ThunarVfsJob         *job)
{
  GtkWidget *message;

  g_return_if_fail (THUNAR_IS_PROGRESS_DIALOG (dialog));
  g_return_if_fail (error != NULL && error->message != NULL);
  g_return_if_fail (THUNAR_VFS_IS_INTERACTIVE_JOB (job));
  g_return_if_fail (dialog->job == job);

  message = gtk_message_dialog_new (GTK_WINDOW (dialog),
                                    GTK_DIALOG_MODAL |
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_ERROR,
                                    GTK_BUTTONS_OK,
                                    "%s", error->message);
  gtk_dialog_run (GTK_DIALOG (message));
  gtk_widget_destroy (message);
}



static void
thunar_progress_dialog_finished (ThunarProgressDialog *dialog,
                                 ThunarVfsJob         *job)
{
  g_return_if_fail (THUNAR_IS_PROGRESS_DIALOG (dialog));
  g_return_if_fail (THUNAR_VFS_IS_INTERACTIVE_JOB (job));
  g_return_if_fail (dialog->job == job);

  gtk_dialog_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);
}



static void
thunar_progress_dialog_info_message (ThunarProgressDialog *dialog,
                                     const gchar          *message,
                                     ThunarVfsJob         *job)
{
  g_return_if_fail (THUNAR_IS_PROGRESS_DIALOG (dialog));
  g_return_if_fail (g_utf8_validate (message, -1, NULL));
  g_return_if_fail (THUNAR_VFS_IS_INTERACTIVE_JOB (job));
  g_return_if_fail (dialog->job == job);

  gtk_label_set_text (GTK_LABEL (dialog->progress_label), message);
}



static void
thunar_progress_dialog_percent (ThunarProgressDialog *dialog,
                                gdouble               percent,
                                ThunarVfsJob         *job)
{
  gchar text[32];

  g_return_if_fail (THUNAR_IS_PROGRESS_DIALOG (dialog));
  g_return_if_fail (percent >= 0.0 && percent <= 100.0);
  g_return_if_fail (THUNAR_VFS_IS_INTERACTIVE_JOB (job));
  g_return_if_fail (dialog->job == job);

  g_snprintf (text, sizeof (text), "%.2f %%", percent);
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (dialog->progress_bar), text);
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (dialog->progress_bar), percent / 100.0);
}



static void
thunar_progress_dialog_response (GtkDialog *dialog,
                                 gint       response)
{
  /* cancel the job appropriately */
  switch (response)
    {
    case GTK_RESPONSE_NONE:
    case GTK_RESPONSE_REJECT:
    case GTK_RESPONSE_DELETE_EVENT:
    case GTK_RESPONSE_CANCEL:
    case GTK_RESPONSE_CLOSE:
    case GTK_RESPONSE_NO:
      if (G_LIKELY (THUNAR_PROGRESS_DIALOG (dialog)->job != NULL))
        thunar_vfs_job_cancel (THUNAR_PROGRESS_DIALOG (dialog)->job);
      break;
    }

  if (GTK_DIALOG_CLASS (thunar_progress_dialog_parent_class)->response != NULL)
    (*GTK_DIALOG_CLASS (thunar_progress_dialog_parent_class)->response) (dialog, response);
}



/**
 * thunar_progress_dialog_new:
 *
 * Allocates a new #ThunarProgressDialog.
 *
 * Return value: the newly allocated #ThunarProgressDialog.
 **/
GtkWidget*
thunar_progress_dialog_new (void)
{
  return thunar_progress_dialog_new_with_job (NULL);
}



/**
 * thunar_progress_dialog_new_with_job:
 * @job : a #ThunarVfsInteractiveJob or %NULL.
 *
 * Allocates a new #ThunarProgressDialog and associates it with
 * the @job.
 *
 * Return value: the newly allocated #ThunarProgressDialog.
 **/
GtkWidget*
thunar_progress_dialog_new_with_job (ThunarVfsJob *job)
{
  g_return_val_if_fail (job == NULL || THUNAR_VFS_IS_INTERACTIVE_JOB (job), NULL);
  return g_object_new (THUNAR_TYPE_PROGRESS_DIALOG, "job", job, NULL);
}



/**
 * thunar_progress_dialog_get_job:
 * @dialog : a #ThunarProgressDialog.
 *
 * Returns the #ThunarVfsInteractiveJob associated with @dialog
 * or %NULL if no job is currently associated with @dialog.
 *
 * Return value: the job associated with @dialog or %NULL.
 **/
ThunarVfsJob*
thunar_progress_dialog_get_job (ThunarProgressDialog *dialog)
{
  g_return_val_if_fail (THUNAR_IS_PROGRESS_DIALOG (dialog), NULL);
  return dialog->job;
}



/**
 * thunar_progress_dialog_set_job:
 * @dialog : a #ThunarProgressDialog.
 * @job    : a #ThunarVfsInteractiveJob or %NULL.
 *
 * Associates @job with @dialog.
 **/
void
thunar_progress_dialog_set_job (ThunarProgressDialog *dialog,
                                ThunarVfsJob         *job)
{
  g_return_if_fail (THUNAR_IS_PROGRESS_DIALOG (dialog));
  g_return_if_fail (job == NULL || THUNAR_VFS_IS_INTERACTIVE_JOB (job));

  /* check if we're already on that job */
  if (G_UNLIKELY (dialog->job == job))
    return;

  /* disconnect from the previous job */
  if (G_LIKELY (dialog->job != NULL))
    {
      g_signal_handlers_disconnect_matched (dialog->job, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, dialog);
      thunar_vfs_job_unref (dialog->job);
    }

  /* activate the new job */
  dialog->job = job;

  /* connect to the new job */
  if (G_LIKELY (job != NULL))
    {
      thunar_vfs_job_ref (job);

      g_signal_connect_swapped (job, "ask", G_CALLBACK (thunar_progress_dialog_ask), dialog);
      g_signal_connect_swapped (job, "error", G_CALLBACK (thunar_progress_dialog_error), dialog);
      g_signal_connect_swapped (job, "finished", G_CALLBACK (thunar_progress_dialog_finished), dialog);
      g_signal_connect_swapped (job, "info-message", G_CALLBACK (thunar_progress_dialog_info_message), dialog);
      g_signal_connect_swapped (job, "percent", G_CALLBACK (thunar_progress_dialog_percent), dialog);
    }

  g_object_notify (G_OBJECT (dialog), "job");
}
