#ifndef CORE_HEADER_SDL_AUDIO_GUARD
#define CORE_HEADER_SDL_AUDIO_GUARD

#include <SDL3/SDL.h>
#include <cmath>
#include <filesystem>
#include <vector>
#include <string>//string for copy SDLError
#include <numbers>

namespace core{

	class realtime_audio{
		//error status
		private:
			bool status = false;
			std::string reason;

		//get error status methods
		public:
			bool       is_ok() noexcept{return status;}
			const char* what() noexcept{return reason.c_str();}

		//vars
		private:
			std::vector<SDL_AudioStream*> realtime_audio_stream;
			const int realtime_audio_stream_num{32};

			SDL_AudioDeviceID cur_did{};//device id

			SDL_AudioSpec src_spec;
			SDL_AudioSpec dst_spec;

		//methods
		public:
			//bind all stream to default plaback
			void bind() noexcept{
				if(!status)return;
				unbind();
				cur_did = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,nullptr);
				if (cur_did == 0){
					status = false;reason = SDL_GetError();return;
				}

				if(!SDL_BindAudioStreams(cur_did,realtime_audio_stream.data(),realtime_audio_stream_num)){
					status = false;reason = SDL_GetError();return;
				}
			}

			//bind all stream to specified device
			void bind(SDL_AudioDeviceID did) noexcept{
				if(!status)return;
				unbind();
				cur_did = did;

				if(!SDL_BindAudioStreams(did,realtime_audio_stream.data(),realtime_audio_stream_num)){
					status = false;reason = SDL_GetError();return;
				}
			}

			//bind all streams
			void unbind() noexcept{
				SDL_UnbindAudioStreams(realtime_audio_stream.data(),realtime_audio_stream_num);
			}

			//reset status
			void reset_status() noexcept{
				status = true;
			}

			//put realtime data, need correct bytes(=nframe * channels * fmttype)
			//ensure buf and bytes is both correct, or errors that access out of range
			void put_audio_stream_data(const void *buf, size_t bytes) {
				if(!status)return;

				bool found = false;

				for(auto &&e : realtime_audio_stream){
					//skip
					if (SDL_GetAudioStreamAvailable(e) != 0) continue;

					if(!SDL_LockAudioStream(e)){//make sure to unlock
						status = false;reason = SDL_GetError();return;
					}

					if(SDL_PutAudioStreamData(e, buf, bytes)){
						found = true;
					}else{
						status = false;
						reason = SDL_GetError();
						//cannot return before unlocking
					}

					if(!SDL_UnlockAudioStream(e)){//unlock
						status = false;reason = SDL_GetError();return;
					}

					//return if !status
					if (!status)return;

					break;
				}

				//0 is consider the oldest, !found will use 0
				if(found) return;

				if(!SDL_ClearAudioStream(realtime_audio_stream[0])){
					status = false;reason = SDL_GetError();return;
				}

				if(!SDL_PutAudioStreamData(realtime_audio_stream[0],buf,bytes)){
					status = false;reason = SDL_GetError();return;
				}
			}

			//pause all streams
			void pause() noexcept{
				if(!status)return;

				for(auto &&e : realtime_audio_stream){
					if(!SDL_PauseAudioStreamDevice(e)){
						status = false;reason = SDL_GetError();return;
					}
				}
			}

			//resume all streams
			void resume() noexcept{
				if(!status)return;

				for(auto &&e : realtime_audio_stream){
					if(!SDL_ResumeAudioStreamDevice(e)){
						status = false;reason = SDL_GetError();return;
					}
				}
			}

			//pause cur_did device
			void pause_dev() noexcept{
				if(!status) return;

				if(!SDL_PauseAudioDevice(cur_did)){
					status = false;reason = SDL_GetError();return;
				}
			}

			//resume cur_did device
			void resume_dev() noexcept{
				if(!status)return;

				if(!SDL_ResumeAudioDevice(cur_did)){
					status = false;reason = SDL_GetError();return;
				}
			}

			//clear all streams data
			void clear() noexcept{
				if(!status)return;

				for(auto &&e : realtime_audio_stream){
					if(!SDL_ClearAudioStream(e)){
						status = false;reason = SDL_GetError();return;
					}
				}
			}

			float get_volume() noexcept{
				if(!status || realtime_audio_stream.empty())return std::nan("NaN");
				return SDL_GetAudioStreamGain(realtime_audio_stream[0]);
			}

			const SDL_AudioSpec get_src_spec() noexcept{
				return src_spec;
			}

			const SDL_AudioSpec get_dst_spec() noexcept{
				return dst_spec;
			}

			void volume(float gain) noexcept{
				if(!status)return;

				for(auto &&e : realtime_audio_stream){
					if(!SDL_SetAudioStreamGain(e,gain)){
						status = false;reason = SDL_GetError();return;
					}
				}
			}

			const char *audio_device_name(SDL_AudioDeviceID did = 0) noexcept{
				if (did == 0){
					did = cur_did;
				}

				return SDL_GetAudioDeviceName(did);
			}


		//RAII
		public:
			//init audio and create all streams
			realtime_audio(
					SDL_AudioSpec src_audio_spec = {.format = SDL_AUDIO_F32,.channels=2,.freq=44100},
					SDL_AudioSpec dst_audio_spec = {.format = SDL_AUDIO_F32,.channels=2,.freq=44100})
				noexcept
				: src_spec(src_audio_spec),dst_spec(dst_audio_spec) {

				if (!(SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO)){
					if(!SDL_Init(SDL_INIT_AUDIO)){
						status = false;reason = SDL_GetError();return;
					}
				}

				realtime_audio_stream.resize(32);
				for(auto &&e : realtime_audio_stream){
					e = SDL_CreateAudioStream(&src_spec, &dst_spec);
					if(!e){status = false;reason = SDL_GetError();return;}
				}

				status = true;
			}

			realtime_audio(realtime_audio &other) = delete;
			realtime_audio& operator=(realtime_audio &other) = delete;

			realtime_audio(realtime_audio &&other) = delete;
			realtime_audio& operator=(realtime_audio &&other) = delete;

			//delete all streams
			~realtime_audio() noexcept{
				for(auto &&e : realtime_audio_stream){
					if(e)SDL_DestroyAudioStream(e);
				}
			}

	};

	class callback_on_time_audio{

		//error status
		private:
			bool status = false;
			std::string reason;

		//get error status methods
		public:
			bool       is_ok() noexcept{return status;}
			const char* what() noexcept{return reason.c_str();}

		//vars
		public:

		//vars
		private:
			size_t read_pos{0};
			std::vector<Uint8> buf;
			SDL_AudioStream *audio_stream = nullptr;
			SDL_AudioDeviceID cur_did{};//device id
			SDL_AudioSpec src_spec;
			SDL_AudioSpec dst_spec;

		//callback
		public:
			//userdata should be type of callback_on_time_audio
			static void loop_buf_get_callback(void *userdata,
					SDL_AudioStream *stream,
					int additional_amount,
					int total_amount){

				callback_on_time_audio *self = static_cast<callback_on_time_audio*>(userdata);

				if(!self)return;
				if(!(*self).status)return;
				if((*self).buf.empty())return;
				if(additional_amount <= 0)return;

				size_t remaining{};
				size_t to_write{};
				size_t frame_size{};

				SDL_AudioSpec dst_spec;//calc correct frame_size

				if(!SDL_GetAudioStreamFormat(stream,nullptr,&dst_spec)){
					(*self).status = false; (*self).reason = SDL_GetError(); return;;
				}else{
					frame_size = SDL_AUDIO_FRAMESIZE(dst_spec);
				}

				while (additional_amount > 0) {

					if ((*self).read_pos >= (*self).buf.size()) {
						(*self).read_pos = 0;
					}

					remaining = (*self).buf.size() - (*self).read_pos;
					to_write = SDL_min(additional_amount, remaining);
					to_write = static_cast<size_t>(to_write/frame_size) * frame_size;
					if (to_write == 0) break;

					//lock start
					if(!SDL_LockAudioStream(stream)){
						(*self).status = false; (*self).reason = SDL_GetError(); return;
					}

					if(!SDL_PutAudioStreamData(stream,
								(*self).buf.data() + (*self).read_pos,
								to_write)){
						(*self).status = false; (*self).reason = SDL_GetError(); return;
					}

					(*self).read_pos += to_write;
					additional_amount -= to_write;

					//lock end
					if(!SDL_UnlockAudioStream(stream)){
						(*self).status = false; (*self).reason = SDL_GetError(); return;
					}
				}
			}

		//methods
		public:
			void put_buf_data(const void *data, size_t bytes){
				if(!status)return;

				if(!data || bytes==0){buf.resize(0);return;}

				std::vector<uint8_t> temp(static_cast<const uint8_t*>(data),
						static_cast<const uint8_t*>(data) + bytes);

				buf.resize(bytes);
				memcpy(buf.data(), temp.data(), bytes);
			}

			//bind the stream to default plaback
			void bind() noexcept{
				if(!status)return;
				unbind();
				cur_did = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,nullptr);
				if (cur_did == 0){
					status = false; reason = SDL_GetError(); return;
				}

				if(!SDL_BindAudioStream(cur_did, audio_stream)){
					status = false; reason = SDL_GetError(); return;
				}
			}

			//bind the stream to specified device
			void bind(SDL_AudioDeviceID did) noexcept{
				if(!status)return;
				unbind();
				cur_did = did;

				if(!SDL_BindAudioStream(did, audio_stream)){
					status = false; reason = SDL_GetError(); return;
				}
			}

			//bind the streams
			void unbind() noexcept{
				if(!status)return;

				SDL_UnbindAudioStream(audio_stream);
			}

			//reset status
			void reset_status() noexcept{
				status = true;
			}

			//pause the stream
			void pause() noexcept{
				if(!status)return;

				if(!SDL_PauseAudioStreamDevice(audio_stream)){
					status = false; reason = SDL_GetError(); return;
				}
			}

			bool is_paused() noexcept{
				return SDL_AudioStreamDevicePaused(audio_stream);
			}

			//resume the stream
			void resume() noexcept{
				if(!status)return;

				if(!SDL_ResumeAudioStreamDevice(audio_stream)){
					status = false; reason = SDL_GetError(); return;
				}
			}

			//pause cur_did device
			void pause_dev() noexcept{
				if(!status) return;

				if(!SDL_PauseAudioDevice(cur_did)){
					status = false; reason = SDL_GetError(); return;
				}
			}

			//resume cur_did device
			void resume_dev() noexcept{
				if(!status)return;

				if(!SDL_ResumeAudioDevice(cur_did)){
					status = false; reason = SDL_GetError(); return;
				}
			}

			//clear the stream data
			void clear() noexcept{
				if(!status)return;

				if(!SDL_ClearAudioStream(audio_stream)){
					status = false; reason = SDL_GetError(); return;
				}
			}

			float get_volume() noexcept{
				return SDL_GetAudioStreamGain(audio_stream);
			}

			void volume(float gain) noexcept{
				if(!status)return;

				if(!SDL_SetAudioStreamGain(audio_stream,gain)){
					status = false; reason = SDL_GetError(); return;
				}
			}

			size_t get_read_pos() noexcept{
				return read_pos;
			}

			void set_read_pos(size_t pos){
				if(!status)return;
				if(!audio_stream)return;

				size_t frame_size{};
				SDL_AudioSpec dst_spec;//calc correct frame_size

				if(!SDL_GetAudioStreamFormat(audio_stream,nullptr,&dst_spec)){
					status = false; reason = SDL_GetError(); return;
				}else{
					frame_size = SDL_AUDIO_FRAMESIZE(dst_spec);
				}

				read_pos = static_cast<size_t>(pos/frame_size) * frame_size;
			}

			const std::vector<Uint8> &get_buf() noexcept{
				return buf;
			}

			const SDL_AudioSpec get_src_spec() noexcept{
				return src_spec;
			}

			const SDL_AudioSpec get_dst_spec() noexcept{
				return dst_spec;
			}

			const char *audio_device_name(SDL_AudioDeviceID did = 0) noexcept{
				if (did == 0){did = cur_did;}

				return SDL_GetAudioDeviceName(did);
			}


		//RAII
		public:
			//init audio and create the stream and set get_callback
			callback_on_time_audio(
					SDL_AudioSpec src_audio_spec = {.format = SDL_AUDIO_F32,.channels=2,.freq=44100},
					SDL_AudioSpec dst_audio_spec = {.format = SDL_AUDIO_F32,.channels=2,.freq=44100})
				noexcept
				: src_spec(src_audio_spec),dst_spec(dst_audio_spec){

				if (!(SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO)){
					if(!SDL_Init(SDL_INIT_AUDIO)){
						status = false;reason = SDL_GetError();return;
					}
				}

				audio_stream = SDL_CreateAudioStream(&src_spec, &dst_spec);
				if(!audio_stream){
					status = false; reason = SDL_GetError(); return;
				}

				if(!SDL_SetAudioStreamGetCallback(audio_stream,loop_buf_get_callback,this)){
					status = false; reason = SDL_GetError(); return;
				}

				status = true;
			}

			callback_on_time_audio(callback_on_time_audio &other) = delete;
			callback_on_time_audio(callback_on_time_audio &&other) = delete;
			callback_on_time_audio &operator=(callback_on_time_audio  &other) = delete;
			callback_on_time_audio &operator=(callback_on_time_audio &&other) = delete;

			//delete the stream
			~callback_on_time_audio() noexcept{
				if(audio_stream)SDL_DestroyAudioStream(audio_stream);
			}
	};

	class recording_audio{

		//error status
		private:
			bool status = false;
			std::string reason;

		//get error status methods
		public:
			bool       is_ok() noexcept{return status;}
			const char* what() noexcept{return reason.c_str();}

		//vars
		private:
			SDL_AudioStream *audio_stream = nullptr;
			SDL_AudioDeviceID cur_did{};//device id
			SDL_AudioSpec src_spec;
			SDL_AudioSpec dst_spec;

			std::vector<Uint8> buf;

		//methods
		public:
			//bind the stream to default plaback
			void bind() noexcept{
				if(!status)return;
				unbind();
				cur_did = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_RECORDING,nullptr);
				if (cur_did == 0){
					status = false; reason = SDL_GetError(); return;
				}

				if(!SDL_BindAudioStream(cur_did, audio_stream)){
					status = false; reason = SDL_GetError(); return;
				}
			}

			//bind the stream to specified device
			void bind(SDL_AudioDeviceID did) noexcept{
				if(!status)return;
				unbind();
				cur_did = did;

				if(!SDL_BindAudioStream(did, audio_stream)){
					status = false; reason = SDL_GetError(); return;
				}
			}

			//bind the streams
			void unbind() noexcept{
				if(!status)return;

				SDL_UnbindAudioStream(audio_stream);
			}

			//reset status
			void reset_status() noexcept{
				status = true;
			}

			//get readonly recoding data
			const std::vector<Uint8> &get_buf() noexcept{
				return buf;
			}

			const SDL_AudioSpec get_src_spec() noexcept{
				return src_spec;
			}

			const SDL_AudioSpec get_dst_spec() noexcept{
				return dst_spec;
			}

			void clear_buf() noexcept{
				buf.clear();
			}

			//dump stream data to buf
			void dump_audio_stream_data() noexcept{
				if(!status)return;

				int available = SDL_GetAudioStreamAvailable(audio_stream);
				if(available > 0){
					buf.resize(available);
				}else{
					return;
				}

				if(!SDL_LockAudioStream(audio_stream)){
					status = false;reason = SDL_GetError();return;
				}

				int bytes{};
				bytes = SDL_GetAudioStreamData(audio_stream, buf.data(), buf.size());

				if(!SDL_UnlockAudioStream(audio_stream)){
					status = false;reason = SDL_GetError();return;
				}

				if(bytes == -1){
					status = false;reason = SDL_GetError();return;
				}else{
					return;
				}
			}

			//pause the stream
			void pause() noexcept{
				if(!status)return;

				if(!SDL_PauseAudioStreamDevice(audio_stream)){
					status = false; reason = SDL_GetError(); return;
				}
			}

			bool is_paused() noexcept{
				return SDL_AudioStreamDevicePaused(audio_stream);
			}

			//resume the stream
			void resume() noexcept{
				if(!status)return;

				if(!SDL_ResumeAudioStreamDevice(audio_stream)){
					status = false; reason = SDL_GetError(); return;
				}
			}

			//pause cur_did device
			void pause_dev() noexcept{
				if(!status) return;

				if(!SDL_PauseAudioDevice(cur_did)){
					status = false; reason = SDL_GetError(); return;
				}
			}


			//resume cur_did device
			void resume_dev() noexcept{
				if(!status)return;

				if(!SDL_ResumeAudioDevice(cur_did)){
					status = false; reason = SDL_GetError(); return;
				}
			}

			//clear the stream data
			void clear() noexcept{
				if(!status)return;

				if(!SDL_ClearAudioStream(audio_stream)){
					status = false; reason = SDL_GetError(); return;
				}
			}

			float get_volume() noexcept{
				return SDL_GetAudioStreamGain(audio_stream);
			}

			void volume(float gain) noexcept{
				if(!status)return;

				if(!SDL_SetAudioStreamGain(audio_stream,gain)){
					status = false; reason = SDL_GetError(); return;
				}
			}

			const char *audio_device_name(SDL_AudioDeviceID did = 0) noexcept{
				if (did == 0){did = cur_did;}

				return SDL_GetAudioDeviceName(did);
			}

		//RAII
		public:
			//init audio and create the stream
			recording_audio(
					SDL_AudioSpec src_audio_spec = {.format = SDL_AUDIO_F32,.channels=2,.freq=44100},
					SDL_AudioSpec dst_audio_spec = {.format = SDL_AUDIO_F32,.channels=2,.freq=44100})
				noexcept
				: src_spec(src_audio_spec),dst_spec(dst_audio_spec){

				if (!(SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO)){
					if(!SDL_Init(SDL_INIT_AUDIO)){
						status = false;reason = SDL_GetError();return;
					}
				}

				audio_stream = SDL_CreateAudioStream(&src_spec, &dst_spec);
				if(!audio_stream){
					status = false; reason = SDL_GetError(); return;
				}

				status = true;
			}

			recording_audio(callback_on_time_audio &other) = delete;
			recording_audio(callback_on_time_audio &&other) = delete;
			recording_audio &operator=(callback_on_time_audio  &other) = delete;
			recording_audio &operator=(callback_on_time_audio &&other) = delete;

			//delete the stream
			~recording_audio() noexcept{
				if(audio_stream)SDL_DestroyAudioStream(audio_stream);
			}
	};

	//generate sin wave data
	template <typename T>
		void write_buf_sin_wave(T *buf, size_t bufsize, size_t channels, int sample_rate,T Hz){
			if(!buf)return;
			if(bufsize == 0)return;
			if(channels <= 0)return;
			if(sample_rate <= 0)return;
			if(Hz <= 0)return;

			T val;
			for (int i=0; i < bufsize/channels;i++){
				//(2*pi*fHz) * (x/samplefreq)
				val = std::sin(2.0 * std::numbers::pi * Hz * static_cast<T>(i) / static_cast<T>(sample_rate));

				for(int j=0;j<channels;j++){
					buf[i*channels + j] = val;
				}
			}
		}

	//load wav from path
	inline bool load_wav_from_path_and_convert(
			std::filesystem::path path,
			const SDL_AudioSpec audio_spec,
			std::vector<Uint8> &audio_buf)
	{
		if(!std::filesystem::exists(path)) return false;

		SDL_AudioSpec wav_spec;
		Uint8 *wav_buf_p = nullptr;
		Uint32 wav_len{};

		std::u8string pathu8str = path.u8string();
		if(!SDL_LoadWAV(reinterpret_cast<const char*>(pathu8str.c_str()),&wav_spec, &wav_buf_p, &wav_len)){
			SDL_free(wav_buf_p);
			return false;
		}

		Uint8 *cvt_data = nullptr;
		int cvt_len{};

		if(SDL_ConvertAudioSamples(&wav_spec, wav_buf_p, wav_len, &audio_spec, &cvt_data, &cvt_len)){
			audio_buf.resize(cvt_len);
			memcpy(audio_buf.data(), cvt_data, cvt_len);
		}else{
			SDL_free(cvt_data);
			SDL_free(wav_buf_p);
			return false;
		}

		SDL_free(cvt_data);
		SDL_free(wav_buf_p);

		return true;
	}

	//load wav from path
	inline bool load_wav_from_mem_and_convert(
			const void *mem, size_t mem_size,
			const SDL_AudioSpec audio_spec,
			std::vector<Uint8> &audio_buf)
	{
		if(!mem) return false;
		SDL_IOStream *io = SDL_IOFromConstMem(mem,mem_size);
		if(!io){return false;}

		SDL_AudioSpec wav_spec;
		Uint8 *wav_buf_p = nullptr;
		Uint32 wav_len{};

		//load wav and close io before returning
		if(!SDL_LoadWAV_IO(io,true,&wav_spec, &wav_buf_p, &wav_len)){
			SDL_free(wav_buf_p);
			return false;
		}

		Uint8 *cvt_data = nullptr;
		int cvt_len{};

		if(SDL_ConvertAudioSamples(&wav_spec, wav_buf_p, wav_len, &audio_spec, &cvt_data, &cvt_len)){
			audio_buf.resize(cvt_len);
			memcpy(audio_buf.data(), cvt_data, cvt_len);
		}else{
			SDL_free(cvt_data);
			SDL_free(wav_buf_p);
			return false;
		}

		SDL_free(cvt_data);
		SDL_free(wav_buf_p);

		return true;
	}


	//log
	#if 0
	inline const char* get_name_sdl_audio_format(SDL_AudioFormat f) noexcept{
		switch(f){
			case SDL_AUDIO_UNKNOWN:return "SDL_AUDIO_UNKNOWN";
			case SDL_AUDIO_U8:return "SDL_AUDIO_U8";
			case SDL_AUDIO_S8:return "SDL_AUDIO_S8";
			case SDL_AUDIO_S16LE:return "SDL_AUDIO_S16LE";
			case SDL_AUDIO_S16BE:return "SDL_AUDIO_S16BE";
			case SDL_AUDIO_S32LE:return "SDL_AUDIO_S32LE";
			case SDL_AUDIO_S32BE:return "SDL_AUDIO_S32BE";
			case SDL_AUDIO_F32LE:return "SDL_AUDIO_F32LE";
			case SDL_AUDIO_F32BE:return "SDL_AUDIO_F32BE";
			default: return nullptr;
		}
	}

	//--------spec---------------
	template<typename LogT>
	void log_sdl_audio_spec(SDL_AudioSpec spec, LogT &&Log = SDL_Log) noexcept{
		Log("[log_sdl_audio_spec] channels:%d freq:%d format:%s",
			spec.channels,
			spec.freq,
			get_name_sdl_audio_format(spec.format)
		);
	}

	//-------devid---------
	template<typename LogT>
	void log_sdl_audio_device_format(SDL_AudioDeviceID devid, LogT && Log = SDL_Log) noexcept{
		SDL_AudioSpec spec{};
		int sample_frames = 0;

		if (SDL_GetAudioDeviceFormat(devid,&spec,&sample_frames)){
			Log("[log_sdl_audio_device_format] name:%s sample_frames:%d",
					SDL_GetAudioDeviceName(devid),sample_frames);
			log_sdl_audio_spec(spec , Log);
		}
	}

	template<typename LogT>
	void log_sdl_audio_device_channel_map(SDL_AudioDeviceID devid, LogT && Log = SDL_Log) noexcept{
		int count = 0;
		int *map = SDL_GetAudioDeviceChannelMap(devid,&count);

		if (map != nullptr){
			for (int i=0; i < count; i++){
				Log("[log_sdl_audio_device_channel_map] map:%d -> %d",i,map[i]);
			}
		}else{
			Log("[log_sdl_audio_device_channel_map] map:default count:%d",count);
		}

		SDL_free(map);
	}

	//-------audio stream---------
	template<typename LogT>
	void log_sdl_audio_stream_device(SDL_AudioStream *stream, LogT && Log = SDL_Log) noexcept{
		SDL_AudioDeviceID a_id = SDL_GetAudioStreamDevice(stream);

		log_sdl_audio_device_format(a_id, Log);
		log_sdl_audio_device_channel_map(a_id , Log);
		Log("[log_sdl_audio_device_gain] %f",SDL_GetAudioDeviceGain(a_id));
	}

	//-------void---------
	template<typename LogT>
	void log_sdl_audio_drivers(LogT && Log = SDL_Log) noexcept{
		int count = SDL_GetNumAudioDrivers();
		for (int i = 0; i < count; i++){
			Log("[log_sdl_audio_driver] index:%d driver:%s",i,SDL_GetAudioDriver(i));
		}

		Log("[log_sdl_audio_driver] current audio driver:%s",SDL_GetCurrentAudioDriver());
	}

	template<typename LogT>
	void log_sdl_audio_playback_devices(LogT && Log = SDL_Log) noexcept{
		int count = 0;
		SDL_AudioDeviceID *a_ids = SDL_GetAudioPlaybackDevices(&count);

		if (a_ids != nullptr){
			Log("[log_sdl_audio_playback_devices]: ");
			for (int i = 0;i < count; i++){
				log_sdl_audio_device_format(a_ids[i], Log);
				log_sdl_audio_device_channel_map(a_ids[i], Log);
				Log("[log_sdl_audio_device_gain] %f",SDL_GetAudioDeviceGain(a_ids[i]));
			}
		}

		if(a_ids)SDL_free(a_ids);
	}

	template<typename LogT>
	void log_sdl_audio_recording_devices(LogT && Log = SDL_Log) noexcept{
		int count = 0;
		SDL_AudioDeviceID *a_ids = SDL_GetAudioRecordingDevices(&count);

		if (a_ids != nullptr){
			Log("[log_sdl_audio_recording_devices]: ");
			for (int i = 0;i < count; i++){
				log_sdl_audio_device_format(a_ids[i], Log);
				log_sdl_audio_device_channel_map(a_ids[i], Log);
				Log("[log_sdl_audio_device_gain] %f",SDL_GetAudioDeviceGain(a_ids[i]));
			}
		}

		if(a_ids)SDL_free(a_ids);
	}


	#endif

}

#endif
