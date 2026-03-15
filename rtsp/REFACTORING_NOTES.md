# RTSP Refactoring Notes - SPS/PPS Auto-Extraction

## Overview
Refactored the RTSP server to automatically extract and manage SPS/PPS (H.264) and VPS/SPS/PPS (H.265) parameters directly within the media sources, eliminating duplicate parsing logic in the wrapper layer.

## Changes Made

### 1. H264Source::HandleFrame() - Auto SPS/PPS Extraction
**File**: `RtspServer/src/xop/H264Source.cpp`

- Added `find_start_code_h264()` helper function to locate NAL unit boundaries
- Added `extract_sps_pps_h264()` function to parse and extract SPS/PPS from video frames
- Modified `HandleFrame()` to automatically detect and store SPS/PPS on first I-frame:
  - Only extracts if `sps_` or `pps_` vectors are empty
  - No performance impact on subsequent frames
  - Transparent to the caller

**Benefits**:
- SDP generation includes complete `fmtp` attributes from the first client connection
- No timing issues between frame arrival and client connection
- Decentralized responsibility: each source manages its own parameters

### 2. H265Source::HandleFrame() - Auto VPS/SPS/PPS Extraction
**File**: `RtspServer/src/xop/H265Source.cpp`

- Added `find_start_code_h265()` helper function
- Added `extract_vps_sps_pps_h265()` function to parse H.265 parameter sets
- Same pattern as H264: automatic extraction on first frame with parameters

### 3. libRtspServer.cpp - Cleanup
**File**: `libRtspServer.cpp`

**Removed**:
- `find_start_code()` function (duplicated in sources)
- `parse_nal_units()` function (duplicated in sources)
- `update_h264_fmtp()` function (now handled by H264Source)
- `update_h265_fmtp()` function (now handled by H265Source)
- `update_fmtp_if_needed()` function (no longer needed)
- Call to `update_fmtp_if_needed()` in `rtspserver_frame()`

**Simplified**:
- `rtspserver_frame()` - removed 60+ lines of duplicate NAL parsing
- Start code stripping logic simplified to inline checks (only at frame boundary)

## Architecture Benefits

### Before
```
Frame received
  → rtsp_media_frame()
    → update_fmtp_if_needed()
      → parse_nal_units()        ← duplicate logic
      → extract parameters
      → SetSPS/SetPPS
    → rtspserver_frame()
      → PushFrame()
        → H264Source::HandleFrame()  ← only receives data
```

### After
```
Frame received
  → rtsp_media_frame()
    → rtspserver_frame()
      → PushFrame()
        → H264Source::HandleFrame()
          → extract_sps_pps_h264()  ← integrated logic
          → SetSPS/SetPPS on first call
          → GetAttribute() generates complete fmtp
```

## Zero-Copy Alignment
The refactoring aligns with zero-copy goals:
- Frames are parsed once per source during `HandleFrame()`
- No redundant parsing in wrapper layer
- Direct buffer assignments without intermediate copies
- Shared `RtpPacket` buffers via `std::shared_ptr`

## Notes on SDP Cache
The SDP is still cached in `MediaSession::sdp_` after first generation. This is acceptable because:
1. Video parameters (SPS/PPS) are extracted before any client connects
2. SDP is generated on first DESCRIBE request with complete parameters
3. Parameters never change during session (fixed resolution, codec profile)
4. If future dynamic changes are needed, `MediaSession::InvalidateSdp()` can be called

## Testing Recommendations
1. Verify SDP contains `fmtp` attributes on first client connection
2. Test with both H.264 and H.265 streams
3. Verify correct profile-level-id extraction for H.264
4. Confirm VPS/SPS/PPS presence in H.265 SDP
5. Check performance with multiple simultaneous clients

## Future Improvements
- Consider pooling NAL parsing buffers if performance testing shows bottlenecks
- Monitor SPS/PPS change detection if dynamic resolution switching is needed
- Evaluate impact of `RTP_PACKET_POOL_SIZE` constant if implemented in submodule