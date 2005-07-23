/* 
 * gnc-plugin-page-invoice.c -- 
 *
 * Copyright (C) 2005 David Hampton <hampton@employees.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, contact:
 *
 * Free Software Foundation           Voice:  +1-617-542-5942
 * 59 Temple Place - Suite 330        Fax:    +1-617-542-2652
 * Boston, MA  02111-1307,  USA       gnu@gnu.org
 */

#include "config.h"

#include "gnc-plugin.h"
#include "dialog-invoice.h"
#include "gnc-plugin-page-invoice.h"

#include "dialog-account.h"
#include "gnc-component-manager.h"
#include "gnc-gconf-utils.h"
#include "gnc-gobject-utils.h"
#include "gnc-gnome-utils.h"
#include "gnc-icons.h"
#include "gnucash-sheet.h"
#include "gnc-ui-util.h"
#include "gnc-window.h"

/* This static indicates the debugging module that this .o belongs to.  */
static short module = MOD_GUI;

static void gnc_plugin_page_invoice_class_init (GncPluginPageInvoiceClass *klass);
static void gnc_plugin_page_invoice_init (GncPluginPageInvoice *plugin_page);
static void gnc_plugin_page_invoice_finalize (GObject *object);

static GtkWidget *gnc_plugin_page_invoice_create_widget (GncPluginPage *plugin_page);
static void gnc_plugin_page_invoice_destroy_widget (GncPluginPage *plugin_page);
static void gnc_plugin_page_invoice_window_changed (GncPluginPage *plugin_page, GtkWidget *window);
static void gnc_plugin_page_invoice_merge_actions (GncPluginPage *plugin_page, GtkUIManager *ui_merge);
static void gnc_plugin_page_invoice_unmerge_actions (GncPluginPage *plugin_page, GtkUIManager *ui_merge);


/* Callbacks */
static gboolean gnc_plugin_page_invoice_button_press_cb (GtkWidget *widget,
							      GdkEventButton *event,
			       				      GncPluginPageInvoice *page);

void gnc_plugin_page_invoice_start_toggle_cb(GtkToggleButton *toggle, gpointer data);
void gnc_plugin_page_invoice_end_toggle_cb(GtkToggleButton *toggle, gpointer data);
void gnc_plugin_page_invoice_today_cb(GtkButton *buttontoggle, gpointer data);

/* Command callbacks */
static void gnc_plugin_page_invoice_cmd_new_invoice (GtkAction *action, GncPluginPageInvoice *plugin_page);
static void gnc_plugin_page_invoice_cmd_new_account (GtkAction *action, GncPluginPageInvoice *plugin_page);
static void gnc_plugin_page_invoice_cmd_print (GtkAction *action, GncPluginPageInvoice *plugin_page);
static void gnc_plugin_page_invoice_cmd_cut (GtkAction *action, GncPluginPageInvoice *plugin_page);
static void gnc_plugin_page_invoice_cmd_copy (GtkAction *action, GncPluginPageInvoice *plugin_page);
static void gnc_plugin_page_invoice_cmd_paste (GtkAction *action, GncPluginPageInvoice *plugin_page);
static void gnc_plugin_page_invoice_cmd_edit (GtkAction *action, GncPluginPageInvoice *plugin_page);
static void gnc_plugin_page_invoice_cmd_post (GtkAction *action, GncPluginPageInvoice *plugin_page);
static void gnc_plugin_page_invoice_cmd_unpost (GtkAction *action, GncPluginPageInvoice *plugin_page);

static void gnc_plugin_page_invoice_cmd_sort_changed (GtkAction *action,
						      GtkRadioAction *current,
						      GncPluginPageInvoice *plugin_page);

static void gnc_plugin_page_invoice_cmd_enter (GtkAction *action, GncPluginPageInvoice *plugin_page);
static void gnc_plugin_page_invoice_cmd_cancel (GtkAction *action, GncPluginPageInvoice *plugin_page);
static void gnc_plugin_page_invoice_cmd_delete (GtkAction *action, GncPluginPageInvoice *plugin_page);
static void gnc_plugin_page_invoice_cmd_blank (GtkAction *action, GncPluginPageInvoice *plugin_page);
static void gnc_plugin_page_invoice_cmd_duplicate (GtkAction *action, GncPluginPageInvoice *plugin_page);
static void gnc_plugin_page_invoice_cmd_pay_invoice (GtkAction *action, GncPluginPageInvoice *plugin_page);
static void gnc_plugin_page_invoice_cmd_company_report (GtkAction *action, GncPluginPageInvoice *plugin_page);

static void gnc_plugin_page_redraw_help_cb( GnucashRegister *gsr, GncPluginPageInvoice *invoice_page );
static void gnc_plugin_page_invoice_refresh_cb (GHashTable *changes, gpointer user_data);

/************************************************************
 *                          Actions                         *
 ************************************************************/

static GtkActionEntry gnc_plugin_page_invoice_actions [] =
{
	/* Toplevel */
	{ "FakeToplevel", NULL, "", NULL, NULL, NULL },
	{ "SortOrderAction", NULL, N_("Sort _Order"), NULL, NULL, NULL },

	/* File menu */
	{ "FileNewInvoiceAction", GTK_STOCK_NEW, N_("New _Invoice"), "",
	  N_("Create a new invoice"),
	  G_CALLBACK (gnc_plugin_page_invoice_cmd_new_invoice) },
	{ "FileNewAccountAction", GNC_STOCK_NEW_ACCOUNT, N_("New _Account..."), NULL,
	  N_("Create a new account"),
	  G_CALLBACK (gnc_plugin_page_invoice_cmd_new_account) },
	{ "FilePrintAction", GTK_STOCK_PRINT, N_("Print Invoice"), NULL,
	  N_("Make a printable invoice"),
	  G_CALLBACK (gnc_plugin_page_invoice_cmd_print) },

	/* Edit menu */
	{ "EditCutAction", GTK_STOCK_CUT, N_("_Cut"), NULL,
	  NULL,
	  G_CALLBACK (gnc_plugin_page_invoice_cmd_cut) },
	{ "EditCopyAction", GTK_STOCK_COPY, N_("Copy"), NULL,
	  NULL,
	  G_CALLBACK (gnc_plugin_page_invoice_cmd_copy) },
	{ "EditPasteAction", GTK_STOCK_PASTE, N_("_Paste"), NULL,
	  NULL,
	  G_CALLBACK (gnc_plugin_page_invoice_cmd_paste) },
	{ "EditEditInvoiceAction", GTK_STOCK_MISSING_IMAGE, N_("_Edit Invoice"), NULL,
	  N_("Edit this invoice"),
	  G_CALLBACK (gnc_plugin_page_invoice_cmd_edit) },
	{ "EditPostInvoiceAction", GTK_STOCK_JUMP_TO, N_("_Post Invoice"), NULL,
	  N_("Post this Invoice to your Chart of Accounts"),
	  G_CALLBACK (gnc_plugin_page_invoice_cmd_post) },
	{ "EditUnpostInvoiceAction", GTK_STOCK_CONVERT, N_("_Unpost Invoice"), NULL,
	  N_("Unpost this Invoice and make it editable"),
	  G_CALLBACK (gnc_plugin_page_invoice_cmd_unpost) },

	/* Actions menu */
	{ "RecordEntryAction", GTK_STOCK_ADD, N_("_Enter"), NULL,
	  N_("Record the current entry"),
	  G_CALLBACK (gnc_plugin_page_invoice_cmd_enter) },
	{ "CancelEntryAction", GTK_STOCK_CANCEL, N_("_Cancel"), NULL,
	  N_("_Cancel the current entry"),
	  G_CALLBACK (gnc_plugin_page_invoice_cmd_cancel) },
	{ "DeleteEntryAction", GTK_STOCK_DELETE, N_("_Delete"), NULL,
	  N_("Delete the current entry"),
	  G_CALLBACK (gnc_plugin_page_invoice_cmd_delete) },
	{ "BlankEntryAction", GTK_STOCK_MISSING_IMAGE, N_("_Blank"), NULL,
	  N_("Move to the blank entry at the bottom of the Invoice"),
	  G_CALLBACK (gnc_plugin_page_invoice_cmd_blank) },
	{ "DuplicateEntryAction", GTK_STOCK_COPY, N_("Dup_licate Entry"), NULL,
	  N_("Make a copy of the current entry"),
	  G_CALLBACK (gnc_plugin_page_invoice_cmd_duplicate) },

	/* Business menu */
	{ "ToolsProcessPaymentAction", NULL, N_("_Pay Invoice"), NULL,
	  N_("Enter a payment for the owner of this Invoice"),
	  G_CALLBACK (gnc_plugin_page_invoice_cmd_pay_invoice) },

	/* Reports menu */
	{ "ReportsCompanyReportAction", NULL, N_("_Company Report"), NULL,
	  N_("Open a company report window for the owner of this Invoice"),
	  G_CALLBACK (gnc_plugin_page_invoice_cmd_company_report) },
};
static guint gnc_plugin_page_invoice_n_actions = G_N_ELEMENTS (gnc_plugin_page_invoice_actions);

static GtkRadioActionEntry radio_entries [] =
{
	{ "SortStandardAction", NULL, N_("_Standard"), NULL, "Keep normal invoice order", BY_STANDARD },
	{ "SortDateAction", NULL, N_("_Date"), NULL, "Sort by date", BY_DATE },
	{ "SortDateEntryAction", NULL, N_("Date of _Entry"), NULL, "Sort by the date of entry", BY_DATE_ENTERED },
	{ "SortQuantityAction", NULL, N_("_Quantity"), NULL, "Sort by quantity", BY_QTY },
	{ "SortPriceAction", NULL, N_("_Price"), NULL, "Sort by price", BY_PRICE },
	{ "SortDescriptionAction", NULL, N_("Descri_ption"), NULL, "Sort by description", BY_DESC },
};
static guint n_radio_entries = G_N_ELEMENTS (radio_entries);

static const gchar *posted_actions[] = {
	"FilePrintAction",
	NULL
};

static const gchar *unposted_actions[] = {
	"EditCutAction",
	"EditPasteAction",
	"EditEditInvoiceAction",
	"EditPostInvoiceAction",
	"RecordEntryAction",
	"CancelEntryAction",
	"DeleteEntryAction",
	"DuplicateEntryAction",
	"BlankEntryAction",
	NULL
};

static const gchar *can_unpost_actions[] = {
	"EditUnpostInvoiceAction",
	NULL
};

/* Short labels: Used on toolbar buttons. */
static action_short_labels short_labels[] = {
  { "RecordEntryAction", 	  N_("Enter") },
  { "CancelEntryAction", 	  N_("Cancel") },
  { "DeleteEntryAction", 	  N_("Delete") },
  { "DuplicateEntryAction",       N_("Duplicate") },
  { "BlankEntryAction",           N_("Blank") },
  { "EditPostInvoiceAction",      N_("Post") },
  { "EditUnpostInvoiceAction",    N_("Unpost") },
  { NULL, NULL },
};


/************************************************************/
/*                      Data Structures                     */
/************************************************************/

struct GncPluginPageInvoicePrivate
{
	GtkActionGroup *action_group;
	guint merge_id;
	GtkUIManager *ui_merge;

	InvoiceWindow *iw;

	GtkWidget *widget;

	gint component_manager_id;
};

static GObjectClass *parent_class = NULL;

/************************************************************/
/*                      Implementation                      */
/************************************************************/

GType
gnc_plugin_page_invoice_get_type (void)
{
	static GType gnc_plugin_page_invoice_type = 0;

	if (gnc_plugin_page_invoice_type == 0) {
		static const GTypeInfo our_info = {
			sizeof (GncPluginPageInvoiceClass),
			NULL,
			NULL,
			(GClassInitFunc) gnc_plugin_page_invoice_class_init,
			NULL,
			NULL,
			sizeof (GncPluginPageInvoice),
			0,
			(GInstanceInitFunc) gnc_plugin_page_invoice_init
		};
		
		gnc_plugin_page_invoice_type = g_type_register_static (GNC_TYPE_PLUGIN_PAGE,
									"GncPluginPageInvoice",
									&our_info, 0);
	}

	return gnc_plugin_page_invoice_type;
}

GncPluginPage *
gnc_plugin_page_invoice_new (InvoiceWindow *iw)
{
	GncPluginPageInvoice *invoice_page;
	GncPluginPage *plugin_page;
	const GList *item;

	/* Is there an existing page? */
	item = gnc_gobject_tracking_get_list(GNC_PLUGIN_PAGE_INVOICE_NAME);
	for ( ; item; item = g_list_next(item)) {
	  invoice_page = (GncPluginPageInvoice *)item->data;
	  if (invoice_page->priv->iw == iw)
	    return GNC_PLUGIN_PAGE(invoice_page);
	}

	invoice_page = g_object_new (GNC_TYPE_PLUGIN_PAGE_INVOICE, NULL);
	invoice_page->priv->iw = iw;

	plugin_page = GNC_PLUGIN_PAGE(invoice_page);
	gnc_plugin_page_invoice_update_title(plugin_page);
	gnc_plugin_page_set_uri(plugin_page, "default:");

	invoice_page->priv->component_manager_id = 0;
	return plugin_page;
}

static void
gnc_plugin_page_invoice_class_init (GncPluginPageInvoiceClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GncPluginPageClass *gnc_plugin_class = GNC_PLUGIN_PAGE_CLASS(klass);

	parent_class = g_type_class_peek_parent (klass);

	object_class->finalize = gnc_plugin_page_invoice_finalize;

	gnc_plugin_class->tab_icon        = NULL;
	gnc_plugin_class->plugin_name     = GNC_PLUGIN_PAGE_INVOICE_NAME;
	gnc_plugin_class->create_widget   = gnc_plugin_page_invoice_create_widget;
	gnc_plugin_class->destroy_widget  = gnc_plugin_page_invoice_destroy_widget;
	gnc_plugin_class->window_changed  = gnc_plugin_page_invoice_window_changed;
	gnc_plugin_class->merge_actions   = gnc_plugin_page_invoice_merge_actions;
	gnc_plugin_class->unmerge_actions = gnc_plugin_page_invoice_unmerge_actions;
}

static void
gnc_plugin_page_invoice_init (GncPluginPageInvoice *plugin_page)
{
	GncPluginPageInvoicePrivate *priv;
	GncPluginPage *parent;
	GtkActionGroup *action_group;
	gboolean use_new;

	priv = g_new0 (GncPluginPageInvoicePrivate, 1);
	plugin_page->priv = priv;

	/* Init parent declared variables */
	parent = GNC_PLUGIN_PAGE(plugin_page);
	gnc_plugin_page_set_title(parent, _("Invoice"));
	gnc_plugin_page_set_tab_name(parent, _("Incoice"));
	gnc_plugin_page_set_uri(parent, "default:");

	use_new = gnc_gconf_get_bool(GCONF_SECTION_INVOICE, KEY_USE_NEW, NULL);
	gnc_plugin_page_set_use_new_window(parent, use_new);

	/* change me when the system supports multiple books */
	gnc_plugin_page_add_book(parent, gnc_get_current_book());

	/* Create menu and toolbar information */
	action_group = gtk_action_group_new ("GncPluginPageInvoiceActions");
	priv->action_group = action_group;
	gtk_action_group_add_actions (action_group, gnc_plugin_page_invoice_actions,
				      gnc_plugin_page_invoice_n_actions, plugin_page);
	gtk_action_group_add_radio_actions (action_group,
					    radio_entries, n_radio_entries,
					    REG_STYLE_LEDGER,
					    G_CALLBACK(gnc_plugin_page_invoice_cmd_sort_changed),
					    plugin_page);

	gnc_plugin_init_short_names (action_group, short_labels);
}

static void
gnc_plugin_page_invoice_finalize (GObject *object)
{
	GncPluginPageInvoice *page;

	ENTER("object %p", object);
	page = GNC_PLUGIN_PAGE_INVOICE (object);

	g_return_if_fail (GNC_IS_PLUGIN_PAGE_INVOICE (page));
	g_return_if_fail (page->priv != NULL);

	g_free (page->priv);

	G_OBJECT_CLASS (parent_class)->finalize (object);
	LEAVE(" ");
}


void
gnc_plugin_page_invoice_update_menus (GncPluginPage *page, gboolean is_posted, gboolean can_unpost)
{ 
  GncPluginPageInvoice *invoice_page;
  GtkActionGroup *action_group;

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_INVOICE(page));

  invoice_page = GNC_PLUGIN_PAGE_INVOICE(page);
  action_group = invoice_page->priv->action_group;
  gnc_plugin_update_actions (action_group, posted_actions,
			     "sensitive", is_posted);
  gnc_plugin_update_actions (action_group, unposted_actions,
			     "sensitive", !is_posted);
  gnc_plugin_update_actions (action_group, can_unpost_actions,
			     "sensitive", can_unpost);
}


/* Virtual Functions */

static GtkWidget *
gnc_plugin_page_invoice_create_widget (GncPluginPage *plugin_page)
{
	GncPluginPageInvoice *page;
	GncPluginPageInvoicePrivate *priv;
	GtkWidget *regWidget;

	ENTER("page %p", plugin_page);
	page = GNC_PLUGIN_PAGE_INVOICE (plugin_page);
	priv = page->priv;
	if (priv->widget != NULL)
		return priv->widget;

	priv->widget = gnc_invoice_create_page(priv->iw, page);
	gtk_widget_show (priv->widget);

	plugin_page->summarybar = gnc_invoice_window_create_summary_bar(priv->iw);
	gtk_widget_show(plugin_page->summarybar);

	regWidget = gnc_invoice_get_register(priv->iw);
	if (regWidget) {
	  g_signal_connect (G_OBJECT (regWidget), "redraw-help",
			    G_CALLBACK (gnc_plugin_page_redraw_help_cb), page);
	  g_signal_connect (G_OBJECT (regWidget), "button-press-event",
			    G_CALLBACK (gnc_plugin_page_invoice_button_press_cb), page);
	}

	priv->component_manager_id =
	  gnc_register_gui_component(GNC_PLUGIN_PAGE_INVOICE_NAME,
				     gnc_plugin_page_invoice_refresh_cb,
				     NULL, page);

	return priv->widget;
}

static void
gnc_plugin_page_invoice_destroy_widget (GncPluginPage *plugin_page)
{
	GncPluginPageInvoice *page;
	GncPluginPageInvoicePrivate *priv;

	ENTER("page %p", plugin_page);
	page = GNC_PLUGIN_PAGE_INVOICE (plugin_page);
	priv = page->priv;

	if (priv->widget == NULL)
		return;

	if (priv->component_manager_id) {
	  gnc_unregister_gui_component(priv->component_manager_id);
	  priv->component_manager_id = 0;
	}

	gtk_widget_hide(priv->widget);
	gnc_invoice_window_destroy_cb(priv->widget, priv->iw);
	priv->widget = NULL;
}

static void
gnc_plugin_page_invoice_window_changed (GncPluginPage *plugin_page,
					GtkWidget *window)
{
	GncPluginPageInvoice *page;
	
	g_return_if_fail (GNC_IS_PLUGIN_PAGE_INVOICE (plugin_page));

	page = GNC_PLUGIN_PAGE_INVOICE(plugin_page);
	gnc_invoice_window_changed (page->priv->iw, window);
}
	
static void
gnc_plugin_page_invoice_merge_actions (GncPluginPage *plugin_page,
					GtkUIManager *ui_merge)
{
	GncPluginPageInvoice *invoice_page;
	GncPluginPageInvoicePrivate *priv;
	
	g_return_if_fail (GNC_IS_PLUGIN_PAGE_INVOICE (plugin_page));

	invoice_page = GNC_PLUGIN_PAGE_INVOICE(plugin_page);
	priv = invoice_page->priv;

	priv->ui_merge = ui_merge;
	priv->merge_id =
	  gnc_plugin_add_actions (priv->ui_merge,
				  priv->action_group,
				  "gnc-plugin-page-invoice-ui.xml");
}
	
static void
gnc_plugin_page_invoice_unmerge_actions (GncPluginPage *plugin_page,
					      GtkUIManager *ui_merge)
{
	GncPluginPageInvoice *plugin_page_register = GNC_PLUGIN_PAGE_INVOICE(plugin_page);
	
	g_return_if_fail (GNC_IS_PLUGIN_PAGE_INVOICE (plugin_page_register));
	g_return_if_fail (plugin_page_register->priv->merge_id != 0);
	g_return_if_fail (plugin_page_register->priv->action_group != NULL);

	gtk_ui_manager_remove_ui (ui_merge, plugin_page_register->priv->merge_id);
	gtk_ui_manager_remove_action_group (ui_merge, plugin_page_register->priv->action_group);

	plugin_page_register->priv->ui_merge = NULL;
}


/* Callbacks */
static gboolean
gnc_plugin_page_invoice_button_press_cb (GtkWidget *widget,
					 GdkEventButton *event,
					 GncPluginPageInvoice *page)
{
	GtkWidget *menu;

	if (event->button == 3 && page->priv->ui_merge != NULL) {
		/* Maybe show a different popup menu if no account is selected. */
		menu = gtk_ui_manager_get_widget (page->priv->ui_merge, "/RegisterPopup");
		if (menu)
		  gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL,
				  event->button, event->time);
		return TRUE;
	}

	return FALSE;
}

/************************************************************/
/*                     Command callbacks                    */
/************************************************************/

static void
gnc_plugin_page_invoice_cmd_new_invoice (GtkAction *action,
					 GncPluginPageInvoice *plugin_page)
{
  GncPluginPageInvoicePrivate *priv;

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_INVOICE(plugin_page));

  ENTER("(action %p, plugin_page %p)", action, plugin_page);
  priv = plugin_page->priv;
  gnc_invoice_window_new_invoice_cb(NULL, priv->iw);
  LEAVE(" ");
}

static void
gnc_plugin_page_invoice_cmd_new_account (GtkAction *action,
					 GncPluginPageInvoice *plugin_page)
{
  g_return_if_fail(GNC_IS_PLUGIN_PAGE_INVOICE(plugin_page));

  ENTER("(action %p, plugin_page %p)", action, plugin_page);
  gnc_ui_new_account_window (NULL);
  LEAVE(" ");
}

static void
gnc_plugin_page_invoice_cmd_print (GtkAction *action,
				   GncPluginPageInvoice *plugin_page)
{
  GncPluginPageInvoicePrivate *priv;

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_INVOICE(plugin_page));

  ENTER("(action %p, plugin_page %p)", action, plugin_page);
  priv = plugin_page->priv;
  gnc_invoice_window_printCB(NULL, priv->iw);
  LEAVE(" ");
}

static void
gnc_plugin_page_invoice_cmd_cut (GtkAction *action,
				 GncPluginPageInvoice *plugin_page)
{
  GncPluginPageInvoicePrivate *priv;

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_INVOICE(plugin_page));

  ENTER("(action %p, plugin_page %p)", action, plugin_page);
  priv = plugin_page->priv;
  gnc_invoice_window_cut_cb(NULL, priv->iw);
  LEAVE(" ");
}

static void
gnc_plugin_page_invoice_cmd_copy (GtkAction *action,
				  GncPluginPageInvoice *plugin_page)
{
  GncPluginPageInvoicePrivate *priv;

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_INVOICE(plugin_page));

  ENTER("(action %p, plugin_page %p)", action, plugin_page);
  priv = plugin_page->priv;
  gnc_invoice_window_copy_cb(NULL, priv->iw);
  LEAVE(" ");
}

static void
gnc_plugin_page_invoice_cmd_paste (GtkAction *action,
				   GncPluginPageInvoice *plugin_page)
{
  GncPluginPageInvoicePrivate *priv;

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_INVOICE(plugin_page));

  ENTER("(action %p, plugin_page %p)", action, plugin_page);
  priv = plugin_page->priv;
  gnc_invoice_window_paste_cb(NULL, priv->iw);
  LEAVE(" ");
}

static void
gnc_plugin_page_invoice_cmd_edit (GtkAction *action,
				  GncPluginPageInvoice *plugin_page)
{
  GncPluginPageInvoicePrivate *priv;

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_INVOICE(plugin_page));

  ENTER("(action %p, plugin_page %p)", action, plugin_page);
  priv = plugin_page->priv;
  gnc_invoice_window_editCB(NULL, priv->iw);
  LEAVE(" ");
}

static void
gnc_plugin_page_invoice_cmd_post (GtkAction *action,
				  GncPluginPageInvoice *plugin_page)
{
  GncPluginPageInvoicePrivate *priv;

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_INVOICE(plugin_page));

  ENTER("(action %p, plugin_page %p)", action, plugin_page);
  priv = plugin_page->priv;
  gnc_invoice_window_postCB(NULL, priv->iw);
  LEAVE(" ");
}

static void
gnc_plugin_page_invoice_cmd_unpost (GtkAction *action,
				    GncPluginPageInvoice *plugin_page)
{
  GncPluginPageInvoicePrivate *priv;

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_INVOICE(plugin_page));

  ENTER("(action %p, plugin_page %p)", action, plugin_page);
  priv = plugin_page->priv;
  gnc_invoice_window_unpostCB(NULL, priv->iw);
  LEAVE(" ");
}

static void
gnc_plugin_page_invoice_cmd_sort_changed (GtkAction *action,
					  GtkRadioAction *current,
					  GncPluginPageInvoice *plugin_page)
{
  GncPluginPageInvoicePrivate *priv;
  invoice_sort_type_t value;

  ENTER("(action %p, radio action %p, plugin_page %p)",
	action, current, plugin_page);

  g_return_if_fail(GTK_IS_ACTION(action));
  g_return_if_fail(GTK_IS_RADIO_ACTION(current));
  g_return_if_fail(GNC_IS_PLUGIN_PAGE_INVOICE(plugin_page));

  priv = plugin_page->priv;
  value = gtk_radio_action_get_current_value(current);
  gnc_invoice_window_sort (priv->iw, value);
  LEAVE(" ");
}


static void
gnc_plugin_page_invoice_cmd_enter (GtkAction *action,
				   GncPluginPageInvoice *plugin_page)
{
  g_return_if_fail(GNC_IS_PLUGIN_PAGE_INVOICE(plugin_page));

  ENTER("(action %p, plugin_page %p)", action, plugin_page);
  gnc_invoice_window_recordCB(NULL, plugin_page->priv->iw);
  LEAVE(" ");
}

static void
gnc_plugin_page_invoice_cmd_cancel (GtkAction *action,
				    GncPluginPageInvoice *plugin_page)
{
  g_return_if_fail(GNC_IS_PLUGIN_PAGE_INVOICE(plugin_page));

  ENTER("(action %p, plugin_page %p)", action, plugin_page);
  gnc_invoice_window_cancelCB(NULL, plugin_page->priv->iw);
  LEAVE(" ");
}

static void
gnc_plugin_page_invoice_cmd_delete (GtkAction *action,
				    GncPluginPageInvoice *plugin_page)
{
  g_return_if_fail(GNC_IS_PLUGIN_PAGE_INVOICE(plugin_page));

  ENTER("(action %p, plugin_page %p)", action, plugin_page);
  gnc_invoice_window_deleteCB(NULL, plugin_page->priv->iw);
  LEAVE(" ");
}

static void
gnc_plugin_page_invoice_cmd_blank (GtkAction *action,
				   GncPluginPageInvoice *plugin_page)
{
  g_return_if_fail(GNC_IS_PLUGIN_PAGE_INVOICE(plugin_page));

  ENTER("(action %p, plugin_page %p)", action, plugin_page);
  gnc_invoice_window_blankCB(NULL, plugin_page->priv->iw);
  LEAVE(" ");
}

static void
gnc_plugin_page_invoice_cmd_duplicate (GtkAction *action,
				       GncPluginPageInvoice *plugin_page)
{
  g_return_if_fail(GNC_IS_PLUGIN_PAGE_INVOICE(plugin_page));

  ENTER("(action %p, plugin_page %p)", action, plugin_page);
  gnc_invoice_window_duplicateCB(NULL, plugin_page->priv->iw);
  LEAVE(" ");
}

static void
gnc_plugin_page_invoice_cmd_pay_invoice (GtkAction *action,
					 GncPluginPageInvoice *plugin_page)
{
  g_return_if_fail(GNC_IS_PLUGIN_PAGE_INVOICE(plugin_page));

  ENTER("(action %p, plugin_page %p)", action, plugin_page);
  gnc_invoice_window_payment_cb(NULL, plugin_page->priv->iw);
  LEAVE(" ");
}

static void
gnc_plugin_page_invoice_cmd_company_report (GtkAction *action,
					    GncPluginPageInvoice *plugin_page)
{
  g_return_if_fail(GNC_IS_PLUGIN_PAGE_INVOICE(plugin_page));

  ENTER("(action %p, plugin_page %p)", action, plugin_page);
  gnc_invoice_window_report_owner_cb(NULL, plugin_page->priv->iw);
  LEAVE(" ");
}

/************************************************************/
/*                    Auxiliary functions                   */
/************************************************************/

static void
gnc_plugin_page_redraw_help_cb (GnucashRegister *g_reg,
				GncPluginPageInvoice *invoice_page)
{
  GncPluginPageInvoicePrivate *priv;
  GncWindow *window;
  const char *status;
  char *help;

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_INVOICE(invoice_page));

  window = GNC_WINDOW(GNC_PLUGIN_PAGE(invoice_page)->window);

  /* Get the text from the ledger */
  priv = invoice_page->priv;
  help = gnc_invoice_get_help(priv->iw);
  status = help ? help : g_strdup("");
  gnc_window_set_status(window, GNC_PLUGIN_PAGE(invoice_page), status);
  g_free(help);
}


void
gnc_plugin_page_invoice_update_title (GncPluginPage *plugin_page)
{
  GncPluginPageInvoice *page;
  gchar *title;

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_INVOICE(plugin_page));

  page = GNC_PLUGIN_PAGE_INVOICE(plugin_page);
  title = gnc_invoice_get_title(page->priv->iw);
  plugin_page = GNC_PLUGIN_PAGE(page);
  gnc_plugin_page_set_tab_name(plugin_page, title);
  gnc_plugin_page_set_title(plugin_page, title);
  g_free(title);
}

static void
gnc_plugin_page_invoice_refresh_cb (GHashTable *changes, gpointer user_data)
{
  GncPluginPageInvoice *page = user_data;
  GtkWidget *reg;

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_INVOICE(page));

  /* We're only looking for forced updates here. */
  if (changes)
    return;

  reg = gnc_invoice_get_register(page->priv->iw);
  gnucash_register_refresh_from_gconf(GNUCASH_REGISTER(reg));
  gtk_widget_queue_draw(page->priv->widget);
}
