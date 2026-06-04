#include "ext.h"
#include "ext_obex.h"
#include "buffer.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define FFT_SIZE    1024
#define HOP_SIZE    512
#define PEAK_WINDOW 10
#define PEAK_THRESH 0.25
#define MAX_ITER    100

void fft(double *re, double *im, int n) {
    int i, j, k, m;
    double u_re, u_im, w_re, w_im, t_re, t_im;
    j = 0;
    for (i = 1; i < n; i++) {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) {
            double tmp;
            tmp = re[i]; re[i] = re[j]; re[j] = tmp;
            tmp = im[i]; im[i] = im[j]; im[j] = tmp;
        }
    }
    for (m = 2; m <= n; m <<= 1) {
        double angle = -2.0 * 3.14159265358979323846 / m;
        w_re = cos(angle); w_im = sin(angle);
        for (k = 0; k < n; k += m) {
            u_re = 1.0; u_im = 0.0;
            for (i = 0; i < m / 2; i++) {
                t_re = u_re * re[k+i+m/2] - u_im * im[k+i+m/2];
                t_im = u_re * im[k+i+m/2] + u_im * re[k+i+m/2];
                re[k+i+m/2] = re[k+i] - t_re;
                im[k+i+m/2] = im[k+i] - t_im;
                re[k+i] += t_re;
                im[k+i] += t_im;
                double new_u_re = u_re * w_re - u_im * w_im;
                u_im = u_re * w_im + u_im * w_re;
                u_re = new_u_re;
            }
        }
    }
}

typedef struct _soundtypes {
    t_object  ob;
    t_symbol *buf_name;
    double    ratio;
    long      num_clusters;
    void     *outlet_segments;  // outputs segment info
    void     *outlet_markov;    // outputs markov transitions
} t_soundtypes;

void *soundtypes_new(t_symbol *s, long argc, t_atom *argv);
void soundtypes_free(t_soundtypes *x);
void soundtypes_assist(t_soundtypes *x, void *b, long m, long a, char *s);
void soundtypes_bang(t_soundtypes *x);
void soundtypes_set(t_soundtypes *x, t_symbol *s);
void soundtypes_clusters(t_soundtypes *x, long n);

static t_class *soundtypes_class;

void ext_main(void *r) {
    t_class *c;
    c = class_new("soundtypes", 
                  (method)soundtypes_new, 
                  (method)soundtypes_free, 
                  sizeof(t_soundtypes), 
                  0L, A_GIMME, 0);
    class_addmethod(c, (method)soundtypes_bang,     "bang",     0);
    class_addmethod(c, (method)soundtypes_set,      "set",      A_SYM,  0);
    class_addmethod(c, (method)soundtypes_clusters, "clusters", A_LONG, 0);
    class_addmethod(c, (method)soundtypes_assist,   "assist",   A_CANT, 0);
    class_register(CLASS_BOX, c);
    soundtypes_class = c;
    post("soundtypes: ready");
}

void *soundtypes_new(t_symbol *s, long argc, t_atom *argv) {
    t_soundtypes *x = (t_soundtypes *)object_alloc(soundtypes_class);
    if (x) {
        x->buf_name     = gensym("");
        x->ratio        = 0.1;
        x->num_clusters = 3;
        // outlets are created right to left
        x->outlet_markov   = outlet_new((t_object *)x, NULL);
        x->outlet_segments = outlet_new((t_object *)x, NULL);
        post("soundtypes: new instance created");
    }
    return x;
}

void soundtypes_free(t_soundtypes *x) {}

void soundtypes_assist(t_soundtypes *x, void *b, long m, long a, char *s) {
    if (m == ASSIST_INLET)
        sprintf(s, "bang to analyse | set <buffer> | clusters <n>");
    else if (a == 0)
        sprintf(s, "segment info: time type start_sample end_sample");
    else
        sprintf(s, "markov: from_type to_type probability");
}

void soundtypes_set(t_soundtypes *x, t_symbol *s) {
    x->buf_name = s;
    post("soundtypes: buffer set to '%s'", s->s_name);
}

void soundtypes_clusters(t_soundtypes *x, long n) {
    if (n < 1) n = 1;
    x->num_clusters = n;
    post("soundtypes: clusters set to %ld", n);
}

void soundtypes_bang(t_soundtypes *x) {
    if (x->buf_name == gensym("")) {
        object_error((t_object *)x, "no buffer set — use [set <buffername>]");
        return;
    }

    // --- open buffer ---
    t_buffer_ref *ref = buffer_ref_new((t_object *)x, x->buf_name);
    t_buffer_obj *buf = buffer_ref_getobject(ref);
    if (!buf) {
        object_error((t_object *)x, "buffer '%s' not found", x->buf_name->s_name);
        object_free(ref);
        return;
    }
    float *samples = buffer_locksamples(buf);
    if (!samples) {
        object_error((t_object *)x, "could not lock buffer");
        object_free(ref);
        return;
    }

    long   num_samples  = buffer_getframecount(buf);
    long   num_channels = buffer_getchannelcount(buf);
    double samplerate   = buffer_getsamplerate(buf);

    // --- mono mixdown ---
    double *mono = (double *)malloc(num_samples * sizeof(double));
    long i;
    for (i = 0; i < num_samples; i++) {
        double sum = 0.0;
        long ch;
        for (ch = 0; ch < num_channels; ch++)
            sum += samples[i * num_channels + ch];
        mono[i] = sum / num_channels;
    }
    buffer_unlocksamples(buf);
    object_free(ref);

    // --- spectral flux ---
    int num_frames = (num_samples - FFT_SIZE) / HOP_SIZE;
    double *flux     = (double *)malloc(num_frames * sizeof(double));
    double *re       = (double *)malloc(FFT_SIZE * sizeof(double));
    double *im       = (double *)malloc(FFT_SIZE * sizeof(double));
    double *prev_mag = (double *)malloc((FFT_SIZE/2) * sizeof(double));

    for (i = 0; i < FFT_SIZE/2; i++) prev_mag[i] = 0.0;

    int frame;
    for (frame = 0; frame < num_frames; frame++) {
        int start = frame * HOP_SIZE;
        for (i = 0; i < FFT_SIZE; i++) {
            double window = 0.5 * (1.0 - cos(2.0 * 3.14159265358979323846 * i / (FFT_SIZE - 1)));
            re[i] = mono[start + i] * window;
            im[i] = 0.0;
        }
        fft(re, im, FFT_SIZE);
        double f = 0.0;
        for (i = 0; i < FFT_SIZE/2; i++) {
            double mag = sqrt(re[i]*re[i] + im[i]*im[i]);
            double diff = mag - prev_mag[i];
            if (diff > 0) f += diff;
            prev_mag[i] = mag;
        }
        flux[frame] = f;
    }
    free(re); free(im); free(prev_mag);

    // --- peak picking ---
    double max_flux = 0.0;
    for (frame = 0; frame < num_frames; frame++)
        if (flux[frame] > max_flux) max_flux = flux[frame];

    int *peak_list = (int *)malloc(num_frames * sizeof(int));
    int  num_peaks = 0;

    for (frame = PEAK_WINDOW; frame < num_frames - PEAK_WINDOW; frame++) {
        if (flux[frame] < PEAK_THRESH * max_flux) continue;
        int is_peak = 1, w;
        for (w = -PEAK_WINDOW; w <= PEAK_WINDOW; w++) {
            if (w == 0) continue;
            if (flux[frame + w] >= flux[frame]) { is_peak = 0; break; }
        }
        if (is_peak) peak_list[num_peaks++] = frame;
    }
    free(flux);

    if (num_peaks < 2) {
        object_error((t_object *)x, "not enough segments found");
        free(mono); free(peak_list);
        return;
    }

    // --- features: RMS per segment ---
    int K = (int)x->num_clusters;
    if (K > num_peaks) K = num_peaks;

    double *features    = (double *)malloc(num_peaks * sizeof(double));
    long   *seg_starts  = (long *)malloc(num_peaks * sizeof(long));
    long   *seg_ends    = (long *)malloc(num_peaks * sizeof(long));

    for (i = 0; i < num_peaks; i++) {
        seg_starts[i] = peak_list[i] * HOP_SIZE;
        seg_ends[i]   = (i + 1 < num_peaks) ? peak_list[i+1] * HOP_SIZE : num_samples;
        double rms = 0.0;
        long j;
        for (j = seg_starts[i]; j < seg_ends[i]; j++)
            rms += mono[j] * mono[j];
        features[i] = sqrt(rms / (seg_ends[i] - seg_starts[i]));
    }
    free(mono);

    // --- KMeans ---
    double *centroids = (double *)malloc(K * sizeof(double));
    int    *labels    = (int *)calloc(num_peaks, sizeof(int));

    for (i = 0; i < K; i++)
        centroids[i] = features[i * (num_peaks / K)];

    int iter, changed;
    for (iter = 0; iter < MAX_ITER; iter++) {
        changed = 0;
        for (i = 0; i < num_peaks; i++) {
            int best = 0; int k;
            double best_dist = fabs(features[i] - centroids[0]);
            for (k = 1; k < K; k++) {
                double dist = fabs(features[i] - centroids[k]);
                if (dist < best_dist) { best_dist = dist; best = k; }
            }
            if (labels[i] != best) { labels[i] = best; changed++; }
        }
        if (!changed) break;
        double *sums   = (double *)calloc(K, sizeof(double));
        int    *counts = (int *)calloc(K, sizeof(int));
        for (i = 0; i < num_peaks; i++) {
            sums[labels[i]]   += features[i];
            counts[labels[i]] ++;
        }
        int k;
        for (k = 0; k < K; k++)
            if (counts[k] > 0) centroids[k] = sums[k] / counts[k];
        free(sums); free(counts);
    }
    free(features); free(centroids);

    // --- output segments from left outlet ---
    // format: segment <index> <time_ms> <type> <start_sample> <end_sample>
    for (i = 0; i < num_peaks; i++) {
        t_atom av[5];
        double time_ms = (seg_starts[i] / samplerate) * 1000.0;
        atom_setlong (av,   i);
        atom_setfloat(av+1, time_ms);
        atom_setlong (av+2, labels[i]);
        atom_setlong (av+3, seg_starts[i]);
        atom_setlong (av+4, seg_ends[i]);
        outlet_anything(x->outlet_segments, gensym("segment"), 5, av);
    }

    // --- build Markov chain ---
    // count transitions between cluster types
    int *trans = (int *)calloc(K * K, sizeof(int));
    for (i = 0; i < num_peaks - 1; i++)
        trans[labels[i] * K + labels[i+1]]++;

    // normalise and output from right outlet
    // format: markov <from_type> <to_type> <probability>
    int from, to;
    for (from = 0; from < K; from++) {
        int total = 0;
        for (to = 0; to < K; to++)
            total += trans[from * K + to];
        if (total == 0) continue;
        for (to = 0; to < K; to++) {
            if (trans[from * K + to] == 0) continue;
            t_atom av[3];
            atom_setlong (av,   from);
            atom_setlong (av+1, to);
            atom_setfloat(av+2, (double)trans[from * K + to] / total);
            outlet_anything(x->outlet_markov, gensym("markov"), 3, av);
        }
    }

    post("soundtypes: analysis complete — %d segments, %d types", num_peaks, K);
    free(trans); free(labels); free(peak_list);
    free(seg_starts); free(seg_ends);
}
