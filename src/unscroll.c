#include <poppler.h>
#include <glib.h>
#include <popt.h>
#include <stdlib.h>
#include <paper.h>
#include "hunks.h"
#include "render.h"
#include "unscroll.h"

Settings settings;

static struct poptOption options[] = {
  { "dpi", 'd', POPT_ARG_DOUBLE, &settings.dpi, 0,
    "DPI at which to read the file", "150" },
  { "quality", 'Q', POPT_ARG_INT, &settings.quality, 0,
    "Jpeg compression quality", "75" },
  { "papersize", 's', POPT_ARG_STRING, &settings.papername, 0,
    "Size of output paper", "A4" },
  POPT_AUTOHELP
  { NULL, 0, 0, NULL, 0, NULL, NULL, },
};

static void read_arguments (int argc, const char** argv)
{
  poptContext popt;
  int rc;
  const struct paper* paper;

  popt = poptGetContext ("unscroll", argc, argv, options, 0);

  poptSetOtherOptionHelp(popt, "[OPTIONS]* <infile> <outfile>");
  if (argc < 2) {
    poptPrintUsage(popt, stderr, 0);
    exit(1);
  }

  /* Set defaults */
  settings.dpi = 150.0;
  settings.quality = 75;

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

  /* Deal with paper sizes... */
  paperinit ();

  if (!settings.papername) {
    settings.papername = systempapername ();
  }
  paper = paperinfo (settings.papername);

  if (!paper) {
    fprintf (stderr, "Paper type %s not recognised.\n",
             settings.papername);
    exit (1);
  }

  settings.pswidth = paperpswidth (paper);
  settings.psheight = paperpsheight (paper);

  settings.psleft = (0.05 * settings.pswidth > 28.35) ?
    0.05 * settings.pswidth : 28.35;
  settings.pstop = (0.05 * settings.psheight > 28.35) ?
    0.05 * settings.psheight : 28.35;
  settings.psright = settings.psleft;
  settings.psbottom = settings.pstop;

  paperdone ();

  /* Sanity check for quality parameter */
  if (settings.quality < 0 || settings.quality > 100) {
    fprintf (stderr, "Invalid quality: %d is not in [0, 100]\n",
             settings.quality);
    exit (1);
  }

  /* Check and warn if the user was over-enthusiastic with command
   * line options... */
  if (poptPeekArg (popt)) {
    fprintf (stderr, "Extra arguments on the command line ignored.\n");
  }

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

  render_pdf (document, pm, settings.outfile);
  
  destroy_page_mappings (&pm);

  g_object_unref (document);
  return 0;
}

double output_bb_height ()
{
  return settings.psheight - settings.pstop - settings.psbottom;
}

double output_bb_width ()
{
  return settings.pswidth - settings.psleft - settings.psright;
}

double output_rel_height ()
{
  return output_bb_height () / output_bb_width ();
}
