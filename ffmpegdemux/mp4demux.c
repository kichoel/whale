#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

static FILE *pVideo_dump = NULL;
static FILE *pAudio_dump = NULL;

static void yuv_save(unsigned char *pY, int y_wrap, 
                     unsigned char *pU, int u_wrap, 
                     unsigned char *pV, int v_wrap, 
                     int xsize, int ysize) 
{
    int i;
    if (!pVideo_dump) {
        fprintf(stderr, "Could not open dump File\n");
        exit(1);
    }

    // Y 데이터 저장
    for (i = 0; i < ysize; i++) {
        fwrite(pY + i * y_wrap, 1, xsize, pVideo_dump);
    }
    // U 데이터 저장
    for (i = 0; i < ysize / 2; i++) {
        fwrite(pU + i * u_wrap, 1, xsize / 2, pVideo_dump);
    }
    // V 데이터 저장
    for (i = 0; i < ysize / 2; i++) {
        fwrite(pV + i * v_wrap, 1, xsize / 2, pVideo_dump);
    }
}

static void V_decode(AVCodecContext *pVDec_ctx, AVFrame *pVFrame, AVPacket *pPacket) {
    int ret;

    // 디코딩 명령 수행. avcodec_open2()으로 AVCodecContext 열려있어야 함
    ret = avcodec_send_packet(pVDec_ctx, pPacket);
    if (ret < 0) {
        fprintf(stderr, "Error sending a packet for video decoding\n");
        exit(1);
    }

    while (ret >= 0) {
        ret = avcodec_receive_frame(pVDec_ctx, pVFrame); // 디코딩 된 데이터 획득

        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            fprintf(stderr, "Error during video decoding\n");
            exit(1);
        }
        fflush(stdout); // 표준 출력 버퍼 비움

        // 디코딩 된 데이터 저장
        yuv_save(pVFrame->data[0], pVFrame->linesize[0], 
         pVFrame->data[1], pVFrame->linesize[1],
         pVFrame->data[2], pVFrame->linesize[2],
         pVFrame->width, pVFrame->height);
    }
}

static void A_decode(AVCodecContext *pADec_ctx, AVFrame *pAFrame, AVPacket *pPacket) {
    int ret ;
    int Adata_size;

    ret = avcodec_send_packet(pADec_ctx, pPacket);
    if (ret < 0) {
        fprintf(stderr, "Error sending a packet for audio decoding\n");
        exit(1);
    }

    while (ret >= 0) {
        ret = avcodec_receive_frame(pADec_ctx, pAFrame);

        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return;
        }

        else if (ret < 0) {
            fprintf(stderr, "Error during audio decoding\n");
            exit(1);
        }
        
        Adata_size = av_get_bytes_per_sample(pADec_ctx->sample_fmt);
        if (Adata_size < 0) {
             /* This should not occur, checking just for paranoia */
             fprintf(stderr, "Failed to calculate data size\n");
             exit(1);
        }

        for (int i = 0; i < pAFrame->nb_samples; i++) {
            for (int ch = 0; ch < pADec_ctx->channels; ch++) {
                fwrite(pAFrame->data[ch] + Adata_size * i, 1, Adata_size, pAudio_dump);
            }
        }
    }
}

int main(int argc, char **argv) {
    const AVCodec *pVideoCodec;
    const AVCodec *pAudioCodec;

    AVFormatContext *pFormatCtx = NULL;
    AVCodecContext *pVCodecCtx = NULL;
    AVCodecContext *pACodecCtx = NULL;
    AVStream *video_stream = NULL;
    AVStream *audio_stream = NULL;
    AVPacket packet;
    AVFrame *pVFrame;
    AVFrame *pAFrame;

    int video_stream_index = -1;
    int audio_stream_index = -1;

    if (argc < 2) {
        printf("Usage: %s <input file>\n", argv[0]);
        return -1;
    }

    // FFmpeg 초기화
    av_register_all();
    // 디코딩을 위해 모든 코덱 등록
    avcodec_register_all();

    // 파일 열기
    if (avformat_open_input(&pFormatCtx, argv[1], NULL, NULL) != 0)
        return -1;

    // 스트림 정보 얻기
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        return -1;
    }

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

    pVFrame = av_frame_alloc();  // AVFrame 크기의 메모리 할당
    if (!pVFrame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }

    pAFrame = av_frame_alloc();
    if (!pAFrame) {
        fprintf(stderr, "Could not allocate audio frame\n");
        exit(1);
    }

    pVideoCodec = avcodec_find_decoder(pFormatCtx->streams[video_stream_index]->codecpar->codec_id);    // 비디오 코덱 찾기
    pAudioCodec = avcodec_find_decoder(pFormatCtx->streams[audio_stream_index]->codecpar->codec_id);    // 오디오 코덱 찾기
    pVCodecCtx = avcodec_alloc_context3(pVideoCodec);
    pACodecCtx = avcodec_alloc_context3(pAudioCodec);
    video_stream = pFormatCtx->streams[video_stream_index];
    audio_stream = pFormatCtx->streams[audio_stream_index];

    // 동영상 파일 정보를 Ctx에 복사하고 없는 정보는 코덱 정보로 유지
    avcodec_parameters_to_context(pVCodecCtx, video_stream->codecpar);
    avcodec_parameters_to_context(pACodecCtx, audio_stream->codecpar);

    // extra data 복사
    if (video_stream->codecpar->extradata_size > 0) {
        pVCodecCtx->extradata = av_mallocz(video_stream->codecpar->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE);

        if (!pVCodecCtx->extradata) {
            fprintf(stderr, "Could not allocate video extradata\n");
            exit(1);
        }

        // 메모리 값 복사. 1 복사받을 메모리 포인터, 2 복사할 메모리 포인터, 3 복사할 값의 길이 (byte)
        memcpy(pVCodecCtx->extradata, video_stream->codecpar->extradata, video_stream->codecpar->extradata_size);
        pVCodecCtx->extradata_size = video_stream->codecpar->extradata_size;
    }

    avcodec_open2(pVCodecCtx, pVideoCodec, NULL); // 비디오 코덱 열기
    avcodec_open2(pACodecCtx, pAudioCodec, NULL); // 오디오 코덱 열기
    
    pVideo_dump = fopen("video_dump.yuv", "wb");
    pAudio_dump = fopen("audio_dump.pcm", "wb");

    while (av_read_frame(pFormatCtx, &packet) >= 0 ) {
        if (packet.stream_index == audio_stream_index) {
            // 오디오 패킷 처리        
            A_decode(pACodecCtx, pAFrame, &packet);

        } else if (packet.stream_index == video_stream_index) {
            // 비디오 패킷 처리
            V_decode(pVCodecCtx, pVFrame, &packet);
        }
    }

    // 파일과 메모리 정리
    fclose(pVideo_dump);
    fclose(pAudio_dump);
    avformat_close_input(&pFormatCtx);
    avcodec_free_context(&pVCodecCtx);
    avcodec_free_context(&pACodecCtx);
    av_frame_free(&pVFrame);
    av_frame_free(&pAFrame);
    return 0;
}