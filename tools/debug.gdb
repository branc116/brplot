b brpl_x11_get_set_context
b brpl_x11_error_callback
tui disable
define pfreelist
  print *($arg0).arr@($arg0).cap
  print *($arg0).free_arr@($arg0).cap
  print ($arg0).len
  print ($arg0).free_len
  print ($arg0).free_next
end

source tools/gdb.py

#define panim
#  set var $an = ($arg0).arr[$arg1]
#  print $an.is_alive
#  if $an.kind == 0
#    printf "Float %d (%f -> %f)\n", $an.is_alive, $an.f.current, $an.f.target
#  else if $an.kind == 1
#    printf "Ex %d ((%.2f, -> %f)\n", $an.is_alive, $an.f.current.x, $an.f.current.y, $an.f.current.width, $an.f.current.height, $an.f.target
#  else
#    print $an.kind
#  end
#end
#
#define pres
#  print *($arg0).arr@($arg0).cap
#  print *($arg0).free_arr@($arg0).cap
#  print ($arg0).len
#  print ($arg0).free_len
#  print ($arg0).free_next
#  set var $i = 0
#  set var $n = ($arg0).len
#  while $i < $n
#    if ($arg0).free_arr[0] == -1
#
#    else
#    end
#    set var $i = $i + 1
#  end
#end
