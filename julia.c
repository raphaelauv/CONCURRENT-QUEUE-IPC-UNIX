/* Par Juliusz Chroboczek, 2017. */

#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <complex.h>
#include <unistd.h>
#include <pthread.h>
#include <gtk/gtk.h>
#include "conduct.h"

#define ITERATIONS 1000
#define QSIZE 100
#define COUNT 100

/* Voir la fonction toc ci-dessous. */

double scale = 1024 / 4.0;
int dx, dy;
complex double julia_c;

/* La structure qui contient une requête du programme principal. */

struct julia_request {
    short int x, y;
    short int count;
};

/* La réponse d'un travailleur. */

struct julia_reply {
    short int x, y;
    short int count;
    short int data[COUNT];
};

struct twocons {
    struct conduct *one, *two;
};

/* La fenêtre principale. */

GtkWidget* window;

/* Le gros du boulot. */

static int
julia(double complex z, double complex c)
{
    int i = 0;
    while(i < 1000 && creal(z) * creal(z) + cimag(z) * cimag(z) <= 4.0) {
        z = z * z + c;
        i++;
    }
    return i;
}

/* Associe à une paire de coordonnées un point du plan complexe. */

static double complex
toc(int x, int y)
{
    return ((x - dx) + I * (y - dy)) / scale;
}

static int
min(int a, int b)
{
    return a <= b ? a : b;
}

/* Le travailleur. */

static void *
julia_thread(void *arg)
{
    struct twocons cons = *(struct twocons*)arg;
    while(1) {
        struct julia_request req;
        struct julia_reply rep;
        int rc;

        rc = conduct_read(cons.one, &req, sizeof(req));
        if(rc <= 0) {
            conduct_write_eof(cons.two);
            return NULL;
        }
        rep.x = req.x;
        rep.y = req.y;
        rep.count = req.count;
        for(int i = 0; i < req.count; i++) {
            rep.data[i] = julia(toc(req.x + i, rep.y), julia_c);
        }
        rc = conduct_write(cons.two, &rep, sizeof(rep));
        if(rc < 0) {
            conduct_write_eof(cons.two);
            return NULL;
        }
    }
}

/* Convertit un nombre d'itérations en une couleur, en format RGB24. */

static unsigned int
torgb(int n)
{
    unsigned char r, g, b;

    if(n < 256)
        n *= 2;
    else
        n += 256;

    if(n < 256) {
        r = 255 - n;
        g = 0;
        b = n;
    } else if(n < 512) {
        r = n - 256;
        g = 511 - n;
        b = 0;
    } else if(n < 768) {
        r = 0;
        g = n - 512;
        b = 767 - n;
    } else if(n < 1024) {
        g = 255;
        r = b = n - 768;
    } else {
        r = g = b = 255;
    }

    return r << 16 | g << 8 | b;
}

/* Lit les réponses du travailleur et les met à l'écran.
   repcount est le nombre de réponses à lire. */

static void
paintit(struct twocons *cons, cairo_t *cr, int repcount)
{
    cairo_surface_t *surface;
    unsigned *data;
    unsigned rgb;

    for(int i = 0; i < repcount; i++) {
        struct julia_reply rep;
        int rc;
        rc = conduct_read(cons->two, &rep, sizeof(rep));
        if(rc <= 0) {
            perror("conduct_recv");
            return;
        }
        /* Avec les toolkits modernes, on ne peut plus travailler pixel par
           pixel.  Pas grave, on va travailler en mémoire principale. */
        surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, rep.count, 1);
        data = (unsigned*)cairo_image_surface_get_data(surface);
        for(int j = 0; j < rep.count; j++) {
            int n = rep.data[j];
            if(n >= ITERATIONS) {
                rgb = 0;
            } else {
                rgb = torgb(n);
            }
            data[j] = rgb;
        }
        cairo_surface_mark_dirty(surface);
        cairo_set_source_surface(cr, surface, rep.x, rep.y);
        cairo_paint(cr);
        cairo_surface_destroy(surface);
    }
}

/* Le toolkit nous demande de nous redessiner. */

gboolean
draw_callback (GtkWidget *widget, cairo_t *cr, gpointer data)
{
    struct twocons *cons = (struct twocons*)data;
    double x1, y1, x2, y2;
    int repcount = 0;
    int rc;
    struct timespec t0, t1;

    cairo_clip_extents(cr, &x1, &y1, &x2, &y2);

    clock_gettime(CLOCK_MONOTONIC, &t0);

    for(int j = y1; j <= y2; j++) {
        for(int i = x1; i <= x2; i += COUNT) {
            struct julia_request req;
            req.x = i;
            req.y = j;
            req.count = min(COUNT, x2 - i);
            rc = conduct_write(cons->one, &req, sizeof(req));
            if(rc <= 0) {
                perror("conduct_write");
                /* Pas de bonne façon de gérer l'erreur sans deadlock. */
                continue;
            }
            repcount++;
            if(repcount >= QSIZE) {
                /* Un conduit bloque lorsqu'il est plein.  Il faut donc
                   vider les conduits à temps pour éviter un deadlock. */
                paintit(cons, cr, repcount);
                repcount = 0;
            }
        }
    }
    /* On vide ce qui reste dans les canaux. */
    paintit(cons, cr, repcount);

    clock_gettime(CLOCK_MONOTONIC, &t1);

    printf("Repaint done in %.2lfs\n",
           ((double)t1.tv_sec - t0.tv_sec) +
           ((double)t1.tv_nsec - t0.tv_nsec) / 1.0E9);
    return FALSE;
}

static void
set_title()
{
    char buf[100];
    snprintf(buf, 100, "Julia %lf + %lfi",
             creal(julia_c), cimag(julia_c));
    gtk_window_set_title(GTK_WINDOW(window), buf);
}

/* L'utilisateur a fait clic. */

gboolean
button_callback(GtkWidget *widget, GdkEventButton *event)
{
    if(event->button == 1)
        julia_c = toc(event->x, event->y);

    set_title();
    gtk_widget_queue_draw(widget);
    return TRUE;
}

int main(int argc, char **argv)
{
    GtkWidget* canvas;
    struct twocons cons;
    int numthreads = 0;
    int rc;
    const char *usage = "./julia [-n numthreads]";

    gtk_init(&argc, &argv);

    while(1) {
        int opt = getopt(argc, argv, "n:");
        if(opt < 0)
            break;

        switch(opt) {
        case 'n':
            numthreads = atoi(optarg);
            break;
        default:
            fprintf(stderr, "%s\n", usage);
            exit(1);
        }
    }

    if(optind < argc) {
        fprintf(stderr, "%s\n", usage);
        exit(1);
    }

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(window), 1024, 512);
    dx = 512;
    dy = 256;

    julia_c = 1.0 - (1.0 + sqrt(5.0)) / 2.0;


    canvas = gtk_drawing_area_new();
    gtk_container_add(GTK_CONTAINER(window), canvas);

    cons.one = conduct_create(NULL, sizeof(struct julia_request),
                              QSIZE * sizeof(struct julia_request));
    if(cons.one == NULL) {
        perror("conduct_create");
        exit(1);
    }

    cons.two = conduct_create(NULL, sizeof(struct julia_reply),
                              QSIZE * sizeof(struct julia_reply));

    if(cons.two == NULL) {
        perror("conduct_create");
        exit(1);
    }

    if(numthreads <= 0)
        numthreads = sysconf(_SC_NPROCESSORS_ONLN);
    if(numthreads <= 0) {
        perror("sysconf(_SC_NPROCESSORS_ONLN)");
        exit(1);
    }
    printf("Running %d worker threads.\n", numthreads);

    for(int i = 0; i < numthreads; i++) {
        pthread_t t;
        rc = pthread_create(&t, NULL, julia_thread, &cons);
        if(rc != 0) {
            errno = rc;
            perror("pthread_create");
            exit(1);
        }
        /* On se synchronise à l'aide des conduits, pas besoin de join. */
        pthread_detach(t);
    }

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(canvas, "draw", G_CALLBACK(draw_callback), &cons);
    g_signal_connect(window, "button_press_event",
                     G_CALLBACK(button_callback), NULL);
    set_title();
    gtk_widget_show_all(window);

    gtk_main();

    return 0;
}
