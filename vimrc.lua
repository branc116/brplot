vim.keymap.set("n", "<F7>", function()
  pcall(vim.api.nvim_exec2(":term make PLATFORM=LINUX CONFIG=DEBUG GUI=IMGUI", {output= false}));
end)

vim.keymap.set("n", "<F6>", function()
  vim.cmd(":GdbStart gdb -q ./bin/brplot_imgui_linux_debug");
end)

vim.keymap.set("n", "<F7>", function()
  vim.cmd(":term sudo ./tools/gdb_attach.sh");
end)

vim.keymap.set("n", "<F5>", function()
  vim.cmd(":term ./tools/gdb_start.sh");
end)

vim.opt.smartindent = true
vim.opt.tabstop = 2
vim.opt.shiftwidth = 2
vim.opt.expandtab = true
vim.opt.nu = true
vim.opt.relativenumber = true
