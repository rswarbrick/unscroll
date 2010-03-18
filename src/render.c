#include "render.h"
#include "hunks.h"
#include "unscroll.h"
#include <stdlib.h>
#include <math.h>
#include <cairo.h>

extern Settings settings;

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
render_pdf_page (PopplerDocument* doc, GSList *rects, cairo_t *cairo)
{
  PopplerPage *page;

  /*
    The scaling from dimensions in [0,1] x [0,aspect] coordinates to
    output coordinates. Since we know that the width of the page is
    595.0 (todo!), this is just 595.0.
  */
  double virt_to_out_scale = 595.0;

  cairo_surface_t *img;

  while (rects) {
    MappedRectangle *mr = (MappedRectangle *)rects->data;
    /*
      The scaling from source dimensions to output coordinates. This
      should be virt_to_out_scale divided by the width of the bounding
      box on the source page.
    */
    double scale = virt_to_out_scale / mr->src.width;

    page = poppler_document_get_page (doc, mr->src_page);
    img = make_page_surface (page);
    if (cairo_surface_status (img) != CAIRO_STATUS_SUCCESS) {
      printf ("Failed to render page to cairo surface. Error: %s\n",
              cairo_status_to_string (cairo_surface_status (img)));
      exit(1);
    }

    cairo_identity_matrix (cairo);
    cairo_new_path (cairo);

    cairo_translate (cairo,
                     mr->dest.x * virt_to_out_scale,
                     mr->dest.y * virt_to_out_scale);
    cairo_rectangle (cairo,
                     0, 0,
                     mr->src.width * scale, mr->src.height * scale);

    cairo_clip (cairo);

    cairo_scale (cairo, scale, scale);
    cairo_set_source_surface (cairo, img, -mr->src.x, -mr->src.y);

    cairo_paint (cairo);

    cairo_reset_clip (cairo);

    g_object_unref (page);
    cairo_surface_destroy (img);

    rects = g_slist_next (rects);
  }
}

void
render_pdf (PopplerDocument* doc, GSList *page_mappings, const char* fname)
{
  cairo_surface_t *surf = cairo_pdf_surface_create (fname, 595.0, 842.0);
  if (cairo_surface_status (surf) != CAIRO_STATUS_SUCCESS) {
    g_error ("Could not create output surface at %s", fname);
  }
  cairo_t *cairo = cairo_create (surf);

  while (page_mappings) {
    render_pdf_page (doc, (GSList*)page_mappings->data, cairo);
    cairo_surface_show_page (surf);
    page_mappings = g_slist_next (page_mappings);
  }

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
