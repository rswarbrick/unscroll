#include <poppler.h>
#include <glib.h>
#include "hunks.h"

PopplerDocument *document_from_filename (char* filename)
{
  PopplerDocument  *ret;
  gchar            *uri;
  GError           *error = NULL;
  GFile            *file;

  file = g_file_new_for_commandline_arg (filename);
  uri = g_file_get_uri (file);
  g_object_unref (file);

  ret = poppler_document_new_from_file (uri, NULL, &error);
  if (error) {
    g_print ("Error reading PDF: %s\n", error->message);
    g_error_free (error);
    g_free (uri);
    return NULL;
  }
  g_free (uri);

  return ret;
}

int main (int argc, char** argv)
{
  PopplerDocument  *document;
  GSList *pm;

  g_type_init();

  if (argc != 2) {
    g_print ("Usage: unscroll FILE\n");
    return 1;
  }

  document = document_from_filename (argv[1]);
  if (!document) return 1;

  pm = find_new_layout (document);
  
  destroy_page_mappings (&pm);

  g_object_unref (document);
  return 0;
}
