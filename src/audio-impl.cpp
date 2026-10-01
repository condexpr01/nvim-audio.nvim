
#include "sdl-audio.hpp"
#include <unordered_map>
#include <string>



#if defined(_WIN32) || defined(_MSC_VER)
	#define DLLEXPORT __declspec(dllexport)
	#define DLLIMPORT __declspec(dllimport)
	#define DLLHIDDEN
#elif defined(__GNUC__) || defined(__clang__)
	#define DLLEXPORT __attribute__((visibility("default")))
	#define DLLIMPORT
	#define DLLHIDDEN __attribute__((visibility("hidden")))
#else
	#define DLLEXPORT
	#define DLLIMPORT
	#define DLLHIDDEN
#endif



//{.format = SDL_AUDIO_F32,.channels=2,.freq=44100},
static core::realtime_audio &get_ra(){static core::realtime_audio ra;return ra;}
//audios data, template type args: alias, data
static std::unordered_map<std::string, std::vector<Uint8>> audios;




extern "C"{

	#if defined(_WIN32) || defined(_MSC_VER)
		#include <windows.h>
	#else
		#include <dlfcn.h>
	#endif


	#define LUA_API
	#define LUALIB_API LUA_API

	#define LUA_NUMBER double
	#define LUA_INTEGER long long

	typedef LUA_NUMBER lua_Number;
	typedef LUA_INTEGER lua_Integer;

	typedef struct lua_State lua_State;
	typedef int (*lua_CFunction) (lua_State *L);
	typedef struct luaL_Reg {
	  const char *name;
	  lua_CFunction func;
	} luaL_Reg;


	LUA_API void        (*lua_pushinteger_p) (lua_State *L, lua_Integer n) = nullptr;
	LUA_API const char *(*lua_pushstring_p)  (lua_State *L, const char *s) = nullptr;
	LUA_API void        (*lua_pushnumber_p)  (lua_State *L, lua_Number n) = nullptr;

	LUALIB_API const char *(*luaL_checklstring_p) (lua_State *L, int arg, size_t *l) = nullptr;
	LUALIB_API lua_Number  (*luaL_checknumber_p)  (lua_State *L, int arg) = nullptr;

	LUA_API void  (*lua_createtable_p) (lua_State *L, int narr, int nrec) = nullptr;

	LUA_API void  (*lua_pushcclosure_p) (lua_State *L, lua_CFunction fn, int n) = nullptr;
	LUA_API void  (*lua_setfield_p) (lua_State *L, int idx, const char *k) = nullptr;

	static bool resolve_lua_symbols_done = false;

	static void resolve_lua_symbols() {
		if (resolve_lua_symbols_done) return;

		void *handle = nullptr;

		#if defined(_WIN32) || defined(_MSC_VER)

			if (!handle) {
				HMODULE h = GetModuleHandle("lua51");
				if (!h) h = LoadLibrary("lua51");
				if (h) handle = (void *)h;
			}

			if (!handle) {
				HMODULE h = GetModuleHandle("liblua51");
				if (!h) h = LoadLibrary("liblua51");
				if (h) handle = (void *)h;
			}

			if (!handle) {
				HMODULE h = GetModuleHandle("luajit-5.1");
				if (!h) LoadLibrary("luajit-5.1");
				if (h) handle = (void *)h;
			}

			if (!handle) {
				HMODULE h = GetModuleHandle("libluajit-5.1");
				if (!h) LoadLibrary("libluajit-5.1");
				if (h) handle = (void *)h;
			}

			if (!handle) {
				HMODULE h = GetModuleHandle(nullptr);
				if (h) handle = (void *)h;
			}


			auto load = [&](const char *name) -> void * {
				void *p = nullptr;
				if (handle) p = (void *)GetProcAddress((HMODULE)handle, name);
				return p;
			};

		#else// Linux/macOS
			handle = dlopen(NULL, RTLD_LAZY | RTLD_GLOBAL);
			auto load = [&](const char *name) -> void * {
				return handle && name ? dlsym(handle, name) : nullptr;
			};
		#endif

		lua_pushinteger_p   = (decltype(lua_pushinteger_p))   load("lua_pushinteger");
		lua_pushstring_p    = (decltype(lua_pushstring_p))    load("lua_pushstring");
		lua_pushnumber_p    = (decltype(lua_pushnumber_p))    load("lua_pushnumber");
		lua_createtable_p   = (decltype(lua_createtable_p))   load("lua_createtable");
		lua_pushcclosure_p  = (decltype(lua_pushcclosure_p))  load("lua_pushcclosure");
		lua_setfield_p      = (decltype(lua_setfield_p))      load("lua_setfield");
		luaL_checklstring_p = (decltype(luaL_checklstring_p)) load("luaL_checklstring");
		luaL_checknumber_p  = (decltype(luaL_checknumber_p))  load("luaL_checknumber");

		resolve_lua_symbols_done =
			   lua_pushinteger_p
			&& lua_pushstring_p
			&& lua_pushnumber_p
			&& lua_createtable_p
			&& lua_pushcclosure_p
			&& lua_setfield_p
			&& luaL_checklstring_p
			&& luaL_checknumber_p
			;

	}

}



extern "C"{
	//add wav data into audios and set alias
	//2: alias exists
	//1(true): add success
	//0(false): err
	static int lua_load_wav(lua_State *L){
		const char *alias_name = luaL_checklstring_p(L,1,NULL);
		const char *wav_path = luaL_checklstring_p(L,2,NULL);

		auto find = audios.find(alias_name);
		if(find != audios.end()){
			lua_pushinteger_p(L, 2);
			return 1;
		}

		std::vector<Uint8> buf;
		int value = core::load_wav_from_path_and_convert(wav_path, get_ra().get_dst_spec(), buf);

		if(value == true)audios.emplace(alias_name, buf);
		lua_pushinteger_p(L, value);
		return 1;
	}

	static int lua_ra_bind(lua_State *L){
		get_ra().bind();

		lua_pushinteger_p(L,get_ra().is_ok());
		lua_pushstring_p(L,get_ra().what());
		return 2;
	}

	static int lua_ra_unbind(lua_State *L){
		get_ra().unbind();

		lua_pushinteger_p(L,get_ra().is_ok());
		lua_pushstring_p(L,get_ra().what());
		return 2;
	}

	static int lua_ra_play(lua_State *L){

		const char *alias_name = luaL_checklstring_p(L,1,NULL);

		auto find = audios.find(alias_name);
		if(find == audios.end()){
			lua_pushinteger_p(L,false);
			lua_pushstring_p(L,"invalid: alias name");
			return 2;
		}

		get_ra().put_audio_stream_data((*find).second.data(),(*find).second.size());
		lua_pushinteger_p(L,get_ra().is_ok());
		lua_pushstring_p(L,get_ra().what());
		return 2;
	}

	static int lua_ra_pause(lua_State *L){
		get_ra().pause();
		return 0;
	}

	static int lua_ra_resume(lua_State *L){
		get_ra().resume();
		return 0;
	}

	static int lua_ra_reset_status(lua_State *L){
		get_ra().reset_status();
		return 0;
	}

	static int lua_ra_clear(lua_State *L){
		get_ra().clear();
		return 0;
	}

	static int lua_ra_get_volume(lua_State *L){
		float v = get_ra().get_volume();

		lua_pushnumber_p(L,v);
		return 1;
	}

	static int lua_ra_volume(lua_State *L){
		float gain = luaL_checknumber_p(L,1);
		get_ra().volume(gain);
		return 0;
	}

	static int lua_ra_audio_device_name(lua_State *L){
		lua_pushstring_p(L, get_ra().audio_device_name());
		return 1;
	}


	DLLEXPORT int luaopen_nvim_audio_lib(lua_State *L){
		resolve_lua_symbols();

		if (!resolve_lua_symbols_done) {
			return 0;
		}

		struct luaL_Reg reg[] = {
			{"load_wav",lua_load_wav},
			{"ra_bind",lua_ra_bind},
			{"ra_unbind",lua_ra_unbind},
			{"ra_play",lua_ra_play},

			{"ra_reset_status",lua_ra_reset_status},
			{"ra_pause",lua_ra_pause},
			{"ra_resume",lua_ra_resume},
			{"ra_clear",lua_ra_clear},
			{"ra_get_volume",lua_ra_get_volume},
			{"ra_volume",lua_ra_volume},
			{"ra_audio_device_name", lua_ra_audio_device_name},
			{NULL,NULL}
		};

		lua_createtable_p(L,0,0);

		for (int i = 0; reg[i].name != NULL; i++) {
			lua_pushcclosure_p(L, reg[i].func, 0);
			lua_setfield_p(L, -2, reg[i].name);
		}

		return 1;
	}

}

