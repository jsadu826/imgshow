#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

const char *char_list = "@8#WkmOI;:\". ";

int main(int argc, char *argv[]) {
  if (argc == 1) {
    fprintf(stderr, "Error! Please provide image path\n");
    return 0;
  } else if (argc > 2) {
    fprintf(stderr, "Error! Too many args\n");
    return 0;
  }

  int width, height, n;
  unsigned char *data = stbi_load(argv[1], &width, &height, &n, 1);
  if (!data) {
    fprintf(stderr, "Error! Failed to load \"%s\".\n", argv[1]);
    return 0;
  }

  char *output = (char *)malloc((width + 1) * height + 20);
  sprintf(output, "%d %d ", width, height);
  size_t init_offs = strlen(output);

  size_t h = 0, w = 0;
  for (; h < height; ++h) {
    for (; w < width; ++w) {
      output[init_offs + (width + 1) * h + w] = char_list[data[width * h + w] / 20];
    }
    output[init_offs + (width + 1) * h + width] = '\n';
    w = 0;
  }

  printf("%s", output);
  free(output);
  free(data);
  return 0;
}
