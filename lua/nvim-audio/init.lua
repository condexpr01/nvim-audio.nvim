---@diagnostic disable: lowercase-global, undefined-global

local M = {}

local all_ok = false

local lib_ok = false;
lib_ok, M.fn = pcall(require, "nvim_audio_lib")
if lib_ok and M.fn
	and M.fn.load_wav
	and M.fn.ra_bind
	and M.fn.ra_unbind
	and M.fn.ra_play
	and M.fn.ra_pause
	and M.fn.ra_resume
	and M.fn.ra_reset_status
	and M.fn.ra_clear
	and M.fn.ra_get_volume
	and M.fn.ra_volume
	and M.fn.ra_audio_device_name
	then

	all_ok = true
else
	all_ok = false
	M.fn = nil
end


local default_opts = {
	bind_when_setup = true,
	volume = 1.0
}


M.setup = function(opts)
	local options = vim.tbl_deep_extend('force', default_opts, opts or {})

	if all_ok then
		if options.bind_when_setup then
			local ok,what = M.fn.ra_bind()
			if not ok then
				vim.notify(what)
			end
		end

		M.fn.ra_volume(options.volume)

	else
		vim.notify("FAILED: require nvim_audio_lib")

	end

end

return M
