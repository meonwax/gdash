#include <stdio.h>

int main(int argc, char *argv[])
{
  FILE *fp;
  unsigned char pal[768];
  int i;
  int rmin=255, rmax=0, gmin=255, gmax=0, bmin=255, bmax=0;
  
  if (argc!=2) {
	printf("Usage: %s <filename>\n", argv[0]);
	return 1;
  }
  
  fp=fopen(argv[1], "rb");
  if (!fp) {
	printf("Cannot open %s!\n", argv[1]);
	return 2;
  }
  fseek(fp, 0, SEEK_END);
  if (ftell(fp)!=768) {
	printf("File %s should be 768 bytes in size!\n", argv[1]);
	fclose(fp);
	return 3;
  }
  fseek(fp, 0, SEEK_SET);
  fread(pal, 1, 768, fp);
  fclose(fp);
  
  for(i=0; i<256; i++) {
	if (i%8==0)
	  printf("\n");
	if (i%16==0)
	  printf("/* %x */\n", i/16);
	printf("0x%02x%02x%02x, ", pal[i*3], pal[i*3+1], pal[i*3+2]);

	if (pal[i*3]<rmin)
		rmin=pal[i*3];
	if (pal[i*3+1]<gmin)
		gmin=pal[i*3+1];
	if (pal[i*3+2]<bmin)
		bmin=pal[i*3+2];

	if (pal[i*3]>rmax)
		rmax=pal[i*3];
	if (pal[i*3+1]>gmax)
		gmax=pal[i*3+1];
	if (pal[i*3+2]>bmax)
		bmax=pal[i*3+2];
  }
  printf("\n");
  printf("/* r: %d-%d, g: %d-%d, b: %d-%d */\n", rmin, rmax, gmin, gmax, bmin, bmax);
  
  return 0;
}
