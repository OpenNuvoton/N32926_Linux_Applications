/*-----------------------------------------------------------------------------
 * CJChen1@nuvoton, 2011/8/19, To meet Nuvoton chip design for BCH T24, we have to
 *      1. cut input data to several 1024 bytes (8192 bits) segments for T24;
 *         cut input data to several  512 bytes (4096 bits) segments for others;
 *      2. pad some bytes 0 for each data segments;
 *              for T4,  parity lenght is  60 bits <=  8 bytes, we need to padding (32-8)  bytes 0
 *              for T8,  parity lenght is 120 bits <= 15 bytes, we need to padding (32-15) bytes 0
 *              for T12, parity lenght is 180 bits <= 23 bytes, we need to padding (32-23) bytes 0
 *              for T15, parity lenght is 225 bits <= 29 bytes, we need to padding (32-29) bytes 0
 *              for T24, parity lenght is             45 bytes, we need to padding (64-45) bytes 0
 *      3. invert each data segment by bit-stream order;
 *      4. calculate BCH parity code for each data segment by normal BCH algorithm;
 *      5. invert each parity by bit-stream order.
 *
 *      Besides, we support enable/disable for SMCR[PROT_3BEN].
 *
 * CJChen1@nuvoton, 2011/1/31, To verify reliability for this program,
 *      6. modify output format in order to compare to chip data easier.
 *              output raw data (no inverting and padding) and parity with Nuvoton style.
 *---------------------------------------------------------------------------*/
#define CFLAG_DEBUG             0
#define CFLAG_NTC               1

#if CFLAG_NTC
    #define NTC_DATA_FULL_SIZE      (ntc_data_size + data_pad_size)
    #define NTC_DATA_SIZE_512       4096
    #define NTC_DATA_SIZE_1024      8192
    int ntc_data_size;              // the bit size for one data segment
    int data_pad_size;              // the bit length to padding 0, value base on BCH type
    int Redundancy_protect;         // Redundancy protect indicator

    // define the total padding bytes for 512/1024 data segment
    #define BCH_PADDING_LEN_512     32
    #define BCH_PADDING_LEN_1024    64
    // define the BCH parity code lenght for 512 bytes data pattern
    #define BCH_PARITY_LEN_T4   8
    #define BCH_PARITY_LEN_T8   15
    #define BCH_PARITY_LEN_T12  23
    #define BCH_PARITY_LEN_T15  29
    // define the BCH parity code lenght for 1024 bytes data pattern
    #define BCH_PARITY_LEN_T24      45
#endif

#include "bch_global.c"

int bb[rr_max] ;        // Parity checks

void parallel_encode_bch(unsigned char *input_ra_data)
/* Parallel computation of n - k parity check bits.
 * Use lookahead matrix T_G_R.
 * The incoming streams are fed into registers from the right hand
 */
{   int i, j, iii, Temp, bb_temp[rr_max] ;
    int loop_count ;
    
#if CFLAG_NTC
/*-----------------------------------------------------------------------------
 * CJChen1@nuvoton, 2011/1/20, To meet Nuvoton chip design, we have to
 *      2. pad some bytes 0 for each data segments;
 *      3. invert each data segment by bit-stream order;
 *      The length of data segment MUST be (segment+padding) bits.
 *      Element of data[x] is one bit for input data.
 *
 *      Besides, we support enable/disable for SMCR[PROT_3BEN].
 *      Here, we modify padding data according to variable Redundancy_protect.
 *---------------------------------------------------------------------------*/
    for (i = kk_shorten; i < NTC_DATA_FULL_SIZE; i++) // padding 0
    {   data[i] = 0; }

    // to support SMCR[PROT_3BEN] enable function.
    if (Redundancy_protect)
    {
//        for (i = kk_shorten; i < kk_shorten + 16; i++)
//            data[i] = 1;    // padding redundancy data 0xffff00 if SMCR[PROT_3BEN] enable

        // padding redundancy data from input_ra_data if SMCR[PROT_3BEN] enable
        for (i = 0; i < 24; i++)
        {
            j = i / 8;      // byte index of input_ra_data[]
            if (i % 8 == 0)
                iii = 7;    // bit index of input_ra_data[j]
            data[kk_shorten+i] = (input_ra_data[j] >> iii) & 0x01;  // convert one bit one element of data[]
            iii--;
        }
    }

    kk_shorten += data_pad_size;  // temporarily, extend kk_shorten to include padding 0

    i = 0;
    j = NTC_DATA_FULL_SIZE - 1;   // always invert (raw data + padding data)
    while (i < j)
    {
        Temp = data[i];
        data[i] = data[j];
        data[j] = Temp;
        i++;
        j--;
    }
#endif

    // Determine the number of loops required for parallelism.
    loop_count = ceil(kk_shorten / (double)Parallel) ;

    // Serial to parallel data conversion
    for (i = 0; i < Parallel; i++)
    {
        for (j = 0; j < loop_count; j++)
        {
            Temp = i + j * Parallel;
            if (Temp < kk_shorten)
                data_p[i][j] = data[Temp];
            else
                data_p[i][j] = 0;
        }
    }

/*-----------------------------------------------------------------------------
 * CJChen1@nuvoton, 2011/1/20, modify nothing, just describe the structure of data_p.
 *      Element of data_p[r][c] is one bit for input stream. The bit order is
 *          data_p[0][0]=bit 0,     data_p[0][1]=bit p,    ...    , data_p[0][loop_count-1]
 *          data_p[1][0]=bit 1,     data_p[1][1]=bit p+1,  ...    , data_p[1][loop_count-1]
 *               ...                     ...
 *          data_p[p-1][0]=bit p-1, data_p[p-1][1]=bit 2*p-1, ... , data_p[p-1][loop_count-1]
 *          where p is Parallel.
 *---------------------------------------------------------------------------*/

    // Initialize the parity bits.
    for (i = 0; i < rr; i++)
        bb[i] = 0;

    // Compute parity checks
    // S(t) = T_G_R [ S(t-1) + M(t) ]
    // Ref: Parallel CRC, Shieh, 2001
    for (iii = loop_count - 1; iii >= 0; iii--)
    {   for (i = 0; i < rr; i++)
            bb_temp[i] = bb[i] ;
        for (i = Parallel - 1; i >= 0; i--)
            bb_temp[rr - Parallel + i] = bb_temp[rr - Parallel + i] ^ data_p[i][iii];

        for (i = 0; i < rr; i++)
        {   Temp = 0;
            for (j = 0; j < rr; j++)
                Temp = Temp ^ (bb_temp[j] & T_G_R[i][j]);
            bb[i] = Temp;
        }
    }

#if CFLAG_NTC
    kk_shorten -= data_pad_size;  // recover kk_shorten

/*-----------------------------------------------------------------------------
 * CJChen1@nuvoton, 2011/1/20, To meet Nuvoton chip design, we have to
 *      5. invert each parity by bit-stream order.
 *      Element of bb[x] is one bit for output parity.
 *---------------------------------------------------------------------------*/
    i = 0;
    j = rr -1;
    while (i < j)
    {
        Temp = bb[i];
        bb[i] = bb[j];
        bb[j] = Temp;
        i++;
        j--;
    }
#endif
}


int calculate_BCH_parity_in_field(
    unsigned char *input_data,
    unsigned char *input_ra_data,
    int bch_error_bits,
    int protect_3B,
    int field_index,
    unsigned char *output_bch_parity,
    int bch_need_initial)
{
    int field_parity_size;      // the BCH parity size for one field to return
    int input_data_index;
    int i, j;
    int in_count, in_v, in_codeword;    // Input statistics
    char in_char;

    //fprintf(stderr, "# Binary BCH encoder.  Use -h for details.\n\n");
    input_data_index = 0;

#if CFLAG_NTC
/*-----------------------------------------------------------------------------
 * CJChen1@nuvoton, 2011/1/27, To meet Nuvoton chip design, we have to
 *      support enable/disable for SMCR[PROT_3BEN].
 *      Here, we disable this feature by default.
 *---------------------------------------------------------------------------*/
    Redundancy_protect = 0;
#endif

    Verbose = 0;
    mm = df_m;
    tt = df_t;
    Parallel = df_p;

    //--- initial BCH parameters for Nuvoton
    mm = 15;
    tt = bch_error_bits;
    /*-----------------------------------------------------------------------------
     * CJChen1@nuvoton, 2011/1/28, To meet Nuvoton chip design, we have to
     *      2. pad some bytes 0 for each data segments;
     *      Here, we set the data size and padding size according to bch_error_bits
     *---------------------------------------------------------------------------*/
    switch (tt)
    {
        case  4: ntc_data_size = NTC_DATA_SIZE_512;
                 data_pad_size = (BCH_PADDING_LEN_512-BCH_PARITY_LEN_T4)*8;   break;  // *8 : byte --> bit
        case  8: ntc_data_size = NTC_DATA_SIZE_512;
                 data_pad_size = (BCH_PADDING_LEN_512-BCH_PARITY_LEN_T8)*8;   break;
        case 12: ntc_data_size = NTC_DATA_SIZE_512;
                 data_pad_size = (BCH_PADDING_LEN_512-BCH_PARITY_LEN_T12)*8;  break;
        case 15: ntc_data_size = NTC_DATA_SIZE_512;
                 data_pad_size = (BCH_PADDING_LEN_512-BCH_PARITY_LEN_T15)*8;  break;
        case 24: ntc_data_size = NTC_DATA_SIZE_1024;
                 data_pad_size = (BCH_PADDING_LEN_1024-BCH_PARITY_LEN_T24)*8; break;
        default:
            fprintf(stderr, "### t must be 4 or 8 or 12 or 15 or 24.\n\n");
            break;
    }

/*-----------------------------------------------------------------------------
 * CJChen1@nuvoton, 2012/7/27, according to Parallel CRC request,
 *      the Parallel MUST < parity code length (n-k), so, for Nuvoton
 *          for T4,  parity lenght is  60 bits, max Parallel is 32
 *          for T8,  parity lenght is 120 bits, max Parallel is 64
 *          for T12, parity lenght is 180 bits, max Parallel should be 128
 *          for T15, parity lenght is 225 bits, max Parallel should be 128
 *          for T24, parity lenght is 360 bits, ??
 *      Please also modify the parallel_max definition in bch_global.c
 *---------------------------------------------------------------------------*/
    if (bch_error_bits == 4)
        Parallel = 32 /*8*/;
    else
        Parallel = 64 /*8*/;
    Redundancy_protect = protect_3B;

#if CFLAG_NTC
/*-----------------------------------------------------------------------------
 * CJChen1@nuvoton, 2011/1/20, To meet Nuvoton chip design, we have to
 *      1. cut input data to several 512/1024 bytes segments;
 *      Here, we force kk_shorten=4096/8192 bits so that algorithm will
 *      calculate one BCH parity code for each 512/1024 bytes segment.
 *---------------------------------------------------------------------------*/
    kk_shorten = ntc_data_size;

    // to show configuration about SMCR[PROT_3BEN] function.
/*
    if (Redundancy_protect)
        fprintf(stdout, "{### Enable SMCR[PROT_3BEN] feature.}\n");
    else
        fprintf(stdout, "{### Disable SMCR[PROT_3BEN] feature.}\n");
*/
#endif

if (bch_need_initial)
{
    //--- really do BCH encoding
    nn = (int)pow(2, mm) - 1 ;
    nn_shorten = nn ;

    // generate the Galois Field GF(2**mm)
    generate_gf() ;

    // Compute the generator polynomial and lookahead matrix for BCH code
    gen_poly() ;

    // Check if code is shortened
    nn_shorten = kk_shorten + rr ;
}   // end of if(bch_need_initial)

    //fprintf(stdout, "{# (m = %d, n = %d, k = %d, t = %d) Binary BCH code.}\n", mm, nn_shorten, kk_shorten, tt) ;

    // Read in data stream
    in_count = 0;
    in_codeword = 0;

    in_char = input_data[input_data_index++];
    while (input_data_index <= (ntc_data_size/8))
    {
        in_v = (int)in_char;
        for (i = 7; i >= 0; i--)
        {   if ((int)pow(2,i) & in_v)
                data[in_count] = 1 ;
            else
                data[in_count] = 0 ;

            in_count++;
        }

/*-----------------------------------------------------------------------------
* CJChen1@nuvoton, 2011/1/20, To meet Nuvoton chip design, we have to
*      1. cut input data to several 512/1024 bytes segments;
*      Here, original program cut input data to 512 bytes if kk_shorten=4096 (512 bytes)
*---------------------------------------------------------------------------*/
        if (in_count == kk_shorten)
        {   in_codeword++ ;

#if CFLAG_NTC // CJChen1, for debugging, show data before pad and invert
/*-----------------------------------------------------------------------------
* CJChen1@nuvoton, 2011/1/31, To verify reliability for this program,
*      6. modify output format in order to compare to chip data easier.
*              output raw data (no inverting and padding) and parity with Nuvoton style.
*---------------------------------------------------------------------------*/
            //fprintf(stdout, "show raw data before pad and invert:\n");
            //print_hex_low(kk_shorten, data, stdout);
            //fprintf(stdout, "\n");
#endif
            parallel_encode_bch(input_ra_data) ;

#if CFLAG_NTC   // CJChen1@nuvoton, 2011/1/20, to show data that include padding data
/*-----------------------------------------------------------------------------
* CJChen1@nuvoton, 2011/1/31, To verify reliability for this program,
*      6. modify output format in order to compare to chip data easier.
*              output raw data (no inverting and padding) and parity with Nuvoton style.
*---------------------------------------------------------------------------*/
//            print_hex_low(NTC_DATA_FULL_SIZE, data, stdout);
#else
            print_hex_low(kk_shorten, data, stdout);
#endif
            //fprintf(stdout, "    ");
            //print_hex_low(rr, bb, stdout);
            //fprintf(stdout, "\n") ;
        }

        in_char = input_data[input_data_index++];
    }   // end of while()
    //fprintf(stdout, "\n{### %d words encoded.}\n", in_codeword) ;

    // copy parity code from bb (bit array) to output_bch_parity (byte array)
    output_bch_parity[0] = 0;
    for (i = 0, j = 0; i < rr; i++)
    {
        output_bch_parity[j] = (output_bch_parity[j] << 1) + bb[i];
        if (i % 8 == 7)
        {
            output_bch_parity[++j] = 0;     // initial next byte
        }
    }

    if (rr % 8 == 0)
        return(rr/8);   // return parity code lenght with unit byte
    else
    {
        // if rr cannot dividable by 8, padding bit 0 after it.
        output_bch_parity[j] = (output_bch_parity[j] << (8 - (rr%8)));
        return((rr/8) + 1);
    }
}
