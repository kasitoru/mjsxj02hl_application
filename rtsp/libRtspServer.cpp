#include <stdint.h>

#include "libRtspServer.h"
#include "xop/RtspServer.h"
#include "xop/H264Parser.h"
#include "../localsdk/localsdk.h"

static std::shared_ptr<xop::EventLoop> event_loop(new xop::EventLoop());
static std::shared_ptr<xop::RtspServer> rtsp_server;
static xop::MediaSessionId primary_session_id;
static xop::MediaSessionId secondary_session_id;

// Create RTSP server
bool rtspserver_create(uint16_t port, bool multicast, uint8_t video_type, uint32_t framerate) {
    rtsp_server = xop::RtspServer::Create(event_loop.get());
	if(rtsp_server->Start("0.0.0.0", port)) {
	    
	    // Primary channel
	    xop::MediaSession *primary_session = xop::MediaSession::CreateNew("primary");
	    if(video_type == LOCALSDK_VIDEO_PAYLOAD_H264) {
	        primary_session->AddSource(xop::channel_0, xop::H264Source::CreateNew(framerate));
	    } else {
	        primary_session->AddSource(xop::channel_0, xop::H265Source::CreateNew(framerate));
	    }
	    primary_session->AddSource(xop::channel_1, xop::G711ASource::CreateNew());
	    if(multicast) { primary_session->StartMulticast(); }
        primary_session->AddNotifyConnectedCallback([] (xop::MediaSessionId session_id, std::string peer_ip, uint16_t peer_port) {
            printf("[libRtspServer]: Client connect to %s channel, ip=%s, port=%hu\n", "primary", peer_ip.c_str(), peer_port);
        });
        primary_session->AddNotifyDisconnectedCallback([](xop::MediaSessionId session_id, std::string peer_ip, uint16_t peer_port) {
            printf("[libRtspServer]: Client disconnect from %s channel, ip=%s, port=%hu\n", "primary", peer_ip.c_str(), peer_port);
        });
	    primary_session_id = rtsp_server->AddSession(primary_session);
        
        // Secondary channel
	    xop::MediaSession *secondary_session = xop::MediaSession::CreateNew("secondary");
	    if(video_type == LOCALSDK_VIDEO_PAYLOAD_H264) {
	        secondary_session->AddSource(xop::channel_0, xop::H264Source::CreateNew(framerate));
	    } else {
	        secondary_session->AddSource(xop::channel_0, xop::H265Source::CreateNew(framerate));
	    }
	    secondary_session->AddSource(xop::channel_1, xop::G711ASource::CreateNew());
	    if(multicast) { secondary_session->StartMulticast(); }
        secondary_session->AddNotifyConnectedCallback([] (xop::MediaSessionId session_id, std::string peer_ip, uint16_t peer_port) {
            printf("[libRtspServer]: Client connect to %s channel, ip=%s, port=%hu\n", "secondary", peer_ip.c_str(), peer_port);
        });
        secondary_session->AddNotifyDisconnectedCallback([](xop::MediaSessionId session_id, std::string peer_ip, uint16_t peer_port) {
            printf("[libRtspServer]: Client disconnect from %s channel, ip=%s, port=%hu\n", "secondary", peer_ip.c_str(), peer_port);
        });
	    secondary_session_id = rtsp_server->AddSession(secondary_session);

		return true;
	}
	return false;
}

// Get primary session id
uint32_t rtspserver_primary_id() {
    return primary_session_id;
}

// Get secondary session id
uint32_t rtspserver_secondary_id() {
    return secondary_session_id;
}

// Timestamp for H264
uint32_t rtspserver_timestamp_h264() {
    return xop::H264Source::GetTimestamp();
}

// Timestamp for H265
uint32_t rtspserver_timestamp_h265() {
    return xop::H265Source::GetTimestamp();
}

// Timestamp for G711A
uint32_t rtspserver_timestamp_g711a() {
    return xop::G711ASource::GetTimestamp();
}

// Send media frame
bool rtspserver_frame(uint32_t session_id, signed char *data, uint8_t type, uint32_t size, uint32_t timestamp) {
    xop::Nal nal;
    xop::AVFrame frame = {0};
    frame.type = type;
    frame.timestamp = timestamp;
    
    uint32_t endpoint = ((uint32_t) data) + size;
    while(true) {
        
        // Divide the video package into separate frames
        if(frame.type != xop::AUDIO_FRAME) {
            nal = xop::H264Parser::findNal((const uint8_t*) data, size);
            data = (signed char *) nal.first;
            size = ((uint32_t) nal.second) - ((uint32_t) nal.first) + 1;
        }
        
        // Send current frame
        if(data) {
            frame.size = size;
            frame.buffer.reset(new uint8_t[frame.size]);
            memcpy(frame.buffer.get(), data, frame.size);
            rtsp_server->PushFrame(session_id, (frame.type != xop::AUDIO_FRAME ? xop::channel_0 : xop::channel_1), frame);
            if(frame.type != xop::AUDIO_FRAME) {
                data = (signed char *) nal.second;
                size = endpoint - ((uint32_t) nal.second) + 1;
            } else break;
        } else break;
    }
    
    return true;
}

// Free RTSP server
bool rtspserver_free() {
    rtsp_server->RemoveSession(primary_session_id);
    rtsp_server->RemoveSession(secondary_session_id);
    rtsp_server->Stop();
    event_loop->Quit();
    return true;
}

