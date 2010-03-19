#include "jpeg.h"
#include "unscroll.h"
#include <string.h>
#include <stdlib.h>

#include <stdio.h> // needed for jpeglib.h. wtf?
#include <jpeglib.h>

#define BUF_LEN 128
static void init_destination (j_compress_ptr cinfo);
static boolean empty_output_buffer (j_compress_ptr cinfo);
static void term_destination (j_compress_ptr cinfo);

extern Settings settings;

/*
  While compressing, we maintain a singly linked list of guchar
  buffers, each of length BUF_LEN. When this finishes, we glob
  together the whole lot and free the temporary buffers. This last
  step is kind of inefficient I suppose, but this way we only have to
  copy everything once, whereas a std::vector style implementation
  might need multiple copy operations, I think.
 */
static GSList *jpeg_data = NULL;
static guchar *jpeg_ret = NULL;
static guint jpeg_length;
static struct jpeg_destination_mgr jdm = {
  NULL, 0, &init_destination, &empty_output_buffer, &term_destination
};

static void push_data_buf ()
{
  guchar *data = g_new (guchar, BUF_LEN);
  jpeg_data = g_slist_prepend (jpeg_data, data);
  jdm.next_output_byte = data;
  jdm.free_in_buffer = BUF_LEN;
}

static void init_destination (j_compress_ptr cinfo)
{
  if (jpeg_data) {
    g_error ("Cannot compress two jpeg hunks at once.");
  }
  push_data_buf ();
}

static boolean empty_output_buffer (j_compress_ptr cinfo)
{
  if (!jpeg_data) {
    g_error ("Trying to continue jpeg compression without starting?");
  }
  push_data_buf ();
}

static void term_destination (j_compress_ptr cinfo)
{
  if (jpeg_ret) {
    g_error ("Jpeg data already exists, which should have been freed");
  }

  GSList *rest = g_slist_next (jpeg_data);
  rest = g_slist_reverse (rest);

  jpeg_length =
    BUF_LEN * g_slist_length (rest) + (BUF_LEN - jdm.free_in_buffer);

  jpeg_ret = g_new (guchar, jpeg_length);
  guchar *dest = jpeg_ret;

  while (rest) {
    memcpy (dest, rest->data, BUF_LEN);
    g_free (rest->data);
    dest += BUF_LEN;
    rest = g_slist_next (rest);
  }

  memcpy (dest, jpeg_data->data, BUF_LEN - jdm.free_in_buffer);
  g_free (jpeg_data->data);
  g_slist_free (jpeg_data);
  jpeg_data = NULL;
}

/*
  Cairo stores image surfaces in RGBX format (X unused or alpha). I
  need RGBRGBRGB for libjpeg, so we need to do some byte
  shuffling. Yay.

  Hokay, I'm going to assume I'm on a _LITTLE ENDIAN_ machine. In
  which case, the data's arranged BGRA in memory.
 */
static guchar *
copy_scanline (cairo_surface_t *img, guint x, guint y, guint w)
{
  guchar *ret = g_new(guchar, w*3);
  guchar *dest = ret;
  int stride = cairo_image_surface_get_stride (img);
  guchar* src = cairo_image_surface_get_data (img) + stride*y + 4*x;
  int i;
  for (i=0; i<w; i++) {
    dest[0] = src[2];
    dest[1] = src[1];
    dest[2] = src[0];
    src += 4;
    dest += 3;
  }

  return ret;
}

guchar*
make_jpeg_hunk (cairo_surface_t *img,
                guint x, guint y, guint w, guint h, guint* length)
{
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;
  JSAMPROW row_pointer[1];
  int i;

  cairo_format_t format = cairo_image_surface_get_format (img);

  if (format != CAIRO_FORMAT_RGB24 &&
      format != CAIRO_FORMAT_ARGB32) {
    /* The two formats above are stored as RGBX, with X either unused
       or alpha (which we'll ignore) */
    g_error ("Can't create jpeg hunks with non RGBX surfaces.");
  }

  g_assert ( (x >= 0) && (y >= 0) &&
             (x+w <= cairo_image_surface_get_width (img)) &&
             (y+h <= cairo_image_surface_get_height (img)) );

  g_assert (length);

  cinfo.err = jpeg_std_error (&jerr);
  jpeg_create_compress (&cinfo);

  cinfo.dest = &jdm;

  cinfo.image_width = w;
  cinfo.image_height = h;
  cinfo.input_components = 3;
  cinfo.in_color_space = JCS_RGB;
  jpeg_set_defaults (&cinfo);
  jpeg_set_quality (&cinfo, settings.quality, 1);

  jpeg_start_compress (&cinfo, TRUE);
  for (i = y; i < y+h; i++) {
    row_pointer[0] = copy_scanline (img, x, i, w);
    jpeg_write_scanlines (&cinfo, row_pointer, 1);
    g_free (row_pointer[0]);
  }
  jpeg_finish_compress (&cinfo);

  jpeg_destroy_compress (&cinfo);

  g_assert (jpeg_ret);

  guchar *ret = jpeg_ret;
  *length = jpeg_length;

  jpeg_ret = NULL;
  jpeg_length = 0;

  return ret;
}

void
free_jpeg_hunk (guchar* hunk)
{
  g_free (hunk);
}
