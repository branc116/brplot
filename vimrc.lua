vim.keymap.set("n", "<F7>", function()
  pcall(vim.api.nvim_exec2(":term make clean && make bin/rlplot_dbg", {output= false}));
end)

vim.keymap.set("n", "<F6>", function()
  vim.cmd(":GdbStart gdb -q bin/rlplot_dbg");
end)
