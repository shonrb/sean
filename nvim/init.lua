-------------------------------------------------------------------------------
--- Settings
-------------------------------------------------------------------------------
vim.opt.guicursor = ""
vim.opt.number = true
vim.opt.mouse = 'a'
vim.opt.ignorecase = true
vim.opt.smartcase = true
vim.opt.hlsearch = false
vim.opt.wrap = true
vim.opt.breakindent = true
vim.opt.tabstop = 4
vim.opt.shiftwidth = 4
vim.opt.softtabstop = 4
vim.opt.expandtab = true
vim.opt.smartindent = true
vim.opt.signcolumn = 'yes'
vim.opt.scrolloff = 4
vim.opt.isfname:append("@-@")
vim.opt.cursorline = true
vim.opt.autoread = true

local group = vim.api.nvim_create_augroup('user_cmds', {clear = true})

-- Highlight text on yank
vim.api.nvim_create_autocmd('TextYankPost', {
  desc = 'Highlight on yank',
  group = group,
  callback = function()
    vim.highlight.on_yank({higroup = 'Visual', timeout = 100})
  end,
})

vim.api.nvim_create_autocmd('FileType', {
  pattern = {'help', 'man'},
  group = group,
  command = 'nnoremap <buffer> q <cmd>quit<cr>'
})

-------------------------------------------------------------------------------
--- Remaps
-------------------------------------------------------------------------------
vim.g.mapleader = ' '

-- Line start / end
vim.keymap.set({'n', 'x', 'o'}, '<leader>h', '^')
vim.keymap.set({'n', 'x', 'o'}, '<leader>l', 'g_')

-- Select all
vim.keymap.set('n', '<leader>a', ':keepjumps normal! ggVG<cr>')

-- Basic clipboard interaction
vim.keymap.set({'n', 'x'}, 'gy', '"+y') -- copy
vim.keymap.set({'n', 'x'}, 'gp', '"+p') -- paste

-- Move selection in visual mode
vim.keymap.set("v", "J", ":m '>+1<CR>gv=gv")
vim.keymap.set("v", "K", ":m '<-2<CR>gv=gv")

-- Keep cursor pos with J
vim.keymap.set("n", "J", "mzJ`z")

-- Keep cursor mid screen when jumping
vim.keymap.set("n", "<C-d>", "<C-d>zz")
vim.keymap.set("n", "<C-u>", "<C-u>zz")
vim.keymap.set("n", "n", "nzzzv")
vim.keymap.set("n", "N", "Nzzzv")

-- ESC to exit terminal mode
vim.keymap.set("t", "<Esc>", "<C-\\><C-n><C-\\><C-n>")

-- Windowing
vim.keymap.set("n", "<leader>wl", "<C-w>l")
vim.keymap.set("n", "<leader>wk", "<C-w>k")
vim.keymap.set("n", "<leader>wj", "<C-w>j")
vim.keymap.set("n", "<leader>wh", "<C-w>h")
vim.keymap.set("n", "<leader>wL", "<C-w>L")
vim.keymap.set("n", "<leader>wK", "<C-w>K")
vim.keymap.set("n", "<leader>wJ", "<C-w>J")
vim.keymap.set("n", "<leader>wH", "<C-w>H")
vim.keymap.set("n", "<leader>w+", "<C-w>H")
vim.keymap.set("n", "<leader>w<Tab>", "<C-w><C-w>")
vim.keymap.set("n", "<leader>w/", "<C-w><C-v><C-w><C-w>")
vim.keymap.set("n", "<leader>w-", "<C-w><C-s><C-w><C-w>")

-- Tabs
vim.keymap.set("n", "<leader>tc", ":tabnew<cr>")
vim.keymap.set("n", "<leader>tx", ":tabclose<cr>")
vim.keymap.set("n", "<leader>tp", ":tabp<cr>")
vim.keymap.set("n", "<leader>th", ":tabp<cr>")
vim.keymap.set("n", "<leader>tn", ":tabN<cr>")
vim.keymap.set("n", "<leader>tl", ":tabN<cr>")
vim.keymap.set("n", "<leader>t1", ":tabN<cr>")

-- Insert mode quick delete
vim.api.nvim_set_keymap('i', '<C-BS>', '<C-W>', {noremap = true})

-- Buffer handling
vim.keymap.set("n", "<leader>q", function()
    if vim.bo.buftype == "terminal" then
        vim.cmd("q!")
    else
        vim.cmd("x")
    end
end)
vim.keymap.set("n", "<leader>s", ":w!<cr>")

-- Set indent
vim.api.nvim_create_user_command("Indent", (function(arg)
    v = tonumber(arg.args)
    vim.opt.tabstop     = v
    vim.opt.softtabstop = v
    vim.opt.shiftwidth  = v
end), {nargs=1})

-- No comments in terminal
vim.api.nvim_command("autocmd TermOpen * setlocal nonumber")

-------------------------------------------------------------------------------
--- Lazy
-------------------------------------------------------------------------------

local lazy = {}

function lazy.install(path)
  if not vim.loop.fs_stat(path) then
    print('Installing lazy.nvim....')
    vim.fn.system({
      'git',
      'clone',
      '--filter=blob:none',
      'https://github.com/folke/lazy.nvim.git',
      '--branch=stable', -- latest stable release
      path,
    })
  end
end

function lazy.setup(plugins)
  if vim.g.plugins_ready then
    return
  end

  -- You can "comment out" the line below after lazy.nvim is installed
  lazy.install(lazy.path)

  vim.opt.rtp:prepend(lazy.path)

  require('lazy').setup(plugins, lazy.opts)
  vim.g.plugins_ready = true
end

lazy.path = vim.fn.stdpath('data') .. '/lazy/lazy.nvim'
lazy.opts = {}

lazy.setup({
  {'comfysage/evergarden'},
  { "ellisonleao/gruvbox.nvim" },
  {"xero/miasma.nvim"},
  {'nvim-lualine/lualine.nvim'},
  {'kyazdani42/nvim-tree.lua'},
  {'nvim-telescope/telescope.nvim', branch = '0.1.x'},
  {'nvim-telescope/telescope-fzf-native.nvim', build = 'make'},
  {'tpope/vim-fugitive'},
  {'tpope/vim-eunuch'},
  {'nvim-treesitter/nvim-treesitter'},
  {'nvim-treesitter/nvim-treesitter-textobjects'},
  {"kwkarlwang/bufresize.nvim"},
  {'nvim-lua/plenary.nvim'},
  {'lewis6991/gitsigns.nvim'},
})

-------------------------------------------------------------------------------
--- Plugin configs
-------------------------------------------------------------------------------

-- treesitter
require('nvim-treesitter.configs').setup({
  highlight = {
    enable = true,
  },
  -- :help nvim-treesitter-textobjects-modules
  textobjects = {
    select = {
      enable = true,
      lookahead = true,
      keymaps = {
        ['af'] = '@function.outer',
        ['if'] = '@function.inner',
        ['ac'] = '@class.outer',
        ['ic'] = '@class.inner',
      }
    },
  },
  ensure_installed = {
    'c',
    'python',
    'bash',
    'haskell',
  },
})

-- themes
require("evergarden").setup {
    theme = {
        variant = 'winter',
        accent = 'green',
    },
    editor = {
        transparent_background = true,
    },
    style = {
        types = {},
        keyword = {},
    },
}
require("gruvbox").setup({
  bold = false,
  italic = {
    strings = false,
    emphasis = false,
    comments = true,
    operators = false,
    folds = false,
  },
  transparent_mode = true,
})
vim.opt.termguicolors = true
vim.cmd.colorscheme('gruvbox')

-- lualine
require('lualine').setup({
  options = {
    theme = 'gruvbox',
    icons_enabled = true,
    component_separators = '|',
    section_separators = '',
  },
  sections = {
    lualine_a = {'mode'},
    lualine_b = {'filename'},
    lualine_c = {'branch', 'diff'},
    lualine_x = {'tabs', 'progress'},
    lualine_y = {'filetype'},
    lualine_z = {'location'}
  },
  inactive_sections = {
    lualine_a = {},
    lualine_b = {'filename'},
    lualine_c = {},
    lualine_x = {},
    lualine_y = {'location'},
    lualine_z = {}
  },
})

-- Telescope
require("telescope").setup {}
local builtin = require "telescope.builtin"
vim.keymap.set('n', '<leader>ff', builtin.find_files, {})
vim.keymap.set('n', '<leader>fg', builtin.live_grep, {})
vim.keymap.set('n', '<leader>fb', builtin.buffers, {})
vim.keymap.set('n', '<leader>fp', builtin.git_files, {})
vim.keymap.set("n", "<leader>fo", builtin.oldfiles, {})
vim.keymap.set("n", "<leader>T", builtin.colorscheme, {})

require("bufresize").setup()

-----
---- Comment.nvim
-----
--require('Comment').setup({})
--
---- toggleterm
-----
---- See :help toggleterm-roadmap
--require('toggleterm').setup({
--  open_mapping = '<C-g>',
--  direction = 'horizontal',
--  shade_terminals = true
--})
--
