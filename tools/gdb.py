import gdb

class BrVec2P:
    def __init__(self, val):
        self.val = val
    def to_string(self):
        x = self.val['x']
        y = self.val['y']
        return f'({x}, {y})'

class BrVec3P:
    def __init__(self, val):
        self.val = val
    def to_string(self):
        x = self.val['x']
        y = self.val['y']
        z = self.val['z']
        return f'({x}, {y}, {z})'

class BrColorP:
    def __init__(self, val):
        self.val = val
    def to_string(self):
        r = self.val['r']
        g = self.val['g']
        b = self.val['b']
        a = self.val['a']
        return f'({r}, {g}, {b}, {a})'

class BrExtentP:
    def __init__(self, val):
        self.val = val
    def to_string(self):
        x = self.val['x']
        y = self.val['y']
        w = self.val['width']
        h = self.val['height']
        return f'POS: ({x}, {y}); SIZE: ({w}, {h})'

class BrAnimP:
    def __init__(self, val):
        self.val = val
    def to_string(self):
        kind = str(self.val['kind'])
        is_alive = self.val['is_alive']
        sub = 'f'
        if kind == 'br_anim_extent':
            sub = 'ex'
        if (is_alive):
            cur = self.val[sub]['current']
            target = self.val[sub]['target']
            return f'{cur} -> {target}'
        else:
            cur = self.val[sub]['current']
            return f'{kind} {cur}'

class BrArr:
    def __init__(self, val):
        self.val = val
        self.eltype = self.val['arr'].dereference().type
        self.arrlen = self.val['len']
        try:
            self.is_free = None != self.val['free_arr']
        except:
            self.is_free = False
    def to_string(self):
        if (self.is_free):
            free_len = self.val['free_len']
            return f'{str(self.eltype)}[{free_len}/{self.arrlen}]'
        else:
            return f'{str(self.eltype)}[{self.arrlen}]'
    def children(self):
        for i in range(self.arrlen):
            index = f'{i}'
            if self.is_free:
                if -1 != (self.val['free_arr'] + i).dereference():
                    index = f'-{i}'
            yield index, (self.val['arr'] + i).dereference()

def has_field(val, field_name):
    try:
        t = val.type
        if t.code == 23:
            return field_name in t.keys()

        return False
    except:
        return False

class BrBaseStructP:
    def __init__(self, val, keys):
        self.val = val
        self.keys = keys
    def to_string(self):
        return f'{self.val.type}'
    def children(self):
        for k in self.keys:
            val = self.val[k]
            try:
                if k.endswith("_ah"):
                    arr = gdb.parse_and_eval('brui__stack.anims->all.arr')
                    anim = (arr + val).dereference()
                    yield k, f'{val} ({anim})'
                else:
                    yield k, f'{val}'
            except:
                pass

def my_pp_func(val):
    if has_field(val, 'arr') and has_field(val, 'len'):
        return BrArr(val)
    elif str(val.type) == 'br_vec2_t': return BrVec2P(val)
    elif str(val.type) == 'br_vec2d_t': return BrVec2P(val)
    elif str(val.type) == 'br_vec3_t': return BrVec3P(val)
    elif str(val.type) == 'br_vec3d_t': return BrVec3P(val)
    elif str(val.type) == 'br_color_t': return BrColorP(val)
    elif str(val.type) == 'br_extent_t': return BrExtentP(val)
    elif str(val.type) == 'br_extenti_t': return BrExtentP(val)
    elif str(val.type) == 'br_extentd_t': return BrExtentP(val)
    elif str(val.type) == 'br_anim_t': return BrAnimP(val)
    try:
        keys = val.type.keys()
        try:
            a = val[keys[0]]
        except:
            return None
        return BrBaseStructP(val, keys)
    except:
        return None

gdb.pretty_printers.clear()
gdb.pretty_printers.append(my_pp_func)
