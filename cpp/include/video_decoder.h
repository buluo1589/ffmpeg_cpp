//
// Created by tang on 2024/6/13.
//

#ifndef ANDROIDPLAYER_VIDEO_DECODER_H
#define ANDROIDPLAYER_VIDEO_DECODER_H
void play(ANativeWindow * nativeWindow1,char *url);
void seek(double position);
extern bool isStart;
extern double total_time;
extern double current_time;
extern bool STOP;
#endif //ANDROIDPLAYER_VIDEO_DECODER_H
