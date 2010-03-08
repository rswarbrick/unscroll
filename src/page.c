#include "page.h"
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <string.h>
#include <stdio.h>

static void find_white (const GdkPixbuf *buf,
                        gboolean **rows, gboolean **cols)
{
  guint width, height, rowstride, i, j;
  guchar *pixels, *p;

  g_assert (gdk_pixbuf_get_colorspace (buf) == GDK_COLORSPACE_RGB);
  g_assert (gdk_pixbuf_get_bits_per_sample (buf) == 8);
  g_assert (gdk_pixbuf_get_has_alpha (buf));
  g_assert (gdk_pixbuf_get_n_channels (buf) == 4);

  g_assert (rows && cols);

  width = gdk_pixbuf_get_width (buf);
  height = gdk_pixbuf_get_height (buf);
  rowstride = gdk_pixbuf_get_rowstride (buf);

  pixels = gdk_pixbuf_get_pixels (buf);

  *rows = g_new(gboolean, height);
  *cols = g_new(gboolean, width);
  g_assert (*rows && *cols);

  /* This will set 'TRUE' rows and cols to 0xffffffff, which if
   * gboolean is just an int is actually -1. But meh - it comes out
   * as true when you use if on it, which is probably fine.
   */
  memset (*rows, 0xff, height*sizeof(gboolean));
  memset (*cols, 0xff, width*sizeof(gboolean));

  for (i = 0; i < height; i++) {
    for (j = 0; j < width; j++) {
      p = pixels + i*rowstride + 4*j;
      if (!((p[0] == 0xff) && (p[1] == 0xff) && (p[2] == 0xff))) {
        *(*rows + i) = FALSE;
        *(*cols + j) = FALSE;
      }
    }
  }
}

static guint num_enclosed_white_regions (const gboolean *lines, guint length)
{
  guint i;
  gboolean last = TRUE;
  guint count = 0;

  for (i = 0; i < length; i++) {
    if ((!*(lines + i)) && last) {
      count++;
    }
    last = *(lines + i);
  }
  return count - 1; // We count the first white stuff otherwise.
}

static guint first_zero (const gboolean *seq, guint length)
{
  guint i;
  for (i = 0; i < length; i++) if (!*(seq + i)) break;
  return i;
}

static guint last_zero (const gboolean *seq, guint length)
{
  guint i;
  for (i = length-1; i >= 0; i--) if (!*(seq + i)) break;
  return i;
}

static HunkData* find_hunks (const gboolean *lines, guint length,
                             guint* bbstart, guint* bbend, guint* count)
{
  g_assert(lines && bbstart && bbend && count);

  *count = num_enclosed_white_regions (lines, length) + 1;
  guint i, start, hunk_n = 0;
  gboolean last = FALSE;

  HunkData *hunks = g_new(HunkData, *count);

  // Set i to be the first non-white line.
  i = first_zero (lines, length);
  *bbstart = i;
  start = i;

  // Walk down the lines, setting HunkData as we go.
  for (; i < length; i++) {
    if (*(lines + i)) {
      if (!last) {
        (hunks+hunk_n)->start = start;
        (hunks+hunk_n)->end = i;
        hunk_n++;
        *bbend = i-1;
      }
    } else {
      if (last) {
        start = i;
      }
    }
    last = *(lines + i);
  }

  return hunks;
}

PageInfo *analyse_page (PopplerDocument *doc, guint page_num)
{
  PopplerPage *page;
  PageInfo *info;

  GdkPixbuf *image;
  double width_points, height_points;
  int width, height;
  double scale = 1.5;

  gboolean *white_rows, *white_cols;

  page = poppler_document_get_page (doc, page_num);
  if (!page) {
    g_error ("Couldn't open page %d of document", page_num);
  }

  poppler_page_get_size (page, &width_points, &height_points);
  width = (int) ((width_points * scale) + 0.5);
  height = (int) ((height_points * scale) + 0.5);

  image = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, width, height);
  if (!image) {
    g_error ("Couldn't create an image (size %d x %d) for page %d",
             width, height, page_num);
  }

  poppler_page_render_to_pixbuf (page, 0, 0, width, height,
                                 scale, 0, image);
  g_object_unref (page);

  find_white (image, &white_rows, &white_cols);
  g_object_unref (image);

  guint firstrow, lastrow, hunkscount;
  HunkData* hunks = find_hunks (white_rows, height,
                                &firstrow, &lastrow, &hunkscount);

  info = g_new (PageInfo, 1);
  info->bbox.x = first_zero (white_cols, width);
  info->bbox.width = last_zero (white_cols, width) - info->bbox.x;
  if (info->bbox.width <= 0) {
    g_error ("Empty page (%d)? Couldn't find a nonwhite column.",
             page_num);
  }

  info->bbox.y = firstrow;
  info->bbox.height = lastrow - firstrow;

  info->num_hunks = hunkscount;
  info->hunks = hunks;

  g_free (white_rows);
  g_free (white_cols);
  return info;
}

void print_page_info (const PageInfo *pi)
{
  guint i;

  printf ("Bounding box (x,y,w,h) = (%d, %d, %d, %d)\n",
          pi->bbox.x, pi->bbox.y, pi->bbox.width, pi->bbox.height);
  printf ("Hunks:\n");

  for (i=0; i < pi->num_hunks; i++) {
    printf ("%5d to %5d\n", (pi->hunks+i)->start, (pi->hunks+i)->end);
  }
}

void free_page_info (PageInfo **pi)
{
  g_free ((*pi)->hunks);
  g_free (*pi);
  *pi = NULL;
}
