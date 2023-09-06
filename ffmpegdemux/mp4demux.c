#include <stdio.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

int main(int argc, char **argv) {
    AVFormatContext *pFormatCtx = NULL;
    int video_stream_index = -1;
    int audio_stream_index = -1;

    if (argc < 2) {
        printf("Usage: %s <input file>\n", argv[0]);
        return -1;
    }

    // FFmpeg 초기화
    av_register_all();

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


    /**
    AVPacket packet;
    FILE *video_dump = fopen("video_dump.h264", "wb"); // 비디오 덤프 파일 열기
    FILE *audio_dump = fopen("audio_dump.aac", "wb");  // 오디오 덤프 파일 열기
    */

    while (av_read_frame(pFormatCtx, &packet) >= 0) {
        if (packet.stream_index == video_stream_index) {
            // 비디오 패킷 처리
            //fwrite(packet.data, 1, packet.size, video_dump); // 비디오 데이터 파일에 쓰기
            //av_pkt_dump2 (video_dump, &packet, 1, pFormatCtx->streams[video_stream_index]);
            //av_hex_dump (video_dump, packet.data, packet.size);
            //av_interleaved_write_frame (pFormatCtx, &packet); // segment fault - muxer에 사용
            //av_write_frame (pFormatCtx, &packet); // segment fault - muxer에 사용
            //av_dump_format(pFormatCtx, 0, argv[1], 0); // output 파일 X meta data 출력
            
            
            
        } else if (packet.stream_index == audio_stream_index) {
            // 오디오 패킷 처리
            //fwrite(packet.data, 1, packet.size, audio_dump); // 오디오 데이터 파일에 쓰기
            //av_pkt_dump2 (audio_dump, &packet, 1, pFormatCtx->streams[audio_stream_index]);
            //av_hex_dump (audio_dump, packet.data, packet.size);
        }
        av_packet_unref(&packet);
    }

    // 파일과 메모리 정리
    fclose(video_dump);
    fclose(audio_dump);
    avformat_close_input(&pFormatCtx);

    return 0;
}