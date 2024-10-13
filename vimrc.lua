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

local dap = require('dap')

vim.keymap.set("n", "<F5>", function()
  dap.list_breakpoints()
  local file = io.open("BPS", "w");
  if file == nil then
    print("Faile to open a file")
    return
  end
  for _, ql in ipairs(vim.fn.getqflist()) do
    local s = string.format("%s:%d\n", vim.fn.getbufinfo(ql.bufnr)[1].name, ql.lnum);
    file:write(s)
  end
  file:close()
  vim.cmd[[silent !gf2 --command=tools/gdb_debug.py ./brplot &]]
end)

