#include <endian.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <sys/time.h>

#define calcIndex(width, x, y)  ((y)*(width) + (x))

int count_live_neighbours(double *currentfield, int x, int y, int width, int height);

long TimeSteps = 100;

void writeVTK2(long timestep, double *data, char prefix[1024], long w, long h) {
    char filename[2048];
    int x, y;

    long offsetX = 0;
    long offsetY = 0;
    float deltax = 1.0;
    float deltay = 1.0;
    long nxy = w * h * sizeof(float);

    snprintf(filename, sizeof(filename), "%s-%05ld%s", prefix, timestep, ".vti");
    FILE *fp = fopen(filename, "w");

    fprintf(fp, "<?xml version=\"1.0\"?>\n");
    fprintf(fp, "<VTKFile type=\"ImageData\" version=\"0.1\" byte_order=\"LittleEndian\" header_type=\"UInt64\">\n");
    fprintf(fp, "<ImageData WholeExtent=\"%d %d %d %d %d %d\" Origin=\"0 0 0\" Spacing=\"%le %le %le\">\n", offsetX,
            offsetX + w - 1, offsetY, offsetY + h - 1, 0, 0, deltax, deltax, 0.0);
    fprintf(fp, "<CellData Scalars=\"%s\">\n", prefix);
    fprintf(fp, "<DataArray type=\"Float32\" Name=\"%s\" format=\"appended\" offset=\"0\"/>\n", prefix);
    fprintf(fp, "</CellData>\n");
    fprintf(fp, "</ImageData>\n");
    fprintf(fp, "<AppendedData encoding=\"raw\">\n");
    fprintf(fp, "_");
    fwrite((unsigned char *) &nxy, sizeof(long), 1, fp);

    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            float value = data[calcIndex(h, x, y)];
            fwrite((unsigned char *) &value, sizeof(float), 1, fp);
        }
    }

    fprintf(fp, "\n</AppendedData>\n");
    fprintf(fp, "</VTKFile>\n");
    fclose(fp);
}


void show(double *currentfield, int w, int h) {
    printf("\033[H");
    int x, y;
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) printf(currentfield[calcIndex(w, x, y)] ? "\033[07m  \033[m" : "  ");
        //printf("\033[E");
        printf("\n");
    }
    fflush(stdout);
}


void evolve(double *currentfield, double *newfield, int w, int h) {
    int square_thread_count = 2;

    omp_set_num_threads(square_thread_count * square_thread_count);
#pragma omp parallel
    {
        int y_region = (h/square_thread_count * (omp_get_thread_num() % square_thread_count));
        int x_region = (w/square_thread_count * (omp_get_thread_num() / square_thread_count));

        int x, y;
        for (y = 0 + y_region; y < h/square_thread_count + y_region; y++) {
            for (x = 0 + x_region; x < w/square_thread_count + x_region; x++) {
                int index = calcIndex(w, x, y);
                int live_neighbours = count_live_neighbours(currentfield, x, y, w, h);

                if (live_neighbours == 3 || (live_neighbours == 2 && currentfield[index])) {
                    newfield[index] = 1;
                } else {
                    newfield[index] = 0;
                }
            }
        }
    }
}

int count_live_neighbours(double *currentfield, int x, int y, int width, int height) {
    int count = 0;
    for (int i = x - 1; i <= x + 1; ++i) {
        for (int j = y - 1; j <= y + 1; ++j) {
            if ((i != x) || (j != y)) {  // Exclude center cell
                int index = calcIndex(width, (i + width) % width, (j + height) % height);
                if (currentfield[index] > 0) {
                    count++;
                }
            }
        }
    }
    return count;
}

void filling(double *currentfield, int w, int h) {
    int i;
    for (i = 0; i < h * w; i++) {
        currentfield[i] = (rand() < RAND_MAX / 10) ? 1 : 0; ///< init domain randomly
    }
}

void game(int w, int h) {
    double *currentfield = calloc(w * h, sizeof(double));
    double *newfield = calloc(w * h, sizeof(double));

    //printf("size unsigned %d, size long %d\n",sizeof(float), sizeof(long));

    filling(currentfield, w, h);
    long t;
    for (t = 0; t < TimeSteps; t++) {
        show(currentfield, w, h);
        evolve(currentfield, newfield, w, h);

        printf("%ld timestep\n", t);
        writeVTK2(t, currentfield, "gol", w, h);

        usleep(200000);

        //SWAP
        double *temp = currentfield;
        currentfield = newfield;
        newfield = temp;
    }

    free(currentfield);
    free(newfield);

}

int main(int c, char **v) {
    int w = 0, h = 0;
    if (c > 1) w = atoi(v[1]); ///< read width
    if (c > 2) h = atoi(v[2]); ///< read height
    if (w <= 0) w = 30; ///< default width
    if (h <= 0) h = 30; ///< default height
    game(w, h);
}
