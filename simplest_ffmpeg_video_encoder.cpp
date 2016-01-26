#include <stdio.h>

#define __STDC_CONSTANT_MACROS

#ifdef _WIN32
//Windows
extern "C"
{
#include <math.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>
};
#else
//Linux...
#ifdef __cplusplus
extern "C"
{
#endif
#include <math.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>
#ifdef __cplusplus
};
#endif
#endif

struct Encoderinfo
{
    AVFormatContext* pFormatCtx;
    AVStream* video_st;
    AVCodecContext* pCodecCtx;
    AVPacket pkt;
    // uint8_t* picture_buf;
    AVFrame* pFrame;
    // int picture_size;
    int y_size;
    int framecnt;
    int frameindex;
    int in_w;
    int in_h;
};

static int flush_encoder(AVFormatContext *fmt_ctx, unsigned int stream_index) {
    int ret;
    int got_frame;
    AVPacket enc_pkt;
    if (!(fmt_ctx->streams[stream_index]->codec->codec->capabilities &
            CODEC_CAP_DELAY))
        return 0;
    while (1) {
        enc_pkt.data = NULL;
        enc_pkt.size = 0;
        av_init_packet(&enc_pkt);
        ret = avcodec_encode_video2 (fmt_ctx->streams[stream_index]->codec, &enc_pkt,
                                     NULL, &got_frame);
        av_frame_free(NULL);
        if (ret < 0)
            break;
        if (!got_frame) {
            ret = 0;
            break;
        }
        printf("Flush Encoder: Succeed to encode 1 frame!\tsize:%5d\n", enc_pkt.size);
        /* mux encoded frame */
        ret = av_write_frame(fmt_ctx, &enc_pkt);
        if (ret < 0)
            break;
    }
    return ret;
}

struct Encoderinfo *
cv_finance_create_encoder() {
    Encoderinfo *h;
    h = (Encoderinfo *)malloc(sizeof( Encoderinfo));
    return h;
}

int  cv_finance_encoder_video_input_begin(
    struct Encoderinfo* encoder_handle,
    const char* filename, //where you want to store
    enum AVPixelFormat pixel_format,   //picture pix_format
    int image_width,
    int image_height
    // int image_stride
) {
    if (pixel_format != AV_PIX_FMT_YUV420P)
    {
        //convert to YUV420P
    }       
    AVOutputFormat* fmt;
    AVCodec* pCodec;
    // set Params
    encoder_handle->in_w = image_width;
    encoder_handle->in_h = image_height;
    encoder_handle->framecnt = 0;
    encoder_handle->frameindex = 0;
    av_register_all();
    encoder_handle->pFormatCtx = avformat_alloc_context();
    //Guess Format
    fmt = av_guess_format(NULL, filename, NULL);
    encoder_handle->pFormatCtx->oformat = fmt;
    //Open output URL
    if (avio_open(&(encoder_handle->pFormatCtx->pb), filename, AVIO_FLAG_READ_WRITE) < 0) {
        printf("Failed to open output file! \n");
        return -1;
    }

    encoder_handle->video_st = avformat_new_stream(encoder_handle->pFormatCtx, 0);

    encoder_handle->video_st->time_base.num = 1;
    encoder_handle->video_st->time_base.den = 10;

    if (encoder_handle->video_st == NULL) {

        return -1;
    }
   
    encoder_handle->pCodecCtx = encoder_handle->video_st->codec;
    //Must Param
    encoder_handle->pCodecCtx->codec_id = fmt->video_codec;
    encoder_handle->pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    encoder_handle->pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    encoder_handle->pCodecCtx->width = encoder_handle->in_w;
    encoder_handle->pCodecCtx->height = encoder_handle->in_h;
    encoder_handle->pCodecCtx->time_base.num = 1;
    encoder_handle->pCodecCtx->time_base.den = 10;
    encoder_handle->pCodecCtx->bit_rate = 1600;
    encoder_handle->pCodecCtx->gop_size = 250;
    encoder_handle->pCodecCtx->qmin = 10;
    encoder_handle->pCodecCtx->qmax = 51;
  
    //Optional Param
    encoder_handle->pCodecCtx->max_b_frames = 3;
// printf("147%d\n", AV_PIX_FMT_YUV420P);
    //Set Option;local var
    AVDictionary *param = 0;
    //H264
    if (encoder_handle->pCodecCtx->codec_id == AV_CODEC_ID_H264) {
        av_dict_set(&param, "preset", "slow", 0);
        av_dict_set(&param, "tune", "zerolatency", 0);
        // printf("153\n");
    }
     //av_dump_format(encoder_handle->pFormatCtx, 0, filename, 1);

    //open codec
    pCodec = avcodec_find_encoder(encoder_handle->pCodecCtx->codec_id);
    if (!pCodec) {
        printf("Can not find encoder! \n");
        return -1;
    }

    if (avcodec_open2(encoder_handle->pCodecCtx, pCodec, &param) < 0) {
        printf("Failed to open encoder! \n");
        return -1;
    }
    int ret;
    //initial frame
    int picture_size;
    encoder_handle->pFrame = av_frame_alloc();
    encoder_handle->pFrame->format = encoder_handle->pCodecCtx->pix_fmt;
    encoder_handle->pFrame->width  = encoder_handle->pCodecCtx->width;
    encoder_handle->pFrame->height = encoder_handle->pCodecCtx->height;
    picture_size=av_image_get_buffer_size(encoder_handle->pCodecCtx->pix_fmt, encoder_handle->pCodecCtx->width, encoder_handle->pCodecCtx->height,1);
    printf("%d:%d\n", __LINE__,picture_size);
    ret = av_image_alloc(encoder_handle->pFrame->data, encoder_handle->pFrame->linesize, encoder_handle->pCodecCtx->width, encoder_handle->pCodecCtx->height,
                         encoder_handle->pCodecCtx->pix_fmt,1);
    // picture_size = avpicture_get_size(encoder_handle->pCodecCtx->pix_fmt, encoder_handle->pCodecCtx->width, encoder_handle->pCodecCtx->height);
    // uint8_t* picture_buf = (uint8_t *)av_malloc(picture_size);
    // avpicture_fill((AVPicture *)encoder_handle->pFrame, picture_buf, encoder_handle->pCodecCtx->pix_fmt, encoder_handle->pCodecCtx->width, encoder_handle->pCodecCtx->height);

    //Write File Header
    ret= avformat_write_header(encoder_handle->pFormatCtx, NULL);
    if (ret < 0) {
        printf("Error occurred when opening output file!\n");
        return 1;
    }
    av_new_packet(&encoder_handle->pkt, picture_size);
    encoder_handle->y_size = encoder_handle->pCodecCtx->width * encoder_handle->pCodecCtx->height;

    return 0;
}


int  cv_finance_encoder_video_input_frame(
    struct Encoderinfo* encoder_handle,
     unsigned char* image
    // ,int timestamp
) {
    //read YUV
    encoder_handle->pFrame->data[0] = image;              // Y
    encoder_handle->pFrame->data[1] = image + encoder_handle->y_size;     // U
    encoder_handle->pFrame->data[2] = image + encoder_handle->y_size * 5 / 4; // V

    //PTS
    encoder_handle->pFrame->pts = encoder_handle->frameindex;
    int got_picture = 0;

    //Encode
    int ret = avcodec_encode_video2(encoder_handle->pCodecCtx, &encoder_handle->pkt, encoder_handle->pFrame, &got_picture);
    if (ret < 0) {
        printf("Failed to encode! \n");
        return -1;
    }
    if (got_picture == 1) {
        printf("Succeed to encode frame: %5d\tsize:%5d\n", encoder_handle->framecnt, (encoder_handle->pkt).size);
        encoder_handle->framecnt++;
        encoder_handle->pkt.stream_index = encoder_handle->video_st->index;
        ret = av_write_frame(encoder_handle->pFormatCtx, &encoder_handle->pkt);
        av_packet_unref(&encoder_handle->pkt);
    }
    encoder_handle->frameindex++;

    return 0;
}


int  cv_finance_encoder_video_input_end(
    struct Encoderinfo* encoder_handle
) {
    //handle delay
    int ret = flush_encoder(encoder_handle->pFormatCtx, 0);
    if (ret < 0) {
        printf("Flushing encoder failed\n");
        return -1;
    }
    //Write file trailer
    av_write_trailer(encoder_handle->pFormatCtx);
    //Clean
    if (encoder_handle->video_st) {
        avcodec_close(encoder_handle->video_st->codec);
        av_free(encoder_handle->pFrame);
        // av_free(encoder_handle->picture_buf);
    }
    avio_close(encoder_handle->pFormatCtx->pb);
    avformat_free_context(encoder_handle->pFormatCtx);
    return 0;
}


// int  cv_finance_encoder_get_result(
//     Encoderinfo* encoder_handle,
//     unsigned char* output_video,
//     int* output_length
// ) {
//     return 0;
// }
// int  cv_finance_encoder_release_result(
//      Encoderinfo* encoder_handle,
//     unsigned char* output_video,
//     int* output_length
// ) {
//     return 0;
// }


int main() {
    FILE *in_file = fopen("/Users/yangyixin/Documents/ds_480x272.yuv", "rb");
    int size = 480 * 272;
    printf("%d\n",size*3/2);
    unsigned char* image;
    struct Encoderinfo * h = cv_finance_create_encoder();
    cv_finance_encoder_video_input_begin(h, "/Users/yangyixin/Documents/test.mp4", AV_PIX_FMT_YUV420P, 480, 272);
    int picture_size =  avpicture_get_size(AV_PIX_FMT_YUV420P, 480, 272);
    printf("%d:%d\n",__LINE__,picture_size );
    image = (unsigned char*)av_malloc(picture_size); //to get the YUV data
    int i = 0;
    
    for (i = 0; i < 100; i++) {

        if (fread(image, 1, size * 3 / 2, in_file) <= 0) {
            printf("Failed to read raw data! \n");
            return -1;
        } else if (feof(in_file)) {
            break;
        }
       // until now we've get YUV data
        cv_finance_encoder_video_input_frame(h, image);
    }
    cv_finance_encoder_video_input_end(h);
    return 0;
}
