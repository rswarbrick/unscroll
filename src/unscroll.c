#include <poppler.h>
#include <glib.h>
#include <popt.h>
#include <stdlib.h>
#include "hunks.h"
#include "render.h"

static struct {
  double dpi;
  const char* infile;
  const char* outfile;
} settings;

static struct poptOption options[] = {
  { "dpi", 'd', POPT_ARG_DOUBLE, &settings.dpi, 0,
    "DPI at which to read the file", "150" },
  POPT_AUTOHELP
  { NULL, 0, 0, NULL, 0, NULL, NULL, },
};

static void read_arguments (int argc, const char** argv)
{
  poptContext popt
    = poptGetContext ("unscroll", argc, argv, options, 0);
  int rc;

  poptSetOtherOptionHelp(popt, "[OPTIONS]* <infile> <outfile>");
  if (argc < 2) {
    poptPrintUsage(popt, stderr, 0);
    exit(1);
  }

  /* Set defaults */
  settings.dpi = 150.0;

  /* Read in all the standard options */
  while ((rc = poptGetNextOpt (popt)) > 0) {}
  /* -1 is standard popt end of args. Lower is error */
  if (rc < -1) {
    fprintf (stderr, "%s: %s\n\n",
             poptBadOption(popt, POPT_BADOPTION_NOALIAS),
             poptStrerror (rc));
    poptPrintUsage (popt, stderr, 0);
    exit (1);
  }

  /* Hopefully, we've still got an input file */
  settings.infile = poptGetArg (popt);
  if (!settings.infile) {
    fprintf (stderr, "No input file specified.\n");
    poptPrintUsage (popt, stderr, 0);
    exit (1);
  }

  /* There should also be an output file */
  settings.outfile = poptGetArg (popt);
  if (!settings.outfile) {
    fprintf (stderr, "No output file specified.\n");
    poptPrintUsage (popt, stderr, 0);
    exit (1);
  }

  if (poptPeekArg (popt)) {
    fprintf (stderr, "Extra arguments on the command line ignored.\n");
  }

  printf ("dpi = %f\n", settings.dpi);

  poptFreeContext (popt);
}


PopplerDocument *document_from_filename (const char* filename)
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

int main (int argc, const char** argv)
{
  PopplerDocument  *document;
  GSList *pm;

  g_type_init();

  read_arguments(argc, argv);

  document = document_from_filename (settings.infile);
  if (!document) return 1;

  pm = find_new_layout (document);

  render_pdf (document, pm, "out.pdf");
  
  destroy_page_mappings (&pm);

  g_object_unref (document);
  return 0;
}
