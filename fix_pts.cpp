#include "stdio.h"

extern "C"
{
#include "libavcodec\avcodec.h"
#include "libavformat\avformat.h"
#include "libswscale\swscale.h"
};

int main(int argc, _TCHAR* argv[])
{
    AVFormatContext* pFormatCtx;
    AVOutputFormat* fmt;
    AVStream* video_st;
    AVCodecContext* pCodecCtx;
    AVCodec* pCodec;
    
    uint8_t* picture_buf;
    AVFrame* picture;
    int size;
    
    FILE *in_file = fopen("src01_480x272.yuv", "rb");	// ”∆µYUV‘¥Œƒº˛
    int in_w=480,in_h=272;//øÌ∏ﬂ
    int framenum=50;
    const char* out_file = "src01.h264";					// ‰≥ˆŒƒº˛¬∑æ∂
    
    av_register_all();
    //∑Ω∑®1.◊È∫œ π”√º∏∏ˆ∫Ø ˝
    pFormatCtx = avformat_alloc_context();
    //≤¬∏Ò Ω
    fmt = av_guess_format(NULL, out_file, NULL);
    pFormatCtx->oformat = fmt;
    
    //∑Ω∑®2.∏¸º”◊‘∂ØªØ“ª–©
    //avformat_alloc_output_context2(&pFormatCtx, NULL, NULL, out_file);
    //fmt = pFormatCtx->oformat;
    
    
    //◊¢“‚ ‰≥ˆ¬∑æ∂
    if (avio_open(&pFormatCtx->pb,out_file, AVIO_FLAG_READ_WRITE) < 0)
    {
        printf(" ‰≥ˆŒƒº˛¥Úø™ ß∞‹");
        return -1;
    }
    
    video_st = av_new_stream(pFormatCtx, 0);
    if (video_st==NULL)
    {
        return -1;
    }
    pCodecCtx = video_st->codec;
    pCodecCtx->codec_id = fmt->video_codec;
    pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    pCodecCtx->pix_fmt = PIX_FMT_YUV420P;
    pCodecCtx->width = in_w;
    pCodecCtx->height = in_h;
    pCodecCtx->time_base.num = 1;
    pCodecCtx->time_base.den = 25;
    pCodecCtx->bit_rate = 400000;
    pCodecCtx->gop_size=250;
    //H264
    //pCodecCtx->me_range = 16;
    //pCodecCtx->max_qdiff = 4;
    pCodecCtx->qmin = 10;
    pCodecCtx->qmax = 51;
    //pCodecCtx->qcompress = 0.6;
    // ‰≥ˆ∏Ò Ω–≈œ¢
    av_dump_format(pFormatCtx, 0, out_file, 1);
    
    pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
    if (!pCodec)
    {
        printf("√ª”–’“µΩ∫œ  µƒ±‡¬Î∆˜£°\n");
        return -1;
    }
    if (avcodec_open2(pCodecCtx, pCodec,NULL) < 0)
    {
        printf("±‡¬Î∆˜¥Úø™ ß∞‹£°\n");
        return -1;
    }
    picture = avcodec_alloc_frame();
    size = avpicture_get_size(pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
    picture_buf = (uint8_t *)av_malloc(size);
    avpicture_fill((AVPicture *)picture, picture_buf, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
    
    //–¥Œƒº˛Õ∑
    avformat_write_header(pFormatCtx,NULL);
    
    AVPacket pkt;
    int y_size = pCodecCtx->width * pCodecCtx->height;
    av_new_packet(&pkt,y_size*3);
    
    for (int i=0; i<framenum; i++){
        //∂¡»ÎYUV
        if (fread(picture_buf, 1, y_size*3/2, in_file) < 0)
        {
            printf("Œƒº˛∂¡»°¥ÌŒÛ\n");
            return -1;
        }else if(feof(in_file)){
            break;
        }
        picture->data[0] = picture_buf;  // ¡¡∂»Y
        picture->data[1] = picture_buf+ y_size;  // U
        picture->data[2] = picture_buf+ y_size*5/4; // V
        //PTS
        picture->pts=i;
        int got_picture=0;
        //±‡¬Î
        int ret = avcodec_encode_video2(pCodecCtx, &pkt,picture, &got_picture);
        if(ret < 0)
        {
            printf("±‡¬Î¥ÌŒÛ£°\n");
            return -1;
        }
        if (got_picture==1)
        {
            printf("±‡¬Î≥…π¶µ⁄%d÷°£°\n",i);
            pkt.stream_index = video_st->index;
            ret = av_write_frame(pFormatCtx, &pkt);
            av_free_packet(&pkt);
        }
    }
    
    //–¥Œƒº˛Œ≤
    av_write_trailer(pFormatCtx);
    
    //«Â¿Ì
    if (video_st)
    {
        avcodec_close(video_st->codec);
        av_free(picture);
        av_free(picture_buf);
    }
    avio_close(pFormatCtx->pb);
    avformat_free_context(pFormatCtx);
    
    fclose(in_file);
    
    return 0;
}

