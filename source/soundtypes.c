#include "ext.h"
#include "ext_obex.h"
#include "z_dsp.h"
#include "buffer.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define FFT_SIZE     1024
#define HOP_SIZE     512
#define PEAK_WINDOW  10
#define MAX_ITER     100
#define N_MFCC       13
#define N_FILTERS    26
#define MAX_SEGMENTS 4096

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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
        double angle = -2.0 * M_PI / m;
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

double hz_to_mel(double hz) { return 2595.0 * log10(1.0 + hz / 700.0); }
double mel_to_hz(double mel) { return 700.0 * (pow(10.0, mel / 2595.0) - 1.0); }

void build_mel_filterbank(double *fb, double sr) {
    int n_fft_bins = FFT_SIZE / 2 + 1;
    double mel_min = hz_to_mel(0.0);
    double mel_max = hz_to_mel(sr / 2.0);
    double mel_points[N_FILTERS + 2];
    int    bin_points[N_FILTERS + 2];
    int f, i;
    for (f = 0; f < N_FILTERS + 2; f++) {
        mel_points[f] = mel_min + (mel_max - mel_min) * f / (N_FILTERS + 1);
        bin_points[f] = (int)floor((FFT_SIZE + 1) * mel_to_hz(mel_points[f]) / sr);
    }
    memset(fb, 0, N_FILTERS * n_fft_bins * sizeof(double));
    for (f = 0; f < N_FILTERS; f++) {
        int start = bin_points[f];
        int mid   = bin_points[f + 1];
        int end   = bin_points[f + 2];
        for (i = start; i < mid; i++)
            if (i < n_fft_bins)
                fb[f * n_fft_bins + i] = (double)(i - start) / (mid - start);
        for (i = mid; i < end; i++)
            if (i < n_fft_bins)
                fb[f * n_fft_bins + i] = (double)(end - i) / (end - mid);
    }
}

void compute_mfcc(double *mag, double *fb, double *mfcc, int n_fft_bins) {
    double log_energies[N_FILTERS];
    int f, c;
    for (f = 0; f < N_FILTERS; f++) {
        double energy = 0.0;
        int i;
        for (i = 0; i < n_fft_bins; i++)
            energy += fb[f * n_fft_bins + i] * mag[i];
        log_energies[f] = log(energy + 1e-10);
    }
    for (c = 0; c < N_MFCC; c++) {
        double sum = 0.0;
        for (f = 0; f < N_FILTERS; f++)
            sum += log_energies[f] * cos(M_PI * c * (f + 0.5) / N_FILTERS);
        mfcc[c] = sum;
    }
}

typedef struct _soundtypes {
    t_pxobject  ob;
    t_symbol   *buf_name;
    long        num_clusters;
    double     *corpus_mfcc;
    long       *seg_starts;
    long       *seg_ends;
    int        *seg_labels;
    int         num_segments;
    int         corpus_ready;
    double     *filterbank;
    double      samplerate;
    double      threshold;     // mic energy gate
    double      sensitivity;   // peak picking threshold (0-1)
    double      minlength_ms;  // minimum segment length in ms
    void       *outlet_pos;
    void       *outlet_info;
} t_soundtypes;

void *soundtypes_new(t_symbol *s, long argc, t_atom *argv);
void soundtypes_free(t_soundtypes *x);
void soundtypes_assist(t_soundtypes *x, void *b, long m, long a, char *s);
void soundtypes_bang(t_soundtypes *x);
void soundtypes_set(t_soundtypes *x, t_symbol *s);
void soundtypes_clusters(t_soundtypes *x, long n);
void soundtypes_threshold(t_soundtypes *x, double f);
void soundtypes_sensitivity(t_soundtypes *x, double f);   // NEW
void soundtypes_minlength(t_soundtypes *x, double f);     // NEW
void soundtypes_dsp64(t_soundtypes *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void soundtypes_perform64(t_soundtypes *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam);

static t_class *soundtypes_class;

void ext_main(void *r) {
    t_class *c;
    c = class_new("soundtypes~",
                  (method)soundtypes_new,
                  (method)soundtypes_free,
                  sizeof(t_soundtypes),
                  0L, A_GIMME, 0);
    class_addmethod(c, (method)soundtypes_bang,        "bang",        0);
    class_addmethod(c, (method)soundtypes_set,         "set",         A_SYM,   0);
    class_addmethod(c, (method)soundtypes_clusters,    "clusters",    A_LONG,  0);
    class_addmethod(c, (method)soundtypes_threshold,   "threshold",   A_FLOAT, 0);
    class_addmethod(c, (method)soundtypes_sensitivity, "sensitivity", A_FLOAT, 0);  // NEW
    class_addmethod(c, (method)soundtypes_minlength,   "minlength",   A_FLOAT, 0);  // NEW
    class_addmethod(c, (method)soundtypes_assist,      "assist",      A_CANT,  0);
    class_addmethod(c, (method)soundtypes_dsp64,       "dsp64",       A_CANT,  0);
    class_dspinit(c);
    class_register(CLASS_BOX, c);
    soundtypes_class = c;
    post("soundtypes~: ready");
}

void *soundtypes_new(t_symbol *s, long argc, t_atom *argv) {
    t_soundtypes *x = (t_soundtypes *)object_alloc(soundtypes_class);
    if (x) {
        dsp_setup((t_pxobject *)x, 1);
        x->buf_name     = gensym("");
        x->num_clusters = 3;
        x->corpus_mfcc  = NULL;
        x->seg_starts   = NULL;
        x->seg_ends     = NULL;
        x->seg_labels   = NULL;
        x->num_segments = 0;
        x->corpus_ready = 0;
        x->samplerate   = 44100.0;
        x->threshold    = 0.01;
        x->sensitivity  = 0.25;   // NEW: default (same as old PEAK_THRESH)
        x->minlength_ms = 200.0;  // NEW: default 200ms minimum segment
        x->filterbank   = (double *)malloc(N_FILTERS * (FFT_SIZE/2+1) * sizeof(double));
        build_mel_filterbank(x->filterbank, x->samplerate);
        x->outlet_info  = outlet_new((t_object *)x, NULL);
        x->outlet_pos   = outlet_new((t_object *)x, NULL);
        post("soundtypes~: new instance created");
    }
    return x;
}

void soundtypes_free(t_soundtypes *x) {
    dsp_free((t_pxobject *)x);
    if (x->corpus_mfcc) free(x->corpus_mfcc);
    if (x->seg_starts)  free(x->seg_starts);
    if (x->seg_ends)    free(x->seg_ends);
    if (x->seg_labels)  free(x->seg_labels);
    if (x->filterbank)  free(x->filterbank);
}

void soundtypes_assist(t_soundtypes *x, void *b, long m, long a, char *s) {
    if (m == ASSIST_INLET)
        sprintf(s, "signal: live audio | bang: analyse | set <buffer> | clusters <n> | threshold <f> | sensitivity <f> | minlength <ms>");
    else if (a == 0)
        sprintf(s, "matched segment: start_sample end_sample");
    else
        sprintf(s, "match info: index cluster distance");
}

void soundtypes_set(t_soundtypes *x, t_symbol *s) {
    x->buf_name = s;
    post("soundtypes~: corpus buffer set to '%s'", s->s_name);
}

void soundtypes_clusters(t_soundtypes *x, long n) {
    if (n < 1) n = 1;
    x->num_clusters = n;
    post("soundtypes~: clusters set to %ld", n);
}

void soundtypes_threshold(t_soundtypes *x, double f) {
    if (f < 0.0) f = 0.0;
    x->threshold = f;
    post("soundtypes~: threshold set to %.4f", f);
}

void soundtypes_sensitivity(t_soundtypes *x, double f) {
    if (f < 0.0) f = 0.0;
    if (f > 1.0) f = 1.0;
    x->sensitivity = f;
    post("soundtypes~: sensitivity set to %.3f (re-analyse to apply)", f);
}

void soundtypes_minlength(t_soundtypes *x, double f) {
    if (f < 0.0) f = 0.0;
    x->minlength_ms = f;
    post("soundtypes~: minlength set to %.1f ms (re-analyse to apply)", f);
}

void soundtypes_bang(t_soundtypes *x) {
    if (x->buf_name == gensym("")) {
        object_error((t_object *)x, "no corpus buffer set");
        return;
    }
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
    double sr           = buffer_getsamplerate(buf);

    if (sr != x->samplerate) {
        x->samplerate = sr;
        build_mel_filterbank(x->filterbank, sr);
    }

    // minimum segment length in frames
    long min_frames = (long)((x->minlength_ms / 1000.0) * sr / HOP_SIZE);
    if (min_frames < 1) min_frames = 1;

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

    int num_frames = (num_samples - FFT_SIZE) / HOP_SIZE;
    int n_fft_bins = FFT_SIZE / 2 + 1;
    double *flux       = (double *)malloc(num_frames * sizeof(double));
    double *re         = (double *)malloc(FFT_SIZE * sizeof(double));
    double *im         = (double *)malloc(FFT_SIZE * sizeof(double));
    double *prev_mag   = (double *)malloc(n_fft_bins * sizeof(double));
    double *frame_mfcc = (double *)malloc(num_frames * N_MFCC * sizeof(double));
    double *mag        = (double *)malloc(n_fft_bins * sizeof(double));

    for (i = 0; i < n_fft_bins; i++) prev_mag[i] = 0.0;

    int frame;
    for (frame = 0; frame < num_frames; frame++) {
        int start = frame * HOP_SIZE;
        for (i = 0; i < FFT_SIZE; i++) {
            double window = 0.5 * (1.0 - cos(2.0 * M_PI * i / (FFT_SIZE - 1)));
            re[i] = mono[start + i] * window;
            im[i] = 0.0;
        }
        fft(re, im, FFT_SIZE);
        for (i = 0; i < n_fft_bins; i++)
            mag[i] = sqrt(re[i]*re[i] + im[i]*im[i]);
        double f = 0.0;
        for (i = 0; i < n_fft_bins; i++) {
            double diff = mag[i] - prev_mag[i];
            if (diff > 0) f += diff;
            prev_mag[i] = mag[i];
        }
        flux[frame] = f;
        compute_mfcc(mag, x->filterbank, frame_mfcc + frame * N_MFCC, n_fft_bins);
    }
    free(re); free(im); free(prev_mag); free(mag);

    double max_flux = 0.0;
    for (frame = 0; frame < num_frames; frame++)
        if (flux[frame] > max_flux) max_flux = flux[frame];

    int *peak_list = (int *)malloc(num_frames * sizeof(int));
    int  num_peaks = 0;
    peak_list[num_peaks++] = 0;

    int last_peak = 0;
    for (frame = PEAK_WINDOW; frame < num_frames - PEAK_WINDOW; frame++) {
        // sensitivity controls the threshold
        if (flux[frame] < x->sensitivity * max_flux) continue;

        int is_peak = 1, w;
        for (w = -PEAK_WINDOW; w <= PEAK_WINDOW; w++) {
            if (w == 0) continue;
            if (flux[frame + w] >= flux[frame]) { is_peak = 0; break; }
        }

        // minlength check — skip if too close to last peak
        if (is_peak && (frame - last_peak) >= min_frames && num_peaks < MAX_SEGMENTS) {
            peak_list[num_peaks++] = frame;
            last_peak = frame;
        }
    }
    free(flux);

    if (num_peaks < 2) {
        object_error((t_object *)x, "not enough segments — try lower sensitivity or shorter minlength");
        free(mono); free(frame_mfcc); free(peak_list);
        return;
    }

    int K = (int)x->num_clusters;
    if (K > num_peaks) K = num_peaks;

    if (x->corpus_mfcc) { free(x->corpus_mfcc); x->corpus_mfcc = NULL; }
    if (x->seg_starts)  { free(x->seg_starts);  x->seg_starts  = NULL; }
    if (x->seg_ends)    { free(x->seg_ends);     x->seg_ends    = NULL; }
    if (x->seg_labels)  { free(x->seg_labels);   x->seg_labels  = NULL; }

    x->corpus_mfcc = (double *)malloc(num_peaks * N_MFCC * sizeof(double));
    x->seg_starts  = (long *)malloc(num_peaks * sizeof(long));
    x->seg_ends    = (long *)malloc(num_peaks * sizeof(long));
    x->seg_labels  = (int *)malloc(num_peaks * sizeof(int));

    for (i = 0; i < num_peaks; i++) {
        int f_start = peak_list[i];
        int f_end   = (i + 1 < num_peaks) ? peak_list[i+1] : num_frames - 1;
        x->seg_starts[i] = (long)f_start * HOP_SIZE;
        x->seg_ends[i]   = (long)f_end   * HOP_SIZE;
        double *seg_mfcc = x->corpus_mfcc + i * N_MFCC;
        int c;
        for (c = 0; c < N_MFCC; c++) seg_mfcc[c] = 0.0;
        int count = f_end - f_start;
        if (count < 1) count = 1;
        for (frame = f_start; frame < f_end; frame++)
            for (c = 0; c < N_MFCC; c++)
                seg_mfcc[c] += frame_mfcc[frame * N_MFCC + c];
        for (c = 0; c < N_MFCC; c++) seg_mfcc[c] /= count;
    }
    free(frame_mfcc); free(mono);

    double *centroids = (double *)malloc(K * N_MFCC * sizeof(double));
    memset(x->seg_labels, 0, num_peaks * sizeof(int));
    for (i = 0; i < K; i++)
        memcpy(centroids + i * N_MFCC, x->corpus_mfcc + i * N_MFCC, N_MFCC * sizeof(double));

    int iter;
    for (iter = 0; iter < MAX_ITER; iter++) {
        int changed = 0;
        for (i = 0; i < num_peaks; i++) {
            double *feat = x->corpus_mfcc + i * N_MFCC;
            int best = 0, k, c;
            double best_dist = 1e18;
            for (k = 0; k < K; k++) {
                double dist = 0.0;
                for (c = 0; c < N_MFCC; c++) {
                    double d = feat[c] - centroids[k * N_MFCC + c];
                    dist += d * d;
                }
                if (dist < best_dist) { best_dist = dist; best = k; }
            }
            if (x->seg_labels[i] != best) { x->seg_labels[i] = best; changed++; }
        }
        if (!changed) break;
        double *sums   = (double *)calloc(K * N_MFCC, sizeof(double));
        int    *counts = (int *)calloc(K, sizeof(int));
        for (i = 0; i < num_peaks; i++) {
            int k = x->seg_labels[i];
            int c;
            for (c = 0; c < N_MFCC; c++)
                sums[k * N_MFCC + c] += x->corpus_mfcc[i * N_MFCC + c];
            counts[k]++;
        }
        int k;
        for (k = 0; k < K; k++) {
            if (counts[k] > 0) {
                int c;
                for (c = 0; c < N_MFCC; c++)
                    centroids[k * N_MFCC + c] = sums[k * N_MFCC + c] / counts[k];
            }
        }
        free(sums); free(counts);
    }
    free(centroids);

    x->num_segments = num_peaks;
    x->corpus_ready = 1;
    post("soundtypes~: corpus ready — %d segments, %d clusters, %d iterations (sensitivity=%.2f minlength=%.0fms)",
         num_peaks, K, iter, x->sensitivity, x->minlength_ms);
    free(peak_list);
}

void soundtypes_dsp64(t_soundtypes *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags) {
    x->samplerate = samplerate;
    build_mel_filterbank(x->filterbank, samplerate);
    object_method(dsp64, gensym("dsp_add64"), x, soundtypes_perform64, 0, NULL);
}

static double live_buf[FFT_SIZE];
static int    live_buf_pos   = 0;
static int    live_hop_count = 0;

void soundtypes_perform64(t_soundtypes *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam) {
    double *in = ins[0];
    long n = sampleframes;
    int n_fft_bins = FFT_SIZE / 2 + 1;

    if (!x->corpus_ready) return;

    long s;
    for (s = 0; s < n; s++) {
        live_buf[live_buf_pos++] = in[s];

        if (live_buf_pos >= HOP_SIZE) {
            live_buf_pos = 0;
            live_hop_count++;
            if (live_hop_count % 2 != 0) continue;

            double rms = 0.0;
            int i;
            for (i = 0; i < HOP_SIZE; i++)
                rms += live_buf[i] * live_buf[i];
            rms = sqrt(rms / HOP_SIZE);

            if (rms < x->threshold) return;

            double re[FFT_SIZE], im[FFT_SIZE];
            for (i = 0; i < FFT_SIZE; i++) {
                double window = 0.5 * (1.0 - cos(2.0 * M_PI * i / (FFT_SIZE - 1)));
                re[i] = live_buf[i % HOP_SIZE] * window;
                im[i] = 0.0;
            }
            fft(re, im, FFT_SIZE);

            double mag[FFT_SIZE/2+1];
            for (i = 0; i < n_fft_bins; i++)
                mag[i] = sqrt(re[i]*re[i] + im[i]*im[i]);

            double live_mfcc[N_MFCC];
            compute_mfcc(mag, x->filterbank, live_mfcc, n_fft_bins);

            int best_seg = 0, j, c;
            double best_dist = 1e18;
            for (j = 0; j < x->num_segments; j++) {
                double dist = 0.0;
                for (c = 0; c < N_MFCC; c++) {
                    double d = live_mfcc[c] - x->corpus_mfcc[j * N_MFCC + c];
                    dist += d * d;
                }
                if (dist < best_dist) { best_dist = dist; best_seg = j; }
            }

            t_atom pos_av[2];
            atom_setlong(pos_av,   x->seg_starts[best_seg]);
            atom_setlong(pos_av+1, x->seg_ends[best_seg]);
            outlet_anything(x->outlet_pos, gensym("segment"), 2, pos_av);

            t_atom info_av[3];
            atom_setlong (info_av,   best_seg);
            atom_setlong (info_av+1, x->seg_labels[best_seg]);
            atom_setfloat(info_av+2, best_dist);
            outlet_anything(x->outlet_info, gensym("match"), 3, info_av);
        }
    }
}
