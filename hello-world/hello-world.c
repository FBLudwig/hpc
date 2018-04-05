#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <omp.h>

int main(int argc, char **argv) {

    char hello_world[5][30] = { "Hola mundo", "Hej varlden", "Bonjour tout le monde", "Hallo Welt", "Hello World" };

    #pragma omp parallel for num_threads(4)
    for( int i = 0; i < 5 ; ++i) {
        printf("%s from thread %d of %d\n", hello_world[i], omp_get_thread_num(), omp_get_num_threads());

    }

    return 0;
}
