#include "render.h"
#include "hunks.h"
#include "unscroll.h"
#include "jpeg.h"
#include <stdlib.h>
#include <math.h>
#include <cairo.h>
#include <cairo-pdf.h>

extern Settings settings;

static cairo_surface_t* rendered_page = NULL;
static int rendered_page_num = -1;

static cairo_surface_t*
make_page_surface (PopplerPage *page)
{
  double width, height;
  double scale_factor = settings.dpi / 72.0;
  cairo_surface_t *surface;
  cairo_t *cr;

  poppler_page_get_size (page, &width, &height);

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                        round(scale_factor * width),
                                        round(scale_factor * height));
  cr = cairo_create (surface);
  cairo_scale (cr, scale_factor, scale_factor);
  cairo_save (cr);
  poppler_page_render_for_printing (page, cr);
  cairo_restore (cr);

  cairo_set_operator (cr, CAIRO_OPERATOR_DEST_OVER);
  cairo_set_source_rgb (cr, 1, 1, 1);
  cairo_paint (cr);
  cairo_destroy (cr);

  return surface;
}

static void
free_rendered_page ()
{
  if (rendered_page) {
    cairo_surface_destroy (rendered_page);
    rendered_page = NULL;
  }
  rendered_page_num = -1;
}

static cairo_surface_t*
get_rendered_page (PopplerDocument *doc, int n)
{
  PopplerPage *page;

  if ((n != rendered_page_num) || !rendered_page) {
    free_rendered_page ();

    page = poppler_document_get_page (doc, n);
    rendered_page = make_page_surface (page);
    if (cairo_surface_status (rendered_page) != CAIRO_STATUS_SUCCESS) {
      printf ("Failed to render page to cairo surface. Error: %s\n",
              cairo_status_to_string (cairo_surface_status (rendered_page)));
      exit(1);
    }
    rendered_page_num = n;

    g_object_unref (page);
  }

  return rendered_page;
}

static void
render_pdf_page (PopplerDocument* doc, GSList *rects, cairo_t *cairo)
{
  PopplerPage *page;

  /*
    The scaling from dimensions in [0,1] x [0,aspect] coordinates to
    output coordinates. The output width in points is
    output_bb_height.
  */
  double virt_to_out_scale = output_bb_width ();

  while (rects) {
    MappedRectangle *mr = (MappedRectangle *)rects->data;
    /*
      The scaling from source dimensions to output coordinates. This
      should be virt_to_out_scale divided by the width of the bounding
      box on the source page.

      The output width for example should be mr->src.width * scale,
      but it's possible that we had to scale down the block further,
      so check by multiplying by the stored width in mr->dest (which
      would be 1 if we hadn't scaled)
    */
    double scale = virt_to_out_scale / mr->src.width * mr->dest.width;
    
    cairo_surface_t *img = get_rendered_page (doc, mr->src_page);

    /*
      To include jpeg data in cairo, you use
      cairo_surface_set_mime_data, but you have to do this on an
      existing surface. Since we assume that the jpeg will get
      successfully passed through to the pdf, we allocate any old
      block. Unfortunately, I think you have to use the memory, but
      hopefully that doesn't matter too much.
     */
    guint length;
    guchar *jpg = make_jpeg_hunk (img, mr->src.x, mr->src.y,
                                  mr->src.width, mr->src.height,
                                  &length);
    cairo_surface_t *white_block =
      cairo_image_surface_create (CAIRO_FORMAT_RGB24,
                                  mr->src.width, mr->src.height);

    cairo_surface_set_mime_data (white_block,
                                 CAIRO_MIME_TYPE_JPEG,
                                 jpg, length,
                                 (cairo_destroy_func_t)&free_jpeg_hunk, jpg);

    cairo_identity_matrix (cairo);
    cairo_new_path (cairo);

    cairo_translate (cairo,
                     settings.psleft + mr->dest.x * virt_to_out_scale,
                     settings.pstop + mr->dest.y * virt_to_out_scale);
    cairo_rectangle (cairo,
                     0, 0,
                     mr->src.width * scale, mr->src.height * scale);

    cairo_clip (cairo);
    cairo_scale (cairo, scale, scale);
    cairo_set_source_surface (cairo, white_block, 0, 0);
    cairo_paint (cairo);

    cairo_reset_clip (cairo);

    cairo_surface_destroy (white_block);

    rects = g_slist_next (rects);
  }
}

void
render_pdf (PopplerDocument* doc, GSList *page_mappings, const char* fname)
{
  cairo_surface_t *surf
    = cairo_pdf_surface_create (fname, settings.pswidth, settings.psheight);

  if (cairo_surface_status (surf) != CAIRO_STATUS_SUCCESS) {
    g_error ("Could not create output surface at %s", fname);
  }
  cairo_t *cairo = cairo_create (surf);

  while (page_mappings) {
    render_pdf_page (doc, (GSList*)page_mappings->data, cairo);
    cairo_surface_show_page (surf);
    page_mappings = g_slist_next (page_mappings);
  }

  free_rendered_page ();

  cairo_status_t status;
  status = cairo_status(cairo);
  if (status)
    printf("%s\n", cairo_status_to_string (status));
  cairo_destroy (cairo);
  cairo_surface_finish (surf);
  status = cairo_surface_status(surf);
  if (status)
    printf("%s\n", cairo_status_to_string (status));
  cairo_surface_destroy (surf);

}
