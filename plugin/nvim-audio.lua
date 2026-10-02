---@diagnostic disable: undefined-global

vim.g.nvim_audio_path = vim.fn.fnamemodify(debug.getinfo(1, "S").source:sub(2), ":p:h:h")

