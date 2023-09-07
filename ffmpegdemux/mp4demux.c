#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

// 디코딩 된 데이터 출력
// data[0]의 linesize[0]만큼만 출력함.. yuv 중 y만 출력함.
// u,v는 data[1], data[2]에 linesize[1], linesize[2]만큼 들어 있음
static void pgm_save(unsigned char *buf, int wrap, int xsize, int ysize, const char *filename) {
    int i;
    filename = "video_dump.h264";
    FILE *video_dump = fopen("video_dump.h264", "w");
// x,y의 사이즈와 255 숫자를 파일에 출력
// yuv 플레이어를 위한 값 같지만, 해당 플레이어를 사용하지 않으면 굳이 출력할 필요 없음
    fprintf(video_dump, "P5\n%d %d\n%d\n", xsize, ysize, 255);

//디코딩된 데이터 출력 , wrap 사이즈 만큼 건너뛰며
// xsize 만큼 출력 함
    for (i = 0; i < ysize; i++)
        fwrite(buf + i * wrap, 1, xsize, video_dump);
    fclose(video_dump);                        
}


static void decode(AVCodecContext *dec_ctx, AVFrame *frame, AVPacket *packet, const char *filename) {
    char buf[1024];
    int ret;

    // 디코딩 명령 수행
    ret = avcodec_send_packet(dec_ctx, packet); //*************************
                
    if (ret < 0) {
        fprintf(stderr, "Error sending a packet for decoding\n");
        exit(1);
    }
                
    while (ret >= 0) {
        // 디코딩 된 데이터 획득
        ret = avcodec_receive_frame(dec_ctx, frame); //***********************
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            fprintf(stderr, "Error during decoding\n");
            exit(1);
        }
        printf("saving frame %4d\n", dec_ctx->frame_number); //********************
        fflush(stdout);

        // the picture is allocated by the decoder. no need to free it
        snprintf(buf, sizeof(buf), "%s-%d", filename, dec_ctx->frame_number); //***************************

        pgm_save(frame->data[0], frame->linesize[0], frame->width, frame->height, buf);
    }
}


int main(int argc, char **argv) {
    AVFormatContext *pFormatCtx = NULL;
    AVCodecContext *pCodecCtx= NULL;

    AVPacket packet;
    AVFrame *frame;

    const AVCodec *pCodecVideo;
    const AVCodec *pCodecAudio;
    
    FILE *video_dump = fopen("video_dump.h264", "wb"); // 비디오 덤프 파일 열기
    FILE *audio_dump = fopen("audio_dump.aac", "wb");  // 오디오 덤프 파일 열기

    int video_stream_index = -1;
    int audio_stream_index = -1;

    if (argc < 2) {
        printf("Usage: %s <input file>\n", argv[0]);
        return -1;
    }

    // FFmpeg 초기화
    av_register_all();
    avcodec_register_all();

    // 파일 열기
    if (avformat_open_input(&pFormatCtx, argv[1], NULL, NULL) != 0)
        return -1;

    // 스트림 정보 얻기
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
        return -1;

    // 스트림들을 순회하며 비디오와 오디오 스트림의 인덱스 찾기
    for (int i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && video_stream_index < 0) {
            video_stream_index = i;
        }
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audio_stream_index < 0) {
            audio_stream_index = i;
        }
    }

    if (video_stream_index == -1 || audio_stream_index == -1) {
        printf("Couldn't find video or audio stream in the input file\n");
        return -1;
    }

    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }

    pCodecVideo = avcodec_find_decoder(pFormatCtx->streams[video_stream_index]->codecpar->codec_id);

    pCodecCtx = avcodec_alloc_context3(pCodecVideo);

    while (av_read_frame(pFormatCtx, &packet) >= 0) {
        if (packet.stream_index == video_stream_index) {
            // 비디오 패킷 처리
            //fwrite(packet.data, 1, packet.size, video_dump); // 비디오 데이터 파일에 쓰기
            //av_pkt_dump2 (video_dump, &packet, 1, pFormatCtx->streams[video_stream_index]);
            //av_hex_dump (video_dump, packet.data, packet.size);
            //av_interleaved_write_frame (pFormatCtx, &packet); // segment fault - muxer에 사용
            //av_write_frame (pFormatCtx, &packet); // segment fault - muxer에 사용
            //av_dump_format(pFormatCtx, 0, argv[1], 0); // output 파일 X meta data 출력
            

            //Todo
            // Do Video decode
            decode(pCodecCtx, frame, &packet, "video_dump_");
            
        } else if (packet.stream_index == audio_stream_index) {
            // 오디오 패킷 처리
            //fwrite(packet.data, 1, packet.size, audio_dump); // 오디오 데이터 파일에 쓰기
            //av_pkt_dump2 (audio_dump, &packet, 1, pFormatCtx->streams[audio_stream_index]);
            //av_hex_dump (audio_dump, packet.data, packet.size);
        }
    }

    // 파일과 메모리 정리
    fclose(video_dump);
    fclose(audio_dump);
    avformat_close_input(&pFormatCtx);
    return 0;
}