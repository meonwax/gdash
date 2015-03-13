#include <SDL.h>

int main(int argc, char *argv[])
{
  SDL_Surface *surf;
  int i;
  
  SDL_Init(SDL_INIT_EVERYTHING);

  if (argc!=2) {
	printf("Usage: %s <filename>\n", argv[0]);
	return 1;
  }
  
  surf=SDL_LoadBMP(argv[1]);
  if (!surf) {
	printf("Error: %s\n", SDL_GetError());
	return 2;
  }

  for (i=0; i<256; i++) {
	int x, y;
	unsigned char *pixel;
	
	x=(i%16)*9;
	y=(i/16)*6;
	
	pixel=(char *) surf->pixels + y*surf->pitch;
	pixel+=x*surf->format->BytesPerPixel;
	
	printf("%c%c%c", pixel[2], pixel[1], pixel[0]);
  }
}
