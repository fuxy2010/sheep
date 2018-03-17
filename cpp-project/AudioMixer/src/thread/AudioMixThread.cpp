// **********************************************************************
// ×÷Õß: ¸µÓÓÃú
// °æ±¾: 1.0
// ÈÕÆÚ: 2011-01 ~ 2011-11
// ÐÞ¸ÄÀúÊ·¼ÇÂ¼: 
// ÈÕÆÚ, ×÷Õß, ±ä¸üÄÚÈÝ
// **********************************************************************
#include "GeneralDef.h"
#include "AudioMixThread.h"
#include "AudioCodec.h"
#include "ScheduleServer.h"

using namespace ScheduleServer;

void CAudioMixThread::mix()
{
    if(!_next_fetch_audio_frame_timestamp)
		_next_fetch_audio_frame_timestamp = clock();
        
    if(_next_fetch_audio_frame_timestamp > clock()) return;
    
    RAW_AUDIO_FRAME_PTR mix_frame_ptr;
    if(false == CMemPool::malloc_raw_audio_frame(mix_frame_ptr)) return;
    
    for(int i = 1; i <= UA_NUM; ++i)
    {
        if(NULL == mix_frame_ptr.frame) continue;
        
        CUserAgent* ua = SINGLETON(CScheduleServer).fetch_ua(i);        
        if(NULL == ua) continue;
        
        RAW_AUDIO_FRAME_PTR frame_ptr = ua->fetch_audio_frame();        
        if(NULL == frame_ptr.frame) continue;
        
        CAudioCodec::mix(mix_frame_ptr.frame->payload, frame_ptr.frame->payload,
							FRAME_LENGTH_IN_BYTE,//sizeof(mix_frame_ptr.frame->payload),
							1.0,
							1);
                            
        CMemPool::free_raw_audio_frame(frame_ptr);
                            
        //记录各音源收到混音帧对应语音包的时戳
        //mix_frame_ptr.frame->update_mix_timestamp(clock() / 1000);
        mix_frame_ptr.frame->available = true;//混音成功一次即表明该语音帧有效
        
        std::cout << "========= mix and send a audio frame at " << clock() << ", " << CLOCKS_PER_SEC << std::endl;
    }
    
    //如没有非本人的发言人能提供有效的原始语音包则不创建任务
    if(true == mix_frame_ptr.frame->available)
	{
        ::memset(packet, 0, sizeof(packet));
        int packet_len = _codec->encode(mix_frame_ptr.frame->payload, packet);
		if(NULL != _rtp_session) _rtp_session->send_rtp_packet(packet, packet_len, ILBCRTPPacket, true, AUDIO_SAMPLING_RATE, 0);
#if 0
        _pcm_player.play(mix_frame_ptr.frame->payload);
#else
        FILE* f = fopen("./mix.pcm", "ab+");        
        fwrite(mix_frame_ptr.frame->payload, sizeof(short), FRAME_LENGTH_IN_SHORT, f);
        fclose(f);
#endif
	}
    
    CMemPool::free_raw_audio_frame(mix_frame_ptr);
    
    _next_fetch_audio_frame_timestamp += AUDIO_SAMPLING_RATE * 1000;
    
    //_next_fetch_audio_frame_timestamp += 30 * 1000;
}

void* CAudioMixThread::Thread()
{
    JThread::ThreadStarted();
    
    _rtp_session = new CRTPRecvSession(31000);
    _rtp_session->add_dest_addr(_sender_ip.c_str(), 40000);
    
    while(true)
    {
        clock_t loop_start = clock();
        
        mix();
        
        //if(1000 > clock() - loop_start) usleep(2);
    }

}

