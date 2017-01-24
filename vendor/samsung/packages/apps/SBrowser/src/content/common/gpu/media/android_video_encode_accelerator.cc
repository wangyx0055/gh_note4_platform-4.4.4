// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/gpu/media/android_video_encode_accelerator.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/metrics/histogram.h"
#include "content/common/gpu/gpu_channel.h"
#include "content/public/common/content_switches.h"
#include "gpu/command_buffer/service/gles2_cmd_decoder.h"
#include "media/base/android/media_codec_bridge.h"
#include "media/base/bitstream_buffer.h"
#include "media/base/limits.h"
#if defined(ENABLE_WEBRTC_H264_CODEC)
#include "media/base/video_decoder_config.h"
#include "media/filters/h264_parser.h"
#endif
#include "media/video/picture.h"
#include "third_party/libyuv/include/libyuv/convert_from.h"
#include "ui/gl/android/scoped_java_surface.h"
#include "ui/gl/gl_bindings.h"

using media::MediaCodecBridge;
using media::VideoCodecBridge;
using media::VideoFrame;

namespace content {

enum {
  // Subset of MediaCodecInfo.CodecCapabilities.
  COLOR_FORMAT_YUV420_SEMIPLANAR = 21,
};

// Helper macros for dealing with failure.  If |result| evaluates false, emit
// |log| to DLOG(ERROR), register |error| with the client, and return.
#define RETURN_ON_FAILURE(result, log, error)                 \
  do {                                                        \
    if (!(result)) {                                          \
      DLOG(ERROR) << log;                                     \
      LOG(ERROR) << "AndroidVideoEncodeAccelerator Error:" << log;                                          \
      if (client_ptr_factory_.GetWeakPtr()) {                 \
        client_ptr_factory_.GetWeakPtr()->NotifyError(error); \
        client_ptr_factory_.InvalidateWeakPtrs();             \
      }                                                       \
      return;                                                 \
    }                                                         \
  } while (0)

// Because MediaCodec is thread-hostile (must be poked on a single thread) and
// has no callback mechanism (b/11990118), we must drive it by polling for
// complete frames (and available input buffers, when the codec is fully
// saturated).  This function defines the polling delay.  The value used is an
// arbitrary choice that trades off CPU utilization (spinning) against latency.
// Mirrors android_video_decode_accelerator.cc::DecodePollDelay().
static inline const base::TimeDelta EncodePollDelay() {
  // An alternative to this polling scheme could be to dedicate a new thread
  // (instead of using the ChildThread) to run the MediaCodec, and make that
  // thread use the timeout-based flavor of MediaCodec's dequeue methods when it
  // believes the codec should complete "soon" (e.g. waiting for an input
  // buffer, or waiting for a picture when it knows enough complete input
  // pictures have been fed to saturate any internal buffering).  This is
  // speculative and it's unclear that this would be a win (nor that there's a
  // reasonably device-agnostic way to fill in the "believes" above).
  return base::TimeDelta::FromMilliseconds(10);
}

static inline const base::TimeDelta NoWaitTimeOut() {
  return base::TimeDelta::FromMicroseconds(0);
}

AndroidVideoEncodeAccelerator::AndroidVideoEncodeAccelerator(
    media::VideoEncodeAccelerator::Client* client)
    : client_ptr_factory_(client),
      num_buffers_at_codec_(0),
      num_output_buffers_(-1),
      output_buffers_capacity_(0),
#if defined(ENABLE_WEBRTC_H264_CODEC)
      h264_sps_pps_buffer(NULL),
      h264_sps_pps_size(0),
      output_profile_(media::VIDEO_CODEC_PROFILE_UNKNOWN),
#endif
      last_set_bitrate_(0) {}

AndroidVideoEncodeAccelerator::~AndroidVideoEncodeAccelerator() {
  DCHECK(thread_checker_.CalledOnValidThread());
#if defined(ENABLE_WEBRTC_H264_CODEC)
  if (h264_sps_pps_buffer != NULL)
    free(h264_sps_pps_buffer);
#endif
}

// static
std::vector<media::VideoEncodeAccelerator::SupportedProfile>
AndroidVideoEncodeAccelerator::GetSupportedProfiles() {
  std::vector<MediaCodecBridge::CodecsInfo> codecs_info =
      MediaCodecBridge::GetCodecsInfo();

  std::vector<SupportedProfile> profiles;

#if defined(ENABLE_WEBRTC)
  const CommandLine* cmd_line = CommandLine::ForCurrentProcess();
  if (cmd_line->HasSwitch(switches::kDisableWebRtcHWEncoding))
    return profiles;
#endif

  for (size_t i = 0; i < codecs_info.size(); ++i) {
    const MediaCodecBridge::CodecsInfo& info = codecs_info[i];
#if defined(ENABLE_WEBRTC_H264_CODEC)
    if (info.direction == media::MEDIA_CODEC_ENCODER) {
      SupportedProfile profile;
      // Wouldn't it be nice if MediaCodec exposed the maximum capabilities
      // of the encoder? Sure would be. Too bad it doesn't. So we hard-code
      // some reasonable defaults.
      profile.max_framerate.numerator = 30;
      profile.max_framerate.denominator = 1;

      if (info.codecs == "vp8"
        && !media::VideoCodecBridge::IsKnownUnaccelerated(
              media::kCodecVP8, media::MEDIA_CODEC_ENCODER)) {
        profile.max_resolution.SetSize(1920, 1088);
        profile.profile = media::VP8PROFILE_MAIN;
        profiles.push_back(profile);
      } else if ((info.codecs == "avc" || info.codecs == "avc1")
        && !media::VideoCodecBridge::IsKnownUnaccelerated(
              media::kCodecH264, media::MEDIA_CODEC_ENCODER)) {
        profile.max_resolution.SetSize(1280, 720);
        profile.profile = media::H264PROFILE_BASELINE;
        profiles.push_back(profile);
      }
    }
#else
    if (info.direction != media::MEDIA_CODEC_ENCODER || info.codecs != "vp8" ||
        VideoCodecBridge::IsKnownUnaccelerated(media::kCodecVP8,
                                               media::MEDIA_CODEC_ENCODER)) {
      // We're only looking for a HW VP8 encoder.
      continue;
    }
    SupportedProfile profile;
    profile.profile = media::VP8PROFILE_MAIN;
    // Wouldn't it be nice if MediaCodec exposed the maximum capabilities of the
    // encoder?  Sure would be.  Too bad it doesn't.  So we hard-code some
    // reasonable defaults.
    profile.max_resolution.SetSize(1920, 1088);
    profile.max_framerate.numerator = 30;
    profile.max_framerate.denominator = 1;
    profiles.push_back(profile);
#endif
  }
  return profiles;
}

void AndroidVideoEncodeAccelerator::Initialize(
    VideoFrame::Format format,
    const gfx::Size& input_visible_size,
    media::VideoCodecProfile output_profile,
    uint32 initial_bitrate) {
  DVLOG(3) << __PRETTY_FUNCTION__ << " format: " << format
           << ", input_visible_size: " << input_visible_size.ToString()
           << ", output_profile: " << output_profile
           << ", initial_bitrate: " << initial_bitrate;
  DCHECK(!media_codec_);
  DCHECK(thread_checker_.CalledOnValidThread());

#if defined(ENABLE_WEBRTC_H264_CODEC)
  media::VideoCodec codec_type = media::kUnknownVideoCodec;
  int32 iframe_interval = kint32max;

  last_set_bitrate_ = initial_bitrate;
  output_profile_ = output_profile;

  if (output_profile == media::VP8PROFILE_MAIN) {
    codec_type = media::kCodecVP8;
    iframe_interval = IFRAME_VP8_INTERVAL;
  } else if (output_profile == media::H264PROFILE_BASELINE) {
    codec_type = media::kCodecH264;
    initial_bitrate = INITIAL_H264_BITRATE;
    iframe_interval = IFRAME_H264_INTERVAL;
  }

  if (!(media::MediaCodecBridge::SupportsSetParameters() &&
        format == VideoFrame::I420) && codec_type == media::kUnknownVideoCodec) {
    DLOG(ERROR) << "Unexpected combo with profile: "
                << format << ", " << output_profile;
    return;
  }

  if (media::VideoCodecBridge::IsKnownUnaccelerated(
          codec_type, media::MEDIA_CODEC_ENCODER)) {
    DLOG(ERROR) << "No HW support";
    return;
  }

  media_codec_.reset(
      media::VideoCodecBridge::CreateEncoder(codec_type,
                                            input_visible_size,
                                            initial_bitrate,
                                            INITIAL_FRAMERATE,
                                            iframe_interval,
                                            COLOR_FORMAT_YUV420_SEMIPLANAR));
#else
  RETURN_ON_FAILURE(media::MediaCodecBridge::IsAvailable() &&
                        media::MediaCodecBridge::SupportsSetParameters() &&
                        format == VideoFrame::I420 &&
                        output_profile == media::VP8PROFILE_MAIN,
                    "Unexpected combo: " << format << ", " << output_profile,
                    kInvalidArgumentError);

  last_set_bitrate_ = initial_bitrate;

  // Only consider using MediaCodec if it's likely backed by hardware.
  RETURN_ON_FAILURE(!media::VideoCodecBridge::IsKnownUnaccelerated(
                         media::kCodecVP8, media::MEDIA_CODEC_ENCODER),
                    "No HW support",
                    kPlatformFailureError);

  // TODO(fischman): when there is more HW out there with different color-space
  // support, this should turn into a negotiation with the codec for supported
  // formats.  For now we use the only format supported by the only available
  // HW.
  media_codec_.reset(
      media::VideoCodecBridge::CreateEncoder(media::kCodecVP8,
                                             input_visible_size,
                                             initial_bitrate,
                                             INITIAL_FRAMERATE,
                                             IFRAME_INTERVAL,
                                             COLOR_FORMAT_YUV420_SEMIPLANAR));
#endif

  RETURN_ON_FAILURE(
      media_codec_,
      "Failed to create/start the codec: " << input_visible_size.ToString(),
      kPlatformFailureError);

  base::MessageLoop::current()->PostTask(
      FROM_HERE,
      base::Bind(&VideoEncodeAccelerator::Client::NotifyInitializeDone,
                 client_ptr_factory_.GetWeakPtr()));

  num_output_buffers_ = media_codec_->GetOutputBuffersCount();
  output_buffers_capacity_ = media_codec_->GetOutputBuffersCapacity();
  base::MessageLoop::current()->PostTask(
      FROM_HERE,
      base::Bind(&VideoEncodeAccelerator::Client::RequireBitstreamBuffers,
                 client_ptr_factory_.GetWeakPtr(),
                 num_output_buffers_,
                 input_visible_size,
                 output_buffers_capacity_));
}

void AndroidVideoEncodeAccelerator::MaybeStartIOTimer() {
  if (!io_timer_.IsRunning() &&
      (num_buffers_at_codec_ > 0 || !pending_frames_.empty())) {
    io_timer_.Start(FROM_HERE,
                    EncodePollDelay(),
                    this,
                    &AndroidVideoEncodeAccelerator::DoIOTask);
  }
}

void AndroidVideoEncodeAccelerator::MaybeStopIOTimer() {
  if (io_timer_.IsRunning() &&
      (num_buffers_at_codec_ == 0 && pending_frames_.empty())) {
    io_timer_.Stop();
  }
}

void AndroidVideoEncodeAccelerator::Encode(
    const scoped_refptr<VideoFrame>& frame,
    bool force_keyframe) {
  DVLOG(3) << __PRETTY_FUNCTION__ << ": " << force_keyframe;
  DCHECK(thread_checker_.CalledOnValidThread());
  RETURN_ON_FAILURE(frame->format() == VideoFrame::I420,
                    "Unexpected format",
                    kInvalidArgumentError);

  // MediaCodec doesn't have a way to specify stride for non-Packed formats, so
  // we insist on being called with packed frames and no cropping :(
  RETURN_ON_FAILURE(frame->row_bytes(VideoFrame::kYPlane) ==
                            frame->stride(VideoFrame::kYPlane) &&
                        frame->row_bytes(VideoFrame::kUPlane) ==
                            frame->stride(VideoFrame::kUPlane) &&
                        frame->row_bytes(VideoFrame::kVPlane) ==
                            frame->stride(VideoFrame::kVPlane) &&
                        gfx::Rect(frame->coded_size()) == frame->visible_rect(),
                    "Non-packed frame, or visible rect != coded size",
                    kInvalidArgumentError);

  pending_frames_.push(MakeTuple(frame, force_keyframe, base::Time::Now()));
  DoIOTask();
}

void AndroidVideoEncodeAccelerator::UseOutputBitstreamBuffer(
    const media::BitstreamBuffer& buffer) {
  DVLOG(3) << __PRETTY_FUNCTION__ << ": bitstream_buffer_id=" << buffer.id();
  DCHECK(thread_checker_.CalledOnValidThread());
  RETURN_ON_FAILURE(buffer.size() >= media_codec_->GetOutputBuffersCapacity(),
                    "Output buffers too small!",
                    kInvalidArgumentError);
  available_bitstream_buffers_.push_back(buffer);
  DoIOTask();
}

void AndroidVideoEncodeAccelerator::RequestEncodingParametersChange(
    uint32 bitrate,
    uint32 framerate) {
  DVLOG(3) << __PRETTY_FUNCTION__ << ": bitrate: " << bitrate
           << ", framerate: " << framerate;
  DCHECK(thread_checker_.CalledOnValidThread());
  if (bitrate != last_set_bitrate_
#if defined(ENABLE_WEBRTC_H264_CODEC)
    && bitrate < INITIAL_H264_BITRATE
#endif
  )
  {
    last_set_bitrate_ = bitrate;
    media_codec_->SetVideoBitrate(bitrate);
  }
  // Note: Android's MediaCodec doesn't allow mid-stream adjustments to
  // framerate, so we ignore that here.  This is OK because Android only uses
  // the framerate value from MediaFormat during configure() as a proxy for
  // bitrate, and we set that explicitly.
}

void AndroidVideoEncodeAccelerator::Destroy() {
  DVLOG(3) << __PRETTY_FUNCTION__;
  DCHECK(thread_checker_.CalledOnValidThread());
  client_ptr_factory_.InvalidateWeakPtrs();
  if (media_codec_) {
    if (io_timer_.IsRunning())
      io_timer_.Stop();
    media_codec_->Stop();
  }
  delete this;
}

void AndroidVideoEncodeAccelerator::DoIOTask() {
  QueueInput();
  DequeueOutput();
  MaybeStartIOTimer();
  MaybeStopIOTimer();
}

void AndroidVideoEncodeAccelerator::QueueInput() {
  if (!client_ptr_factory_.GetWeakPtr() || pending_frames_.empty())
    return;

  int input_buf_index = 0;
  media::MediaCodecStatus status =
      media_codec_->DequeueInputBuffer(NoWaitTimeOut(), &input_buf_index);
  if (status != media::MEDIA_CODEC_OK) {
    DCHECK(status == media::MEDIA_CODEC_DEQUEUE_INPUT_AGAIN_LATER ||
           status == media::MEDIA_CODEC_ERROR);
    RETURN_ON_FAILURE(status != media::MEDIA_CODEC_ERROR,
                      "MediaCodec error",
                      kPlatformFailureError);
    return;
  }

  const PendingFrames::value_type& input = pending_frames_.front();
  bool is_key_frame = input.b;
  if (is_key_frame) {
    // Ideally MediaCodec would honor BUFFER_FLAG_SYNC_FRAME so we could
    // indicate this in the QueueInputBuffer() call below and guarantee _this_
    // frame be encoded as a key frame, but sadly that flag is ignored.
    // Instead, we request a key frame "soon".
    media_codec_->RequestKeyFrameSoon();
  }
  scoped_refptr<VideoFrame> frame = input.a;

  uint8* buffer = NULL;
  size_t capacity = 0;
  media_codec_->GetInputBuffer(input_buf_index, &buffer, &capacity);

  size_t queued_size =
      VideoFrame::AllocationSize(VideoFrame::I420, frame->coded_size());
  RETURN_ON_FAILURE(capacity >= queued_size,
                    "Failed to get input buffer: " << input_buf_index,
                    kPlatformFailureError);

  uint8* dst_y = buffer;
  int dst_stride_y = frame->stride(VideoFrame::kYPlane);
  uint8* dst_uv = buffer + frame->stride(VideoFrame::kYPlane) *
                               frame->rows(VideoFrame::kYPlane);
  int dst_stride_uv = frame->stride(VideoFrame::kUPlane) * 2;
  // Why NV12?  Because COLOR_FORMAT_YUV420_SEMIPLANAR.  See comment at other
  // mention of that constant.
  bool converted = !libyuv::I420ToNV12(frame->data(VideoFrame::kYPlane),
                                       frame->stride(VideoFrame::kYPlane),
                                       frame->data(VideoFrame::kUPlane),
                                       frame->stride(VideoFrame::kUPlane),
                                       frame->data(VideoFrame::kVPlane),
                                       frame->stride(VideoFrame::kVPlane),
                                       dst_y,
                                       dst_stride_y,
                                       dst_uv,
                                       dst_stride_uv,
                                       frame->coded_size().width(),
                                       frame->coded_size().height());
  RETURN_ON_FAILURE(converted, "Failed to I420ToNV12!", kPlatformFailureError);

#if defined(ENABLE_WEBRTC_H264_CODEC)
  if (output_profile_ == media::VP8PROFILE_MAIN)
    fake_input_timestamp_ += base::TimeDelta::FromMicroseconds(1);
  else if (output_profile_ == media::H264PROFILE_BASELINE)
    fake_input_timestamp_ += base::TimeDelta::FromMicroseconds(INITIAL_H264_BITRATE / INITIAL_FRAMERATE);
#else
  fake_input_timestamp_ += base::TimeDelta::FromMicroseconds(1);
#endif
  status = media_codec_->QueueInputBuffer(
      input_buf_index, NULL, queued_size, fake_input_timestamp_);
  UMA_HISTOGRAM_TIMES("Media.AVEA.InputQueueTime", base::Time::Now() - input.c);
  RETURN_ON_FAILURE(status == media::MEDIA_CODEC_OK,
                    "Failed to QueueInputBuffer: " << status,
                    kPlatformFailureError);
  ++num_buffers_at_codec_;
  pending_frames_.pop();
}

bool AndroidVideoEncodeAccelerator::DoOutputBuffersSuffice() {
  // If this returns false ever, then the VEA::Client interface will need to
  // grow a DismissBitstreamBuffer() call, and VEA::Client impls will have to be
  // prepared to field multiple requests to RequireBitstreamBuffers().
  int count = media_codec_->GetOutputBuffersCount();
  size_t capacity = media_codec_->GetOutputBuffersCapacity();
  bool ret = media_codec_->GetOutputBuffers() && count <= num_output_buffers_ &&
             capacity <= output_buffers_capacity_;
  LOG_IF(ERROR, !ret) << "Need more/bigger buffers; before: "
                      << num_output_buffers_ << "x" << output_buffers_capacity_
                      << ", now: " << count << "x" << capacity;
  UMA_HISTOGRAM_BOOLEAN("Media.AVEA.OutputBuffersSuffice", ret);
  return ret;
}

void AndroidVideoEncodeAccelerator::DequeueOutput() {
  if (!client_ptr_factory_.GetWeakPtr() ||
      available_bitstream_buffers_.empty() || num_buffers_at_codec_ == 0) {
    return;
  }

  int32 buf_index = 0;
  size_t offset = 0;
  size_t size = 0;
  bool key_frame = false;
  do {
    media::MediaCodecStatus status = media_codec_->DequeueOutputBuffer(
        NoWaitTimeOut(), &buf_index, &offset, &size, NULL, NULL, &key_frame);
    switch (status) {
      case media::MEDIA_CODEC_DEQUEUE_OUTPUT_AGAIN_LATER:
        return;

      case media::MEDIA_CODEC_ERROR:
        RETURN_ON_FAILURE(false, "Codec error", kPlatformFailureError);
        // Unreachable because of previous statement, but included for clarity.
        return;

      case media::MEDIA_CODEC_OUTPUT_FORMAT_CHANGED:  // Fall-through.
      case media::MEDIA_CODEC_OUTPUT_BUFFERS_CHANGED:
        RETURN_ON_FAILURE(DoOutputBuffersSuffice(),
                          "Bitstream now requires more/larger buffers",
                          kPlatformFailureError);
        break;

      case media::MEDIA_CODEC_OK:
        DCHECK_GE(buf_index, 0);
        break;

      default:
        NOTREACHED();
        break;
    }
  } while (buf_index < 0);

  media::BitstreamBuffer bitstream_buffer = available_bitstream_buffers_.back();
  available_bitstream_buffers_.pop_back();
  scoped_ptr<base::SharedMemory> shm(
      new base::SharedMemory(bitstream_buffer.handle(), false));
  RETURN_ON_FAILURE(shm->Map(bitstream_buffer.size()),
                    "Failed to map SHM",
                    kPlatformFailureError);
  RETURN_ON_FAILURE(size <= shm->mapped_size(),
                    "Encoded buffer too large: " << size << ">"
                                                 << shm->mapped_size(),
                    kPlatformFailureError);

  media_codec_->CopyFromOutputBuffer(buf_index, offset, shm->memory(), size);
  media_codec_->ReleaseOutputBuffer(buf_index, false);
  --num_buffers_at_codec_;

#if defined(ENABLE_WEBRTC_H264_CODEC)
  uint8* shm_buffer_ptr = (uint8*)shm->memory();

  if (output_profile_ == media::H264PROFILE_BASELINE) {
    // If the buffer's nalu type is SPS, it just copies to h264_sps_pps_buffer to
    // combine with IDRSlice.
    if ((shm_buffer_ptr[4] & 0x1F) == media::H264NALU::kSPS) {
      h264_sps_pps_buffer = (uint8*)malloc(sizeof(uint8) * size);
      memset(h264_sps_pps_buffer, 0, size);
      memcpy(h264_sps_pps_buffer, shm_buffer_ptr, size);
      h264_sps_pps_size = size;
      return;
    }

    // If the buffer is IDRSlice, it should be combined
    // with SPS that previously stored.
    if ((h264_sps_pps_buffer != NULL)
      && ((shm_buffer_ptr[4] & 0x1F) == media::H264NALU::kIDRSlice)) {
      RETURN_ON_FAILURE(size + h264_sps_pps_size <= shm->mapped_size(),
                "SHM size is less than encoded bufer: " << size + h264_sps_pps_size
                << ">" << shm->mapped_size(), kPlatformFailureError);
      scoped_ptr<uint8> swap_buffer(new uint8[size]);
      uint8* swap_buffer_ptr = (uint8*)swap_buffer.get();
      memcpy(swap_buffer_ptr, shm->memory(), size);
      memcpy(shm->memory(), h264_sps_pps_buffer, h264_sps_pps_size);
      memcpy((uint8*)shm->memory() + h264_sps_pps_size, swap_buffer_ptr, size + h264_sps_pps_size);
      size = size + h264_sps_pps_size;
    }
  }
#endif

  UMA_HISTOGRAM_COUNTS_10000("Media.AVEA.EncodedBufferSizeKB", size / 1024);
  base::MessageLoop::current()->PostTask(
      FROM_HERE,
      base::Bind(&VideoEncodeAccelerator::Client::BitstreamBufferReady,
                 client_ptr_factory_.GetWeakPtr(),
                 bitstream_buffer.id(),
                 size,
                 key_frame));
}

}  // namespace content