vim.keymap.set("n", "<F7>", function()
  pcall(vim.api.nvim_exec2(":term make PLATFORM=LINUX CONFIG=DEBUG GUI=IMGUI", {output= false}));
end)

vim.keymap.set("n", "<F6>", function()
  vim.cmd(":GdbStart gdb -q ./bin/brplot_imgui_linux_debug");
end)
