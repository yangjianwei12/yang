/*======================= COPYRIGHT NOTICE ==================================*]
[* Copyright (c) 2020 Qualcomm Technologies, Inc.                            *]
[* All Rights Reserved.                                                      *]
[* Confidential and Proprietary - Qualcomm Technologies, Inc.                *]
[*===========================================================================*/

#include "spectral.h"
#include "mel.h"
#include "taper.h"
#include "math_lib.h"
#include "math_library_c_stubs.h"

#include "string.h"
#include <audio_log/audio_log.h>

#define SCALED_VAD

#define STAT_TENSOR_INDEX (1)
#define NORM_TENSOR_INDEX (2)


static void spectral_do_taper(tEAISpectrum * eaiSpec);





#pragma datasection const
// received from norm pickle
accum norm_init_data_mean[] = {
    -0.001985899090190286k, -0.002094437556995647k, -0.002258811671615319k,
    -0.002315123820303798k, -0.002382603923315807k, -0.002520083383578897k,
    -0.002657099004623679k, -0.002779743316715859k, -0.002860818240718146k,
    -0.002894937705680742k, -0.002996703259359979k, -0.003118800969474389k,
    -0.003166972458703315k, -0.003198158932026395k, -0.003261549231183187k,
    -0.003340757878783005k,
    };

// The following scale is actually in inverse, with scale down by 8 bit
#pragma datasection const
accum norm_init_data_sd[] = {
    0.131435558990803614k, 0.112319449370064217k, 0.121231744459969787k,
    0.110812044944505056k, 0.104454052698102609k, 0.107052900202408402k,
    0.102753432336050290k, 0.103142926148608788k, 0.109417674931516745k,
    0.122691108492241713k, 0.131024662181495721k, 0.135903187460701957k,
    0.146333633422563530k, 0.156226490728766099k, 0.163229665152434034k,
    0.168850677372624491k,
    };
#define NORM_INIT_DATA_SD_SCALE (8)

#define TAPER_STRENGTH_PC (12)
#define FREQ_LOW_HZ (60)
#define FREQ_HIGH_HZ (1000)

static void spectral_do_taper(tEAISpectrum * eaiSpec)
{
    ml_taper_update(eaiSpec->taper_data, (signed *)eaiSpec->internal_buffer_r,(signed *)eaiSpec->internal_buffer_r );
}

static inline sat fract find_mean(sat fract * data, int fft_input_size, int fft_num_points)
{
    accum mean = 0;
#pragma loop minitercount(1)
    for (int i=0; i < fft_input_size; ++i) {
        mean += *data++;
    }
    int shift_points = 30 - __builtin_signdet(fft_num_points);
    mean >>= shift_points;
    return (sat fract)mean;
}

// TODO: Use Normalization
static inline sat fract find_mean_no_integral(sat fract * data, int fft_input_size)
{
    accum mean = 0;
    sat fract by_size = (sat fract)(1.0 / fft_input_size);
#pragma loop minitercount(1)
    for (int i=0; i < fft_input_size; ++i) {
        mean += *data++;
    }
    return (sat fract)(mean * by_size);
}

static int ml_sqrt_ceil(int number)
{
    int start = 0, end = number;
    int mid;
    int inum;

    float ans = 0.0;

    while (start <= end)
    {
        mid = (start + end) >> 1;

        if (mid * mid == number)
        {
            ans = mid;
            break;
        }

        if (mid * mid < number)
        {
            start = mid + 1;
            ans = mid;
        }

        else
        {
            end = mid - 1;
        }
    }

    float increment = 0.1;
    for(int i = 0; i < 5; i++)
    {
        while (ans * ans <= number)
        {
            ans += increment;
        }

        ans = ans - increment;
        increment = (float)(increment * 0.10);
    }

    /* Now get the ciel */
    inum = (int)ans;
    if (ans == (float)inum) {
        return inum;
    }
    return inum + 1;
}


/**
  * The real part is in scratch_buffer and the imaginary part is in real_buffer
  */
STATIC void spectral_do_fft(tEAISpectrum * eaiSpec, bool is_ortho) {
    tFFTStruct fft_data;
    fft_data.numPoints = eaiSpec->fft_size;
    fft_data.real = (int *)eaiSpec->internal_buffer_r;
    fft_data.imag = (int *)eaiSpec->internal_buffer_i;

    sat fract mean = find_mean(eaiSpec->internal_buffer_r, eaiSpec->fft_input_size, eaiSpec->fft_size);
    sat fract * real_ptr = eaiSpec->internal_buffer_r;
#pragma loop minitercount(1)
    for (int i=0; i < eaiSpec->fft_size; ++i) {
        sat fract real_ptr_value = *real_ptr;
        real_ptr_value -= mean;
#ifndef SCALED_VAD
        if (is_ortho) {
            real_ptr_value *= 0.03125r ; //sqrt_1024;
        }
#endif
        *real_ptr++ = real_ptr_value;
    }
#ifdef SCALED_VAD
    /* Calculate scaling */
           accum sum=0;
           int scal2 = 1;
           real_ptr = eaiSpec->internal_buffer_r;
           for (int i=0; i < eaiSpec->fft_size; ++i) {
               sat fract real_ptr_value = *real_ptr++;
               sat fract x2 = real_ptr_value * real_ptr_value;
               sat fract x2_1 = SFRACT_MAX-x2;
               if(sum < x2_1){
                   sum += x2;
               }else{
                   sum -= x2_1;
                   scal2 += 1;
               }
           }

           scal2= ml_sqrt_ceil(scal2);
           sat fract scal_factor = scal2 == 1?SFRACT_MAX:(SFRACT_MAX/scal2);
           eaiSpec->scale_factor = scal_factor;

           /* Apply scaling */
           real_ptr = eaiSpec->internal_buffer_r;
           for (int i=0; i < eaiSpec->fft_size; ++i) {
               sat fract real_ptr_value = *real_ptr;
               //real_ptr_value *= scal_factor;
               if (is_ortho) {
                   real_ptr_value *= (scal_factor >> 5) ; //sqrt_1024;  = 1/32 * scale
               }
               *real_ptr++ = real_ptr_value;
           }
#endif

    ml_fft( &fft_data);
    ml_bitreverse_array((int *)eaiSpec->internal_buffer_r, (int *)eaiSpec->internal_buffer_scratch, eaiSpec->fft_size);
    ml_bitreverse_array((int *)eaiSpec->internal_buffer_i, (int *)eaiSpec->internal_buffer_r, eaiSpec->fft_size);

    eaiSpec->internal_buffer_scratch[0] = 0;
}

static void spec_update_std(tEAISpectrum * eaiSpec, sat fract std, ALGO_OUTPUT_INFO *algo_output) {
    eaiSpec->stat_out_data = std;
    algo_output += STAT_TENSOR_INDEX;
    algo_output->size = 1;
    algo_output->tensor_id = eaiSpec->stat_output;
    algo_output->output_data = (signed *)&eaiSpec->stat_out_data;
}

static void spec_update_norm(tEAISpectrum * eaiSpec, sat fract std, sat fract norm, ALGO_OUTPUT_INFO *algo_output) {
    eaiSpec->norm_out_data[0] = std;
    eaiSpec->norm_out_data[1] = norm;
    algo_output += NORM_TENSOR_INDEX;
    algo_output->size = 2;
    algo_output->tensor_id = eaiSpec->norm_output;
    algo_output->output_data = (signed *)eaiSpec->norm_out_data;
}

STATIC void stats_and_norm(tEAISpectrum * eaiSpec, sat fract * algo_input, ALGO_OUTPUT_INFO *algo_output)
{
    unsigned start = __builtin_max(1, eaiSpec->start_spec_idx);
    unsigned end = eaiSpec->end_spec_idx;
    accum var = 0;
    sat fract * start_ptr = algo_input + start;
//    var = np.sum(self.working_buffer[start:end])
    for (int i= start; i < end; i++)
    {
        var += *start_ptr++;
    }
//        var = var * 2 / self.config.PROCESSING_SAMPLE_SIZE
    var = (accum)((var << 1) * 1.0f / eaiSpec->fft_input_size);

    sat fract taper_correction_1 = 1.0 - (5.0 * TAPER_STRENGTH_PC /  800.0);
    if (TAPER_STRENGTH_PC >= 80) {
                var >>= 2;
    }
    else if (TAPER_STRENGTH_PC > 0){
                var >>= 1;
    }
//    var /= taper_correction_1;

#ifdef SCALED_VAD
   /* Apply scaling */
   accum temp_taper_corr = taper_correction_1;
   temp_taper_corr *= eaiSpec->scale_factor;
   temp_taper_corr *= eaiSpec->scale_factor;
   var /= temp_taper_corr;
#endif

    if (eaiSpec->stat_output >= 0) {
        sat fract std = ml_sqrt(var);
        spec_update_std(eaiSpec, std, algo_output);
    }
    if (eaiSpec->norm_output >= 0) {
        sat fract one_alpha = SFRACT_MAX - eaiSpec->norm_alpha;
        if (eaiSpec->is_next_run) {
            eaiSpec->prev_mean = eaiSpec->norm_alpha * eaiSpec->sample_mean + one_alpha * eaiSpec->prev_mean;
        }
        else {
            eaiSpec->prev_mean = eaiSpec->sample_mean;
        }

        if (eaiSpec->is_next_run) {
                eaiSpec->prev_var = eaiSpec->norm_alpha * var + one_alpha * eaiSpec->prev_var * eaiSpec->prev_var;
        }
        else {
            eaiSpec->prev_var = var;
        }
        eaiSpec->prev_var = ml_sqrt(eaiSpec->prev_var);
        sat fract std = (fract)eaiSpec->prev_var;
        spec_update_norm(eaiSpec, eaiSpec->prev_mean, std, algo_output);
    }
}

STATIC void compute_spectrum(tEAISpectrum * eaiSpec, sat fract * restrict real, sat fract * restrict imag, sat fract * scratch, ALGO_OUTPUT_INFO *algo_output)
{
    sat fract * scratch_old = scratch;

    // use rfft from fft
    int n = ( eaiSpec->fft_size >> 1) + 1;

#pragma loop minitercount(1)
    for (int i=0; i < n; ++i) {
        sat fract rval = *real++;
        sat fract ival = *imag++;
        *scratch++ = (rval*rval + ival*ival);
    }

    // tensor id for the 1st preprocessing output tensor assigned as spec_output
    algo_output->tensor_id = eaiSpec->spec_output;

    stats_and_norm(eaiSpec, scratch_old, algo_output);
    ml_mel_update(eaiSpec->mel_data, (signed *)scratch_old, eaiSpec->fft_size, algo_output);

    sat fract * buffer_ptr = (sat fract *)algo_output->output_data;
#pragma loop minitercount(1)
    for (int i=0; i < algo_output->size; ++i) {
        // ln_by64 : returns ln(x) / 64  (Q1.31)
        accum data = *buffer_ptr;
        sat fract ln_by64_val = (sat fract)(ml_ln(data) >> 6);
        *buffer_ptr ++ = ln_by64_val;
    }
}

STATIC void normalise_spectral_features(tEAISpectrum * eaiSpec, ALGO_OUTPUT_INFO *algo_output)
{
    sat fract * data_ptr = (sat fract *)algo_output->output_data;
    accum * norm_data_mean = eaiSpec->normalization_buffer_mean;
    accum * norm_data_sd = eaiSpec->normalization_buffer_sd;
#pragma loop minitercount(1)
    for (int i=0; i < algo_output->size; ++i) {
        accum data_val = * data_ptr;
//                self.working_buffer[start:end] /= 1 << self.spec_norm_scale
        data_val >>=6; // self.spec_norm_scale = 6
//                self.working_buffer[start:end] -= self.spec_norm_mean[:]
        data_val -= *norm_data_mean++;
//                self.working_buffer[start:end] /= self.spec_norm_std[:]
        data_val *= *norm_data_sd++;
//      apply sd scale
        * data_ptr++ = (sat fract)(data_val << NORM_INIT_DATA_SD_SCALE);
    }
}

#ifdef SCALED_VAD
STATIC void apply_scale(tEAISpectrum * spectrum_data, ALGO_OUTPUT_INFO *algo_output)
{
//        if negate:
//            ln_scale = -ln_scale
//        if self.config.TYPE in (SpectrumType.POWER, SpectrumType.MEL):  # MFCC
//            ln_scale *= 2
    accum ln_scale_k = ml_ln((accum)spectrum_data->scale_factor) >> 6;
    ln_scale_k *= -1;
    ln_scale_k <<= 1;
    sat fract ln_scale_f = (sat fract) ln_scale_k;
    sat fract * output_data = (sat fract *)algo_output->output_data;
#pragma loop minitercount(1)
    for (int i=0; i < algo_output->size; i++){
        *output_data++ += ln_scale_f;
    }
}
#endif

void ml_spectrum_update(tEAISpectrum * spectrum_data, signed *algo_input, ALGO_OUTPUT_INFO *algo_output, unsigned input_size)
{
    int padding_size = (spectrum_data->fft_size - input_size)<<2;
    sat fract * memset_dest  = spectrum_data->internal_buffer_r;

    // pad with zeros in the back
    memcpy(memset_dest, algo_input, input_size * sizeof(sat fract));

    memset_dest += input_size;
    memset(memset_dest, 0, padding_size);

    memset(spectrum_data->internal_buffer_i, 0, spectrum_data->fft_size * sizeof(sat fract));

    spectrum_data->sample_mean = find_mean_no_integral((sat fract*)algo_input, input_size);
    spectral_do_taper(spectrum_data);
    // do orthogonal fft
    spectral_do_fft(spectrum_data, TRUE);
    // Note that becasue of the bit reverse funcitons in spectral_do_fft(),
    //      internal_buffer_scratch contains the real data
    //      internal_buffer_r contains the imaginary data
    //      internal_buffer_i is uses as scratch
    compute_spectrum(spectrum_data, spectrum_data->internal_buffer_scratch, spectrum_data->internal_buffer_r, spectrum_data->internal_buffer_i,algo_output);
    #ifdef SCALED_VAD
   /* Apply scaling */
   apply_scale(spectrum_data, algo_output);
   #endif
    if (spectrum_data->normalization_required) {
        normalise_spectral_features(spectrum_data, algo_output);
    }
    spectrum_data->is_next_run = 1;
}

void ml_spectrum_create(tEAISpectrum ** spectrum_data_ptr, int sampling_rate, int fft_input_size,
                                        int fft_size, int n_spectral_channels, int spec_output, int stat_output, int norm_output, unsigned normalization_required, sat fract norm_alpha)
{
    tEAISpectrum * spectrum_data;
    
    *spectrum_data_ptr = xzpmalloc(sizeof(tEAISpectrum));
    spectrum_data = *spectrum_data_ptr;
    bool init_continue = FALSE;
    if (spectrum_data) {
        init_continue = TRUE;
        //memcpy(eaiSpec, config_data, sizeof(tEAISpectrum));
        spectrum_data->sampling_rate = sampling_rate;
        spectrum_data->fft_input_size = fft_input_size;
        spectrum_data->fft_size = fft_size;
        spectrum_data->n_spectral_channels = n_spectral_channels;
        spectrum_data->spec_output = spec_output;
        spectrum_data->stat_output = stat_output;
        spectrum_data->norm_output = norm_output;
        spectrum_data->norm_alpha = norm_alpha;
        spectrum_data->normalization_required = normalization_required;
        spectrum_data->start_spec_idx = (unsigned)(spectrum_data->fft_size * FREQ_LOW_HZ * 1.0 / spectrum_data->sampling_rate);
        spectrum_data->end_spec_idx = (unsigned)(-((spectrum_data->fft_size * FREQ_HIGH_HZ * 1.0) / (-spectrum_data->sampling_rate)));

        spectrum_data->internal_buffer_r = xzppmalloc(spectrum_data->fft_size * sizeof(sat fract), MALLOC_PREFERENCE_DM1);
        init_continue = (spectrum_data->internal_buffer_r != NULL);
        if (!init_continue)
        {
                pfree(spectrum_data->internal_buffer_r);
                 L2_DBG_MSG("ML_SPECTRUM: cannot create internal buffer r"); 
        }
        if (init_continue) {
            spectrum_data->internal_buffer_i = xzppmalloc(spectrum_data->fft_size * sizeof(sat fract), MALLOC_PREFERENCE_DM2);
            init_continue = (spectrum_data->internal_buffer_i != NULL);
            if (!init_continue)
            {
                pfree(spectrum_data->internal_buffer_i);
                 L2_DBG_MSG("ML_SPECTRUM: cannot create internal buffer i"); 
            }
        }
        if (init_continue) {
            spectrum_data->internal_buffer_scratch = xzppmalloc(spectrum_data->fft_size * sizeof(sat fract), MALLOC_PREFERENCE_DM2);
            init_continue = (spectrum_data->internal_buffer_scratch != NULL);
            if (!init_continue)
            {
                pfree(spectrum_data->internal_buffer_scratch);
                L2_DBG_MSG("ML_SPECTRUM: cannot create internal scratch buffer"); 
            }
        }
        if (init_continue) {
            // init taper values
            L2_DBG_MSG2("ML_SPECTRUM: taper str:%d, fft input size:%d",TAPER_STRENGTH_PC,spectrum_data->fft_input_size); 
            ml_taper_create(&spectrum_data->taper_data,TAPER_STRENGTH_PC,spectrum_data->fft_input_size );
            L2_DBG_MSG1("ML_SPECTRUM: taper ptr:%d",spectrum_data->taper_data);
            init_continue = (spectrum_data->taper_data != NULL);
            if (!init_continue)
            {
               ml_taper_destroy(spectrum_data->taper_data);
               L2_DBG_MSG("ML_SPECTRUM: cannot create taper_data"); 
            }
        }
        if (init_continue) {
            // init mel, using hardcoded data
            ml_mel_create(&spectrum_data->mel_data, spectrum_data->sampling_rate, spectrum_data->fft_size, spectrum_data->n_spectral_channels);
            L2_DBG_MSG1("ML_SPECTRUM: mel ptr:%d",spectrum_data->mel_data);
            init_continue = (spectrum_data->mel_data != NULL);
            if (!init_continue)
            {
               ml_mel_destroy(spectrum_data->mel_data);
                L2_DBG_MSG("ML_SPECTRUM: cannot create mel_data"); 
            }
        }

        if (init_continue && spectrum_data->normalization_required) {
            // init normalization
            spectrum_data->normalization_buffer_mean = xzpmalloc(spectrum_data->n_spectral_channels * sizeof(accum));
            init_continue = (spectrum_data->normalization_buffer_mean != NULL);
            if (init_continue) {
                memcpy(spectrum_data->normalization_buffer_mean, norm_init_data_mean, spectrum_data->n_spectral_channels * sizeof(accum));
            }
            else
            {
                pfree(spectrum_data->normalization_buffer_mean);
                L2_DBG_MSG("ML_SPECTRUM: cannot create normalization_buffer_mean"); 
            }

            spectrum_data->normalization_buffer_sd = xzpmalloc(spectrum_data->n_spectral_channels * sizeof(accum));
            init_continue = (spectrum_data->normalization_buffer_sd != NULL);
            if (init_continue) {
                memcpy(spectrum_data->normalization_buffer_sd, norm_init_data_sd, spectrum_data->n_spectral_channels * sizeof(accum));
            }
            else
            {
                pfree(spectrum_data->normalization_buffer_sd);
                L2_DBG_MSG("ML_SPECTRUM: cannot create normalization_buffer_sd"); 
            }
        }
        if (init_continue) {
            ml_twiddle_init(EAI_TWIDDLE_SIZE);
        }
    }
    if(!init_continue)
    {
         L2_DBG_MSG("ML_SPECTRUM: cannot create Spectrum");
        pfree(spectrum_data);
    }
}

void ml_spectrum_destroy(tEAISpectrum * spectrum_data)
{
    if (spectrum_data) {
        if (spectrum_data->internal_buffer_r) {
            pfree(spectrum_data->internal_buffer_r);
        }
        if (spectrum_data->internal_buffer_i) {
            pfree(spectrum_data->internal_buffer_i);
        }
        if (spectrum_data->internal_buffer_scratch) {
            pfree(spectrum_data->internal_buffer_scratch);
        }
        if (spectrum_data->mel_data) {
            ml_mel_destroy(spectrum_data->mel_data);
        }
        if (spectrum_data->taper_data) {
            ml_taper_destroy(spectrum_data->taper_data);
        }
        if (spectrum_data->normalization_buffer_mean) {
            pfree(spectrum_data->normalization_buffer_mean);
        }
        if (spectrum_data->normalization_buffer_sd) {
            pfree(spectrum_data->normalization_buffer_sd);
        }
        ml_twiddle_release();
        pfree(spectrum_data);
    }

}
