#include <stdint.h>
#include <stdarg.h>

#include "libRtspServer.h"
#include "xop/RtspServer.h"
#include "xop/H264Parser.h"

static std::shared_ptr<xop::EventLoop> event_loop(new xop::EventLoop());
static std::shared_ptr<xop::RtspServer> rtsp_server;

// Default log printf function
static int logprintf_default(const char *format, ...) {
    int result = 0;
    char *message;
    va_list params;
    va_start(params, format);
    if(vasprintf(&message, format, params) > 0) {
        result = printf("[rtspserver]: %s\n", message);
        free(message);
    }
    va_end(params);
    return result;
}
static int (*logprintf_function)(const char*, ...) = logprintf_default;

// Default connected callback function
static void connected_default(uint32_t session_id, const char *peer_ip, uint16_t peer_port) { }
static void (*connected_function)(uint32_t session_id, const char *peer_ip, uint16_t peer_port) = connected_default;

// Default disconnected callback function
static void disconnected_default(uint32_t session_id, const char *peer_ip, uint16_t peer_port) { }
static void (*disconnected_function)(uint32_t session_id, const char *peer_ip, uint16_t peer_port) = disconnected_default;

// Set log printf function
bool rtspserver_logprintf(int (*function)(const char*, ...)) {
    logprintf_function = function;
    return true;
}

// Set connected callback function
bool rtspserver_connected(void (*function)(uint32_t session_id, const char *peer_ip, uint16_t peer_port)) {
    connected_function = function;
    return true;
}

// Set disconnected callback function
bool rtspserver_disconnected(void (*function)(uint32_t session_id, const char *peer_ip, uint16_t peer_port)) {
    disconnected_function = function;
    return true;
}

// Create RTSP server
bool rtspserver_create(uint16_t port, char *username, char *password) {
    rtsp_server = xop::RtspServer::Create(event_loop.get());
	if(rtsp_server->Start("0.0.0.0", port)) {
	    logprintf_function("The RTSP server is running on port %d.", port);
	    // Digest authentication
	    if(username && username[0]) {
	        rtsp_server->SetAuthConfig("RTSP", std::string(username), std::string(password));
	        logprintf_function("Digest authentication is enabled.");
	    }
		return true;
	}
	logprintf_function("RTSP server startup error! Port %d is busy?", port);
	return false;
}

// Create new session
uint32_t rtspserver_session(char *name, bool multicast, uint8_t video_type, uint32_t framerate, uint8_t audio_type, uint32_t samplerate, uint32_t channels, bool has_adts) {
    if(!rtsp_server) { return 0; }
    logprintf_function("A new multimedia session \"%s\" has been created.", name);
    xop::MediaSession *session = xop::MediaSession::CreateNew(std::string(name));
    // Video
    switch(video_type) {
        case LIBRTSPSERVER_TYPE_H264:
            session->AddSource(xop::channel_0, xop::H264Source::CreateNew(framerate));
            logprintf_function("%s source is %s for session \"%s\" (channel = %d).", "H264", "enabled", name, xop::channel_0);
            break;
        case LIBRTSPSERVER_TYPE_H265:
            session->AddSource(xop::channel_0, xop::H265Source::CreateNew(framerate));
            logprintf_function("%s source is %s for session \"%s\" (channel = %d).", "H265", "enabled", name, xop::channel_0);
            break;
        case LIBRTSPSERVER_TYPE_NONE:
            logprintf_function("%s source is %s for session \"%s\" (channel = %d).", "Video", "disabled", name, xop::channel_0);
            break;
        default:
            logprintf_function("%s source is %s for session \"%s\" (channel = %d).", "Video", "unknown", name, xop::channel_0);
    }
    // Audio
    switch(audio_type) {
        case LIBRTSPSERVER_TYPE_AAC:
            session->AddSource(xop::channel_1, xop::AACSource::CreateNew(samplerate, channels, has_adts));
            logprintf_function("%s source is %s for session \"%s\" (channel = %d).", "AAC", "enabled", name, xop::channel_1);
            break;
        case LIBRTSPSERVER_TYPE_G711A:
            session->AddSource(xop::channel_1, xop::G711ASource::CreateNew());
            logprintf_function("%s source is %s for session \"%s\" (channel = %d).", "G711A", "enabled", name, xop::channel_1);
            break;
        case LIBRTSPSERVER_TYPE_NONE:
            logprintf_function("%s source is %s for session \"%s\" (channel = %d).", "Audio", "disabled", name, xop::channel_1);
            break;
        default:
            logprintf_function("%s source is %s for session \"%s\" (channel = %d).", "Audio", "unknown", name, xop::channel_1);
    }
    // Multicast
    if(multicast) {
        session->StartMulticast();
        logprintf_function("Multicast is enabled for session \"%s\".", name);
    }
    // Callbacks
    session->AddNotifyConnectedCallback([] (xop::MediaSessionId session_id, std::string peer_ip, uint16_t peer_port) {
        logprintf_function("Client connected to media session #%d (IP = %s, port = %hu).", session_id, peer_ip.c_str(), peer_port);
        connected_function(session_id, peer_ip.c_str(), peer_port);
    });
    session->AddNotifyDisconnectedCallback([](xop::MediaSessionId session_id, std::string peer_ip, uint16_t peer_port) {
        logprintf_function("Client disconnected from media session #%d (IP = %s, port = %hu).", session_id, peer_ip.c_str(), peer_port);
        disconnected_function(session_id, peer_ip.c_str(), peer_port);
    });
    // Done
    xop::MediaSessionId session_id = rtsp_server->AddSession(session);
    logprintf_function("Media session \"%s\" was started with ID = %d.", name, session_id);
    return session_id;
}

// Get current timestamp
uint32_t rtspserver_timestamp(uint8_t source, uint32_t samplerate) {
    switch(source) {
        case LIBRTSPSERVER_TYPE_H264:  return xop::H264Source::GetTimestamp();
        case LIBRTSPSERVER_TYPE_H265:  return xop::H265Source::GetTimestamp();
        case LIBRTSPSERVER_TYPE_AAC:   return xop::AACSource::GetTimestamp(samplerate);
        case LIBRTSPSERVER_TYPE_G711A: return xop::G711ASource::GetTimestamp();
        default: return 0;
    }
}

// Send media frame
bool rtspserver_frame(uint32_t session_id, signed char *data, uint8_t type, uint32_t size, uint32_t timestamp, bool split_iframes) {
    if(!rtsp_server) { return false; }
    xop::AVFrame frame = {0};
    frame.type = type;
    frame.timestamp = timestamp;
    // Prepare and send
    if(split_iframes) {
        xop::Nal nal;
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
    } else {
        uint32_t offset = ((frame.type == xop::AUDIO_FRAME) ? 0 : 4); // Skip 00 00 00 01 (for video frames)
        frame.size = size - offset;
        frame.buffer.reset(new uint8_t[frame.size]);
        memcpy(frame.buffer.get(), data + offset, frame.size);
        return rtsp_server->PushFrame(session_id, (frame.type != xop::AUDIO_FRAME ? xop::channel_0 : xop::channel_1), frame);
    }
}

// Free RTSP server
bool rtspserver_free(uint32_t count, ...) {
    bool result = false;
    if(event_loop && rtsp_server) {
        // Remove sessions
        va_list sessions;
        va_start(sessions, count);
        for(uint32_t i=0;i<count;i++) {
            if(xop::MediaSessionId session_id = va_arg(sessions, xop::MediaSessionId)) {
                logprintf_function("Stopping the media session #%d...", session_id);
                rtsp_server->RemoveSession(session_id);
            }
        }
        va_end(sessions);
        // Stop server
        rtsp_server->Stop();
        event_loop->Quit();
        logprintf_function("The RTSP server is stopped.");
        result = true;
    }
    return result;
}

