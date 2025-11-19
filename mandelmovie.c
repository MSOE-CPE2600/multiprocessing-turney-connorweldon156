/*
 * Connor Weldon
 * mandelmovie.c
 * 
 * Usage (short):
 *./mandelmovie -n <num_children> -f <frames> -x <xcenter> -y <ycenter> -s <start_scale>
 *               -z <zoom_factor_per_frame> -W <width> -H <height> -m <maxiter> -o <prefix>
 * 
 */

#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <math.h>
#include <errno.h>

static void usage(const char *prog) {

    fprintf(stderr,
        "Usage: %s -n <num_children> [options]\n"
        "Options:\n"
        "  -n <num>    Number of child processes to run (required)\n"
        "  -f <num>    Number of frames to make (default 50)\n"
        "  -x <val>    X center (default 0)\n"
        "  -y <val>    Y center (default 0)\n"
        "  -s <val>    Starting scale (default 4)\n        (scale is the width in Mandelbot coordinates)\n"
        "  -z <val>    Zoom multiplier per frame (default 0.97)\n        (scale_frame = start_scale * pow(z, frame))\n"
        "  -W <num>    Image width in pixels (default 1000)\n        -H <num>    Image height in pixels (default 1000)\n  -m <num>    Max iterations (default 1000)\n  -o <str>    Output prefix (default mandel)\n\n"
        "Example:\n"
        "%s -n 5 -f 50 -x -0.5 -y 0 -s 4 -z 0.97 -W 1000 -H 1000 -m 1000 -o mandel\n",
        prog, prog);
    exit(1);
}

int main(int argc, char *argv[]){

    // default values
    int opt;
    int num_children = -1; // required
    int frames = 50;
    double xcenter = 0.0, ycenter = 0.0;
    double start_scale = 4.0;
    double zoom = 0.97; // multiplier per frame
    int width = 1000, height = 1000;
    int maxiter = 1000;
    char outprefix[256] = "mandel";

    // parse options
    while ((opt = getopt(argc, argv, "n:f:x:y:s:z:W:H:m:o:h")) != -1) {

        switch (opt) {

            case 'n': num_children = atoi(optarg); break;
            case 'f': frames = atoi(optarg); break;
            case 'x': xcenter = atof(optarg); break;
            case 'y': ycenter = atof(optarg); break;
            case 's': start_scale = atof(optarg); break;
            case 'z': zoom = atof(optarg); break;
            case 'W': width = atoi(optarg); break;
            case 'H': height = atoi(optarg); break;
            case 'm': maxiter = atoi(optarg); break;
            case 'o': strncpy(outprefix, optarg, sizeof(outprefix)-1); outprefix[sizeof(outprefix)-1]=0; break;
            case 'h':
            default:
                usage(argv[0]);
        }
    }

    // error messages
    if (num_children <= 0) {

        fprintf(stderr, "Error: number of children (-n) must be specified and > 0\n");
        usage(argv[0]);
    }

    if (frames <= 0) {

        fprintf(stderr, "Error: number of frames (-f) must be > 0\n");
        exit(1);
    }

    printf("mandelmovie: spawning %d children to create %d frames\n", num_children, frames);
    printf("center=(%g,%g) start_scale=%g zoom=%g size=%dx%d maxiter=%d outprefix=%s\n",
           xcenter, ycenter, start_scale, zoom, width, height, maxiter, outprefix);

    int running = 0;
    int next_frame = 0;

    // keep up to num_children running, create children for frames
    while (next_frame < frames || running > 0) {

        // while there is capacity and frames left
        while (running < num_children && next_frame < frames) {

            pid_t pid = fork();

            if (pid < 0) {

                perror("fork");

                // If fork fails, wait for one child to finish and retry
                if (running > 0) {
                    int status;
                    wait(&status);
                    running--;
                    continue;
                } else {
                    exit(1);
                }
            }

            if (pid == 0) {
                // prepare args and exec
                // compute scale for this frame
                double scale = start_scale * pow(zoom, (double)next_frame);

                // build output filename: "<outprefix><frame>.jpg" (e.g., mandel0.jpg)
                char outname[512];
                snprintf(outname, sizeof(outname), "%s%d.jpg", outprefix, next_frame);

                // argument strings
                char s_x[64], s_y[64], s_scale[64], s_W[16], s_H[16], s_m[16], s_o[512];
                snprintf(s_x, sizeof(s_x), "%g", xcenter);
                snprintf(s_y, sizeof(s_y), "%g", ycenter);
                snprintf(s_scale, sizeof(s_scale), "%g", scale);
                snprintf(s_W, sizeof(s_W), "%d", width);
                snprintf(s_H, sizeof(s_H), "%d", height);
                snprintf(s_m, sizeof(s_m), "%d", maxiter);
                snprintf(s_o, sizeof(s_o), "%s", outname);

                // argv for exec
                char *args[] = {
                    "./mandel",
                    "-x", s_x,
                    "-y", s_y,
                    "-s", s_scale,
                    "-W", s_W,
                    "-H", s_H,
                    "-m", s_m,
                    "-o", s_o,
                    NULL
                };

                // assumes ./mandel exists
                execv(args[0], args);

                // If execv returns, error occurred
                fprintf(stderr, "execv failed for frame %d: %s\n", next_frame, strerror(errno));
                _exit(127);

            } else {
                // parent process
                running++;
                printf("Started child pid %d for frame %d (running=%d)\n", pid, next_frame, running);
                next_frame++;
            }
        }

        // If we've reached capacity, wait for one child to finish before making more
        if (running >= num_children) {

            int status;
            pid_t finished = wait(&status);

            if (finished > 0) {
                
                running--;
                if (WIFEXITED(status)) {
                    int rc = WEXITSTATUS(status);
                    printf("Child %d exited with status %d. running=%d\n", finished, rc, running);
                } else if (WIFSIGNALED(status)) {
                    printf("Child %d killed by signal %d. running=%d\n", finished, WTERMSIG(status), running);
                } else {
                    printf("Child %d ended. running=%d\n", finished, running);
                }

            } else {

                // wait returned error
                if (errno == ECHILD) {
                    // no child processes
                    running = 0;
                } else {
                    perror("wait");
                }

            }
        } else {
            // not at capacity and either we've made all frames or some started but not full:
            // if there are still frames left, loop will try to make more
            // if no frames left, wait for remaining children.
            if (next_frame >= frames && running > 0) {

                // wait for one to finish
                int status;
                pid_t finished = wait(&status);

                if (finished > 0) {

                    running--;

                    if (WIFEXITED(status)) {
                        int rc = WEXITSTATUS(status);
                        printf("Child %d exited with status %d. running=%d\n", finished, rc, running);
                    } else if (WIFSIGNALED(status)) {
                        printf("Child %d killed by signal %d. running=%d\n", finished, WTERMSIG(status), running);
                    } else {
                        printf("Child %d ended. running=%d\n", finished, running);
                    }

                } else {

                    if (errno == ECHILD) running = 0;
                    else perror("wait");
                }
            }
        }
    }

    printf("All frames spawned and children completed.\n");
    return 0;
}