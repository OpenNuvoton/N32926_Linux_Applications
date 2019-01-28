#ifndef _H264_DEFINE_H_
#define _H264_DEFINE_H_


//#define USE_MMAP

// Encode source from File
#define TEST_IMG1_WIDTH  176 //720
#define TEST_IMG1_HEIGHT 144 //480

// Encode source from sensor
#define TEST_IMG2_WIDTH  320 //720
#define TEST_IMG2_HEIGHT 240 //480

//-------------------------------------
// Decoder part
//-------------------------------------
#define FAVC_DECODER_DEV  "/dev/w55fa92_264dec"

#define FRAME_COUNT		300 //17 //300
#define OUTPUT_FMT     OUTPUT_FMT_YUV422 //OUTPUT_FMT_YUV420

//-------------------------------------
// Encoder part
//-------------------------------------
#define FAVC_ENCODER_DEV  "/dev/w55fa92_264enc"

#define RATE_CTL

#define TEST_ROUND	300
#define FIX_QUANT	0
#define IPInterval	31

//-------------------------------------
// Data structure
//-------------------------------------
typedef struct AVFrame {
    uint8_t *data[4];
} AVFrame;

typedef struct video_profile
{
    unsigned int bit_rate;
    unsigned int width;   //length per dma buffer
    unsigned int height;
    unsigned int framerate;
    unsigned int frame_rate_base;
    unsigned int gop_size;
    unsigned int qmax;
    unsigned int qmin;
    unsigned int quant;
    unsigned int intra;
    AVFrame *coded_frame;
    char *priv;
} video_profile;

#endif //#ifndef _H264_DEFINE_H_
