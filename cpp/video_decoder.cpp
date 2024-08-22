//
// Created by tang on 2024/6/13.
//
#include <fstream>
#include <string.h>
#include <ANWRender.h>
#include <AAudioRender.h>
#include <mutex>
#include <pthread.h>
#include <aaudio/AAudio.h>
#include <sonic.h>
#include "buffer.h"
extern "C"
{
#include "ffmpeg/include/libswresample/swresample.h"
#include "ffmpeg/include/libavutil/opt.h"
#include "ffmpeg/include/libavcodec/avcodec.h"
#include "ffmpeg/include/libavformat/avformat.h"
#include "ffmpeg/include/libswscale/swscale.h"
#include "ffmpeg/include/libavutil/pixfmt.h"
#include "ffmpeg/include/libavutil/time.h"
#include "ffmpeg/include/libavutil/imgutils.h"
}
#define __STDC_CONSTANT_MACROS
using namespace std;

AVFormatContext *avFormatContext;
AVCodecContext *videoContext;
AVCodecContext *audioContext;

bool isStart; // 开始标志
int videoIndex;// 视频索引
int audioIndex;// 音频索引
QUEUE<AVPacket*> videoQueue;
QUEUE<AVPacket*> audioQueue;
AVFrame *rgbFrame;
int width;
int height;

uint8_t * videoBuffer;
uint8_t * audioBuffer;

mutex videoLock;
mutex audioLock;

SwsContext *swsContext;// 视频转换上下文
SwrContext *swrContext;// 音频转换上下文


ANWRender videoRender(NULL);
AAudioRender audioRender;


bool STOP = false; // 用于停止的标志位

double total_time;
double current_time;
int64_t target_time;

bool isJump;// 是否跳转

using AAudioCallback = int(*)(AAudioStream*, void*, void*, int32_t);

double videoTime;
double audioTime;
double defaultDelay;
double audioNow;
double videoNow;

int size;
bool isPause = true;


void seek(double position)
{
    target_time=(int64_t)(position*total_time*1e6);
    isJump = true;
}

void *decodePacket(void *v) {
    while(true) {
        if(STOP)
        {
            break;
        }
        while (isStart) {
            if(isJump)
            {
                av_seek_frame(avFormatContext,-1,target_time,AVSEEK_FLAG_BACKWARD);
                videoQueue.clear();
                audioQueue.clear();
                isJump = false;
            }
            AVPacket *avPacket = av_packet_alloc();
            int ret = av_read_frame(avFormatContext, avPacket);
            if (ret < 0) {
                break;
            }
            videoLock.lock();
            audioLock.lock();
            if (avPacket->stream_index == videoIndex) {// 视频包

                videoQueue.push(avPacket);

            } else if (avPacket->stream_index == audioIndex) {// 音频包

                audioQueue.push(avPacket);

            }
            videoLock.unlock();
            audioLock.unlock();
        }
    }
    return NULL;
}

void *decodeVideo(void *v)
{
    while(true) {
        if(STOP)
        {
            videoQueue.clear();
            break;
        }
        while (isStart) {
            videoLock.lock();
            if (videoQueue.is_empty() == 1) {
                __android_log_print(ANDROID_LOG_INFO, "videoQueue","%d", videoQueue.size());
                videoLock.unlock();
                continue;
            }
            AVPacket *videoPacket = av_packet_alloc();
            videoPacket = videoQueue.front();
            videoQueue.pop();
            avcodec_send_packet(videoContext, videoPacket);
            AVFrame *videoFrame = av_frame_alloc();
            int ret = avcodec_receive_frame(videoContext, videoFrame);
            if (ret != 0) {
                videoLock.unlock();
                continue;
            }
            double pts = videoFrame->best_effort_timestamp;
            pts = pts * videoTime;
            videoNow = pts;
            double time = defaultDelay;
            double diff = audioNow - videoNow;
            if (diff > 0.003) // 音频超前
            {
                time = defaultDelay * 2 / 3;
            } else if (diff < -0.03) {
                time *= 2;
            }
            av_usleep(time * 1e6);
            sws_scale(swsContext, videoFrame->data, videoFrame->linesize,
                      0, videoContext->height, rgbFrame->data, rgbFrame->linesize);// 转换
            current_time = pts;
            if (current_time >= total_time) {
                STOP = true;
                isStart = false;
                videoLock.unlock();
                continue;
            }
            videoRender.render(videoBuffer);// 渲染

            av_frame_free(&videoFrame);
            av_free(videoFrame);
            videoFrame = NULL;
            av_packet_free(&videoPacket);
            av_free(videoPacket);
            videoPacket = NULL;

            videoLock.unlock();
        }
    }
    return NULL;
}

// 回调函数实现
int audioDataCallback(AAudioStream* stream, void* user_data, void* audioData, int32_t numFrames) {

    memcpy(audioData,audioBuffer,sizeof(audioBuffer)*numFrames);
    if(STOP)
    {
        return 1;
    }
    return 0;
}

void *decodeAudio(void*p)
{
    audioRender.setCallback(audioDataCallback,audioBuffer);
    audioRender.start();
    audioRender.flush(); // 刷新
    while(true) {
        if (STOP) {
            audioRender.flush(); // 刷新
            audioQueue.clear();
            break;
        }
        if(!isStart&&isPause)
        {
            audioRender.pause(true);
            isPause = false;
        }
        if(isStart&&!isPause)
        {
            audioRender.pause(false);
            isPause = true;
        }
        while (isStart) {
            if (audioQueue.is_empty() == 1) {
                continue;
            }
            do {
                if (isJump) {
                    audioRender.flush(); // 刷新
                    audioLock.unlock();
                    break;
                }
                audioLock.lock();
                AVPacket *audioPacket = av_packet_alloc();
                audioPacket = audioQueue.front();
                audioQueue.pop();
                int ret = avcodec_send_packet(audioContext, audioPacket);
                if(ret < 0 &&ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
                {
                    audioLock.unlock();
                    continue;
                }
                AVFrame *audioFrame = av_frame_alloc();
                ret = avcodec_receive_frame(audioContext, audioFrame);
                if (ret != 0) {
                    audioLock.unlock();
                    continue;
                }
                swr_convert(swrContext, &audioBuffer, 44100 * 2,
                            (const uint8_t **) audioFrame->data,
                            audioFrame->nb_samples);

                audioNow = audioFrame->pts*audioTime;
                av_frame_free(&audioFrame);
                av_free(audioFrame);
                audioFrame = NULL;
                av_packet_free(&audioPacket);
                av_free(audioPacket);
                audioPacket = NULL;

                audioLock.unlock();
            }while(false);
        }
    }
    return NULL;
}

void play(ANativeWindow * nativeWindow,char *url)
{
    ANWRender anwRender(nativeWindow);
    videoRender=anwRender;
    int i;

    avFormatContext=avformat_alloc_context();

    if(avformat_open_input(&avFormatContext,url,NULL,NULL)!=0){//打开输入的视频文件
        __android_log_print(ANDROID_LOG_ERROR, "native-log", "Couldn't open input stream.");
    }
    avformat_find_stream_info(avFormatContext,NULL);
    for(i=0;i<avFormatContext->nb_streams;++i) {
        if (avFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {// 视频
            videoIndex = i;
            AVCodecParameters *parameters = avFormatContext->streams[i]->codecpar;// 参数
            AVCodec *VCodec = avcodec_find_decoder(AV_CODEC_ID_H264);// 解码器
            videoContext = avcodec_alloc_context3(VCodec);// 解码器上下文
            avcodec_parameters_to_context(videoContext, parameters);// 为解码器上下文设置参数
            avcodec_open2(videoContext, VCodec, 0);// 开始解码

            AVRational avRational = avFormatContext->streams[i]->time_base;
            videoTime = (double)avRational.num/(double)avRational.den;
            int num = avFormatContext->streams[i]->avg_frame_rate.num;
            int den = avFormatContext->streams[i]->avg_frame_rate.den;
            double fps = (double)num/(double)den;
            defaultDelay = 1.0/fps;// 算出的一帧的时间 1/24 实际上可以直接通过视频信息查询

        }
        else if (avFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {// 音频
            audioIndex = i;
            AVCodecParameters *parameters = avFormatContext->streams[i]->codecpar;// 参数
            AVCodec *ACodec = avcodec_find_decoder(parameters->codec_id);// 解码器
            audioContext = avcodec_alloc_context3(ACodec);// 解码器上下文
            avcodec_parameters_to_context(audioContext, parameters);// 为解码器上下文设置参数
            avcodec_open2(audioContext, ACodec, 0);// 开始解码

            AVRational avRational = avFormatContext->streams[i]->time_base;
            audioTime = (double)avRational.num/(double)avRational.den;
        }
    }
    // video 设置
    width = videoContext->width;
    height = videoContext->height;

    rgbFrame = av_frame_alloc();
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA,width,height,1);
    videoBuffer=(uint8_t*)av_malloc(numBytes*sizeof(uint8_t));
    av_image_fill_arrays(rgbFrame->data,rgbFrame->linesize,videoBuffer,AV_PIX_FMT_RGBA,width,height,1);// 关联videoBuffer

    swsContext = sws_getContext(
            width, height, videoContext->pix_fmt, width, height,
            AV_PIX_FMT_RGBA, SWS_BICUBIC,NULL,NULL,NULL
                                    ); // 设置上下文

    videoRender.init(width,height); // 初始化渲染的视频宽和高
    total_time = (double)avFormatContext->duration/1e6;

    audioBuffer=(uint8_t*)av_malloc(44100*2);
    swrContext=swr_alloc();
    swr_alloc_set_opts(
            swrContext,AV_CH_LAYOUT_STEREO,AV_SAMPLE_FMT_S16,audioContext->sample_rate,
            audioContext->channel_layout,audioContext->sample_fmt,audioContext->sample_rate,
            0,NULL
            );// 设置上下文
    swr_init(swrContext);

    isStart = true; // 开始标志置true
    isJump = false; // 跳转标志置false
    // 多线程  读取 音频 视频三线程
    pthread_t thread_packet;
    pthread_t thread_audio;
    pthread_t thread_video;
    // 读取线程
    pthread_create(&thread_packet,NULL,decodePacket,NULL);
    // 音频
    pthread_create(&thread_audio,NULL,decodeAudio,NULL);
    // 视频
    pthread_create(&thread_video,NULL,decodeVideo,NULL);
}