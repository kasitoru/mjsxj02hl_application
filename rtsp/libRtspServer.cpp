#include <stdint.h>
#include <stdarg.h>
#include <vector>
#include <string>
#include <cstring>

#include "libRtspServer.h"
#include "xop/RtspServer.h"
#include "xop/H264Source.h"
#include "xop/H265Source.h"
#include "xop/DigestAuthenticator.h"

static std::shared_ptr<xop::EventLoop> event_loop(new xop::EventLoop());
static std::shared_ptr<xop::RtspServer> rtsp_server;

static bool find_start_code(const uint8_t *data, size_t size, size_t from, size_t *sc_pos, size_t *sc_len) {
    for (size_t i = from; i + 3 < size; ++i) {
        if (data[i] == 0x00 && data[i + 1] == 0x00) {
            if (data[i + 2] == 0x01) {
                *sc_pos = i;
                *sc_len = 3;
                return true;
            }
            if (i + 3 < size && data[i + 2] == 0x00 && data[i + 3] == 0x01) {
                *sc_pos = i;
                *sc_len = 4;
                return true;
            }
        }
    }
    return false;
}

static bool push_frame(xop::MediaSessionId session_id, xop::MediaChannelId channel_id, uint8_t type, int64_t timestamp, const uint8_t *data, size_t size) {
    if (size == 0) {
        return false;
    }

    xop::AVFrame frame;
    frame.type = type;
    frame.timestamp = timestamp;
    frame.last = 1;
    frame.buffer.assign(data, data + size);
    return rtsp_server->PushFrame(session_id, channel_id, frame);
}

// Default log printf function
static int logprintf_default(const char *format, ...) {
    int result = 0;
    va_list params;
    va_start(params, format);
    char *message;
    if(vasprintf(&message, format, params) != -1) {
        result = printf("[rtspserver]: %s\n", message);
        free(message);
    }
    va_end(params);
    return result;
}
static int (*logprintf_function)(const char *, ...) = logprintf_default;

// Default connected callback function
static void connected_default(uint32_t session_id, const char *peer_ip, uint16_t peer_port) { }
static void (*connected_function)(uint32_t session_id, const char *peer_ip, uint16_t peer_port) = connected_default;

// Default disconnected callback function
static void disconnected_default(uint32_t session_id, const char *peer_ip, uint16_t peer_port) { }
static void (*disconnected_function)(uint32_t session_id, const char *peer_ip, uint16_t peer_port) = disconnected_default;



// Set log printf function
bool rtspserver_logprintf(int (*function)(const char *, ...)) {
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
	        rtsp_server->SetAuthenticator(std::make_shared<xop::DigestAuthenticator>("RTSP", std::string(username), std::string(password)));
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
        case LIBRTSPSERVER_TYPE_VP8:
            session->AddSource(xop::channel_0, xop::VP8Source::CreateNew(framerate));
            logprintf_function("%s source is %s for session \"%s\" (channel = %d).", "VP8", "enabled", name, xop::channel_0);
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
        case LIBRTSPSERVER_TYPE_H264:  return (uint32_t)xop::H264Source::GetTimestamp();
        case LIBRTSPSERVER_TYPE_H265:  return (uint32_t)xop::H265Source::GetTimestamp();
        case LIBRTSPSERVER_TYPE_AAC:   return xop::AACSource::GetTimestamp(samplerate);
        case LIBRTSPSERVER_TYPE_G711A: return xop::G711ASource::GetTimestamp();
        case LIBRTSPSERVER_TYPE_VP8:   return xop::VP8Source::GetTimestamp();
        default: return 0;
    }
}

// Send media frame
bool rtspserver_frame(uint32_t session_id, signed char *data, uint8_t type, uint32_t size, uint32_t timestamp, bool split_video) {
    if(!rtsp_server) { return false; }
    xop::MediaChannelId channel_id = (type != xop::AUDIO_FRAME ? xop::channel_0 : xop::channel_1);
    const uint8_t *buffer = (const uint8_t*)data;

    // Prepare and send
    if(split_video) {
        if (type == xop::AUDIO_FRAME) {
            return push_frame(session_id, channel_id, type, (int64_t)timestamp, buffer, size);
        }

        size_t pos = 0;
        size_t sc_pos = 0;
        size_t sc_len = 0;
        bool pushed = false;

        while (find_start_code(buffer, size, pos, &sc_pos, &sc_len)) {
            size_t nal_start = sc_pos;
            size_t next_sc_pos = 0;
            size_t next_sc_len = 0;
            size_t nal_end = size;

            if (find_start_code(buffer, size, sc_pos + sc_len, &next_sc_pos, &next_sc_len)) {
                nal_end = next_sc_pos;
            }

            if (nal_end > nal_start && !push_frame(session_id, channel_id, type, (int64_t)timestamp, buffer + nal_start, nal_end - nal_start)) {
                return false;
            }

            pushed = (nal_end > nal_start);
            pos = nal_end;
            if (pos >= size) {
                break;
            }
        }

        if (pushed) {
            return true;
        }

        return push_frame(session_id, channel_id, type, (int64_t)timestamp, buffer, size);
    } else {
        // For non-split mode, strip the first start code if present
        uint32_t offset = 0;
        if (type != xop::AUDIO_FRAME) {
            // Check for start code at the beginning (0x00 0x00 0x01 or 0x00 0x00 0x00 0x01)
            if (size >= 3 && data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x01) {
                offset = 3;
            } else if (size >= 4 && data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x00 && data[3] == 0x01) {
                offset = 4;
            }
        }
        if (size <= offset) {
            return false;
        }
        return push_frame(session_id, channel_id, type, (int64_t)timestamp, buffer + offset, size - offset);
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
