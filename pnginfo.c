#include <stdio.h>
#include <stdlib.h>

typedef struct {
    unsigned int length;    /* length of data in the chunk, host byte order */
    unsigned char type[4];  /* chunk type */
    unsigned char *p_data;  /* pointer to location where the actual data are */
    unsigned int crc;       /* CRC field  */
} chunk_p;

typedef struct {
    unsigned int length;          /* length of data in the chunk, host byte order */
    unsigned char type[4];        /* chunk type */
    unsigned int crc;             /* CRC field  */

    unsigned int width;           /* width in pixels, big endian   */
    unsigned int height;          /* height in pixels, big endian  */
    unsigned char bit_depth;      /* num of bits per sample or per palette index.
                                   * valid values are: 1, 2, 4, 8, 16 */
    unsigned char color_type;     /* =0: Grayscale; =2: Truecolor; =3 Indexed-color
                                   * =4: Greyscale with alpha; =6: Truecolor with alpha */
    unsigned char compression;    /* only method 0 is defined for now */
    unsigned char filter;         /* only method 0 is defined for now */
    unsigned char interlace;      /* =0: no interlace; =1: Adam7 interlace */
} data_IHDR_p;

typedef struct {
    unsigned long long signature;
    data_IHDR_p* p_IHDR;
    chunk_p* p_IDAT; /* only handles one IDAT chunk */
    chunk_p* p_IEND;
} simple_PNG_p;

/* Prototypes */
void getSignature(const unsigned char* png_buffer, unsigned long long* signature);
void getIHDR(const unsigned char* png_buffer, data_IHDR_p* IHDR);
void getIDAT(unsigned char* png_buffer, chunk_p* IDAT, long png_size);
void getIEND(const unsigned char* png_buffer, chunk_p* IEND, long png_size);
unsigned long crc(unsigned char *buf, int len);

int main(int argc, char *argv[])
{

    if (argc == 1) {
        printf("%s: Not a PNG file\n", argv[1]);
        return 0;
    }

    FILE* png = NULL;                                      /* points to potential png file */
    unsigned char* png_buffer = NULL;                      /* stores png file */
    long png_size = 0;                                     /* stores png size in bytes */

    /*Open png for reading only*/
    png = fopen(argv[1], "r");

    /* File doesn't exist */
    if(png == NULL) {
        printf("%s: Not a PNG file\n", argv[1]);
        return 0;
    }

    simple_PNG_p* png_data = malloc(sizeof(simple_PNG_p)); /* stores organized png data */
    png_data->p_IHDR = malloc(sizeof(data_IHDR_p));        /* stores IHDR chunk */
    png_data->p_IDAT = malloc(sizeof(chunk_p));            /* stores IDAT chunk */
    png_data->p_IEND = malloc(sizeof(chunk_p));            /* stores IEND chunk */

    /*Set file position indicator to end of file, get size,
     * then set indicator back to start of file*/
    fseek(png, 0, SEEK_END);
    png_size = ftell(png);
    fseek(png, 0, SEEK_SET);

    /*Allocate enough memory to accommodate size of png*/
    png_buffer = malloc(png_size);

    /*Store all png_size bytes of png into buffer*/
    fread(png_buffer, 1, png_size, png);

    /* Get PNG data */
    getSignature(png_buffer, &png_data->signature);
    getIHDR(png_buffer, png_data->p_IHDR);
    getIDAT(png_buffer, png_data->p_IDAT, png_size);
    getIEND(png_buffer, png_data->p_IEND, png_size);

    /*Check if valid png signature*/
    if (png_data->signature == 0x89504e470d0a1a0a) {

        /*Print height and width*/
        printf("%s: %d x %d\n", argv[1], png_data->p_IHDR->width, png_data->p_IHDR->height);

        /*Check for crc errors*/
        unsigned int IHDR_crc = crc(&png_buffer[12], png_data->p_IHDR->length+4);         /*Check IHDR crc*/
        unsigned int IDAT_crc = crc(&png_buffer[37], png_data->p_IDAT->length+4);         /*Check IDAT crc*/
        unsigned int IEND_crc = crc(&png_buffer[png_size-8], png_data->p_IEND->length+4); /*Check IEND crc*/

        if (IHDR_crc != png_data->p_IHDR->crc) {
            printf("IHDR chunk CRC error: computed %x, expected %x\n", IHDR_crc, png_data->p_IHDR->crc);
        }
        if (IDAT_crc != png_data->p_IDAT->crc) {
            printf("IDAT chunk CRC error: computed %x, expected %x\n", IDAT_crc, png_data->p_IDAT->crc);
        }
        if (IEND_crc != png_data->p_IEND->crc) {
            printf("IEND chunk CRC error: computed %x, expected %x\n", IEND_crc, png_data->p_IEND->crc);
        }
    }
    else {
        printf("%s: Not a PNG file\n", argv[1]);
    }

    /* Free heap */
    free(png_buffer);
    free(png_data->p_IHDR);
    free(png_data->p_IDAT);
    free(png_data->p_IEND);
    free(png_data);

    fclose(png);
    return 0;
}

void getSignature(const unsigned char* png_buffer, unsigned long long* signature) {
    unsigned long long signature_temp1 = (png_buffer[0] << 24) + (png_buffer[1] << 16) + (png_buffer[2] << 8) + png_buffer[3];
    unsigned long long signature_temp2 = (png_buffer[4] << 24) + (png_buffer[5] << 16) + (png_buffer[6] << 8) + png_buffer[7];
    *signature = (signature_temp1 << 32) + signature_temp2;
}

void getIHDR(const unsigned char* png_buffer, data_IHDR_p* IHDR) {
    IHDR->length = (png_buffer[8] << 24) + (png_buffer[9] << 16) + (png_buffer[10] << 8) + png_buffer[11];
    IHDR->type[0] = png_buffer[12]; IHDR->type[1] = png_buffer[13]; IHDR->type[2] = png_buffer[14]; IHDR->type[3] = png_buffer[15];
    IHDR->width = (png_buffer[16] << 24) + (png_buffer[17] << 16) + (png_buffer[18] << 8) + png_buffer[19];
    IHDR->height = (png_buffer[20] << 24) + (png_buffer[21] << 16) + (png_buffer[22] << 8) + png_buffer[23];
    IHDR->bit_depth = png_buffer[24];
    IHDR->color_type = png_buffer[25];
    IHDR->compression = png_buffer[26];
    IHDR->filter = png_buffer[27];
    IHDR->interlace = png_buffer[28];
    IHDR->crc = (png_buffer[29] << 24) + (png_buffer[30] << 16) + (png_buffer[31] << 8) + png_buffer[32];
}

void getIDAT(unsigned char* png_buffer, chunk_p* IDAT, long png_size) {
    IDAT->length = (png_buffer[33] << 24) + (png_buffer[34] << 16) + (png_buffer[35] << 8) + png_buffer[36];
    IDAT->type[0] = png_buffer[37]; IDAT->type[1] = png_buffer[38]; IDAT->type[2] = png_buffer[39]; IDAT->type[3] = png_buffer[40];
    IDAT->p_data = &png_buffer[41];
    IDAT->crc = (png_buffer[png_size-16] << 24) + (png_buffer[png_size-15] << 16) + (png_buffer[png_size-14] << 8) + png_buffer[png_size-13];
}

void getIEND(const unsigned char* png_buffer, chunk_p* IEND, long png_size) {
    IEND->length = (png_buffer[png_size-12] << 24) + (png_buffer[png_size-11] << 16) + (png_buffer[png_size-10] << 8) + png_buffer[png_size-9];
    IEND->type[0] = png_buffer[png_size-8]; IEND->type[1] = png_buffer[png_size-7]; IEND->type[2] = png_buffer[png_size-6]; IEND->type[3] = png_buffer[png_size-5];
    IEND->p_data = NULL;
    IEND->crc = (png_buffer[png_size-4] << 24) + (png_buffer[png_size-3] << 16) + (png_buffer[png_size-2] << 8) + png_buffer[png_size-1];
}

/********************************************************************
 * @file: crc.c
 * @brief: PNG crc calculation
 * Reference: https://www.w3.org/TR/PNG-CRCAppendix.html
 */

/* Table of CRCs of all 8-bit messages. */
unsigned long crc_table[256];

/* Flag: has the table been computed? Initially false. */
int crc_table_computed = 0;

/* Make the table for a fast CRC. */
void make_crc_table(void)
{
    unsigned long c;
    int n, k;

    for (n = 0; n < 256; n++) {
        c = (unsigned long) n;
        for (k = 0; k < 8; k++) {
            if (c & 1)
                c = 0xedb88320L ^ (c >> 1);
            else
                c = c >> 1;
        }
        crc_table[n] = c;
    }
    crc_table_computed = 1;
}

/* Update a running CRC with the bytes buf[0..len-1]--the CRC
   should be initialized to all 1's, and the transmitted value
   is the 1's complement of the final running CRC (see the
   crc() routine below)). */

unsigned long update_crc(unsigned long crc, unsigned char *buf, int len)
{
    unsigned long c = crc;
    int n;

    if (!crc_table_computed)
        make_crc_table();
    for (n = 0; n < len; n++) {
        c = crc_table[(c ^ buf[n]) & 0xff] ^ (c >> 8);
    }
    return c;
}

/* Return the CRC of the bytes buf[0..len-1]. */
unsigned long crc(unsigned char *buf, int len)
{
    return update_crc(0xffffffffL, buf, len) ^ 0xffffffffL;
}